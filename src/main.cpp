#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <EEPROM.h>

#define STEP_PIN 2        // Пин для STEP драйвера A4988
#define DIR_PIN 3         // Пин для DIR драйвера A4988
#define BUTTON_PIN 4       // Пин для кнопки старт/стоп
#define MEM_BUTTON_PIN 5  // Пин для кнопки памяти
#define STEPS_PER_REV 800 // Количество шагов на один оборот двигателя
#define LED_PIN 8         // Встроенный светодиод на ESP32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

int frequency = 0;                // Начальная частота ШИМ (частота шагов)
int maxFrequency = 10000;         // Максимальная частота ШИМ для полной скорости
int curFrequency = 0;             // Текущая частота ШИМ для полной скорости
int accelDelay = 15;              // Задержка для плавного изменения скорости (мс)
int decelDelay = 3;               // Задержка для плавного изменения скорости (мс)
int accelStep = 100;              // Задержка для плавного изменения скорости (мс)
bool isRunning = false;           // Флаг состояния работы моталки
unsigned long startTime = 0;      // Время начала намотки
unsigned long currentRunTime = 0; // Сохраненное время намотки
bool isPaused = false;            // Флаг паузы

bool memoryCleared = false; // Флаг очистки памяти

float totalRevolutions = 0; // Общее количество оборотов
float maxRevolutions = 0;   // Максимальное количество оборотов 

 
unsigned long longPressDuration = 1000; // Время для долгого нажатия

// OLED reset pin (set to -1 if not used)
#define OLED_RESET -1

void clearMemory();
void startWinding();
void pauseWinding();
void stopWinding();
void accelerateMotor();
void decelerateMotor();
void saveToMemory();
 
void updateDisplay();
void loadMem();

void handleInterruptPinStart();
void handleInterruptPinMem();

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2; // Создаем объект для работы с кириллицей

// Глобальная структура JSON
DynamicJsonDocument jsonData(512);

// Переменные для хранения предыдущих строк
String lastDisplay[5] = {"", "", "", "", ""};


void blinkLED(int times,uint32_t delayMs = 200)
{
    for (int i = 0; i < times; i++)
    {
        digitalWrite(LED_PIN, LOW);   
        delay(delayMs);                   
        digitalWrite(LED_PIN, HIGH);  
        delay(delayMs);                  
    }
}


void setup()
{


    // Настройка пинов
    pinMode(DIR_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);     // Кнопка с подтяжкой
    pinMode(MEM_BUTTON_PIN, INPUT_PULLUP); // Кнопка mem с подтяжкой
    pinMode(LED_PIN, OUTPUT);              // Встроенный светодиод
 
    Serial.begin(115200); // Инициализация последовательного порта
    while (!Serial)
    {
        ; // Ожидание, пока последовательный порт не будет готов (это может быть необходимо для некоторых плат)
    }
    blinkLED(5,50);
    
     Serial.println(F("ESP Starting..."));


    digitalWrite(DIR_PIN, HIGH); // Устанавливаем направление вращения
    digitalWrite(LED_PIN, HIGH); // Устанавливаем направление вращения

    // Настройка ШИМ на STEP_PIN
    ledcSetup(0, frequency, 8); // Канал 0, частота 1 кГц, разрешение 8 бит
    ledcAttachPin(STEP_PIN, 0); // Привязываем канал 0 к STEP_PIN

    // Initialize I2C communication on the correct pins
    Wire.begin(6, 7); // SDA = GPIO6, SCL = GPIO7
    EEPROM.begin(64);
    loadMem();

    // Initialize the OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
        while (true)
            ; // Halt if display fails to initialize
    }

    display.clearDisplay();
    display.display();

    u8g2.begin(display);
    u8g2.setFont(u8g2_font_7x13_t_cyrillic);  
    u8g2.setCursor(0, 20);
    u8g2.print(F("Запуск...")); 
 
 

    display.display();

    delay(1000);
    // Инициализация JSON-структуры
    jsonData["time"]["label"] = "Время";
    jsonData["time"]["value"] = "00:00";
    jsonData["meters"]["label"] = "Метры";
    jsonData["meters"]["value"] = 0;
    jsonData["isrunning"]["label"] = "Вкл";
    jsonData["isrunning"]["value"] = "нет";
    jsonData["freq"]["label"] = "Част.";
    jsonData["freq"]["value"] = "0";
    jsonData["maxMeters"]["label"] = "Память";
    jsonData["maxMeters"]["value"] = "0";

    // Начальный вывод на дисплей
    updateDisplay();
    
    //Кнопки
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleInterruptPinStart, CHANGE);
    attachInterrupt(digitalPinToInterrupt(MEM_BUTTON_PIN), handleInterruptPinMem, CHANGE);
}

void calculateRevolutions(unsigned long lastUpdateTime, float curFrequency)
{

    unsigned long timeElapsed = millis() - lastUpdateTime;
    if (timeElapsed > 0)
    {
        float revolutions = curFrequency * (timeElapsed / 1000.0);
        totalRevolutions += revolutions / 800;
    }
   
}

/* Стстаус остановки по памяти 0 - небыло остановки 1 - была остановка */
bool isStopedOnMem = false;
void updateMetersStep()
{
    static unsigned long lastTime = 0;

    if (millis() - lastTime > 500)
    {
        calculateRevolutions(lastTime, curFrequency);
        if (maxRevolutions>0 && totalRevolutions >= maxRevolutions && !isStopedOnMem )
        {             
            isStopedOnMem=true;
            stopWinding();
            totalRevolutions=0;
            isStopedOnMem=false;
        }
        lastTime = millis();
    }
}

void updateDisplayStep()
{

    static unsigned long lastTime = 0;
    if (millis() - lastTime > 250)
    {
        // Обновляем значения в JSON-структуре
        if (startTime)
        {
            jsonData["time"]["value"] = String("00:") + String(((millis() - startTime) / 1000) % 60);
        }
        else
        {
            jsonData["time"]["value"] = String("0");
        }

        jsonData["meters"]["value"] = totalRevolutions;
        jsonData["isrunning"]["value"] = isRunning ? "да" : "нет";
        jsonData["freq"]["value"] = curFrequency;
        jsonData["maxMeters"]["value"] = maxRevolutions;

        updateDisplay();
        lastTime = millis();
    }
}

void onButtonPress()
{
    if (!isRunning)
    {
        startWinding();
        blinkLED(3);
    }
    else
    {
        stopWinding();
    }
}

void onButtonLongPress()
{
    blinkLED(2);
}

/** Стираем память*/
void onMemButtonLongPress()
{
    maxRevolutions = 0;
    totalRevolutions = 0;
    EEPROM.put(0, maxRevolutions);
    EEPROM.commit();
    blinkLED(4);
}

/** Читаем из памяти */
void loadMem()
{
     EEPROM.get(0, maxRevolutions);
      if (isnan(maxRevolutions)){
        maxRevolutions=0.0;
      }
}

void onMemButtonPress()
{
    maxRevolutions = totalRevolutions;
    Serial.print("Сохраняем значение... ");
    Serial.println(maxRevolutions);
    EEPROM.put(0, maxRevolutions);
    EEPROM.commit();
    totalRevolutions=0;

    Serial.print("Текущее значение в EEPROM: ");
    float test;
    EEPROM.get(0,test);
    Serial.println(test);
}




void checkButtonStep(uint8_t pin, unsigned long &lastPressTime, void (*shortPressFunc)(), void (*longPressFunc)())
{

   // delay(10);
             
   
    int buttonState = digitalRead(pin);

         Serial.print(" pin=");
         Serial.print(pin);
         Serial.print(" state=");
         Serial.print(buttonState);
         Serial.print(" lt=");
         Serial.println(lastPressTime);


    if (buttonState == LOW){
         if (lastPressTime==0){
            lastPressTime = millis();
        }
    }
     
     if (buttonState == HIGH ){
          
     if (lastPressTime>0 && millis() -lastPressTime>5){

        if (millis() - lastPressTime >= longPressDuration)
            {
                 lastPressTime = 0;
                  Serial.print(" pin=");
         Serial.print(pin);
                //longPressFunc();
                //  blinkLED(pin,20);
                // jsonData["isrunning"]["value"] = "2";
                Serial.println("longpress");
            }
            else
            {
                 lastPressTime = 0;
                  Serial.print(" pin=");
         Serial.print(pin);
                 
                    //   jsonData["isrunning"]["value"] = "1";
                    //  blinkLED(1);
                         Serial.println("press");
                         
                     //  blinkLED(pin,50);
                //    shortPressFunc();
                   // Serial.println("stop func");
              
            }
        }
           

     }
}


  unsigned long lastPressTimeButton = 0;
        unsigned long lastPressTimeMem = 0;
 
void IRAM_ATTR handleInterruptPinStart() {
   checkButtonStep(BUTTON_PIN, lastPressTimeButton, onButtonPress, onButtonLongPress);
}

void IRAM_ATTR handleInterruptPinMem() {
     
    checkButtonStep(MEM_BUTTON_PIN, lastPressTimeMem, onMemButtonPress, onMemButtonLongPress);
   
}


void loop()
{ 
    
    updateDisplayStep();
    updateMetersStep();
}

void startWinding()
{
    if (currentRunTime == 0)
    {
        // Если нет сохраненного значения, начинаем новый отсчет
        startTime = millis();
        currentRunTime = 0;
    }
    else
    {
        // Восстанавливаем намотку по сохраненному времени
        startTime = millis();
    }
    isRunning = true;
    accelerateMotor();
}

void pauseWinding()
{
    isPaused = true;
    currentRunTime = millis() - startTime; // Сохраняем текущее время намотки
    decelerateMotor();
    isRunning = false;
    saveToMemory(); // Сохраняем в память
}

void stopWinding()
{
    decelerateMotor();
    isRunning = false;
    isPaused = false;
}

void saveToMemory()
{
     blinkLED(1);
}

void clearMemory()
{
     currentRunTime = 0;
     blinkLED(3);
}



void accelerateMotor()
{
    for (int freq = frequency; freq <= maxFrequency; freq += accelStep)
    {
        ledcWriteTone(0, freq); // Изменяем частоту ШИМ для плавного разгона
        curFrequency = freq;
        delay(accelDelay);
        updateDisplayStep();
        updateMetersStep();
    }
}

void decelerateMotor()
{
    for (int freq = maxFrequency; freq >= frequency; freq -= accelStep)
    {
        ledcWriteTone(0, freq); // Изменяем частоту ШИМ для плавного торможения
        curFrequency = freq;
        delay(decelDelay);
        updateDisplayStep();
        updateMetersStep();
    }
    ledcWrite(0, 0); // Полная остановка ШИМ
}

// Работа с экраном

void updateDisplay()
{
    String line1 = String(jsonData["time"]["label"].as<const char *>()) + ": " + String(jsonData["time"]["value"].as<const char *>()) + "     ";
    String line2 = String(jsonData["meters"]["label"].as<const char *>()) + ": " + String(jsonData["meters"]["value"].as<int>()) + "     ";
    String line3 = String(jsonData["isrunning"]["label"].as<const char *>()) + ": " + String(jsonData["isrunning"]["value"].as<const char *>()) + "     ";
    String line4 = String(jsonData["freq"]["label"].as<const char *>()) + ": " + String(jsonData["freq"]["value"].as<int>()) + "     ";
    String line5 = String(jsonData["maxMeters"]["label"].as<const char *>()) + ": " + String(jsonData["maxMeters"]["value"].as<int>()) + "     ";

      

    if (
        line1 != lastDisplay[0]
        || line2 != lastDisplay[1]
        || line3 != lastDisplay[2]
        || line4 != lastDisplay[3]
        || line5 != lastDisplay[4]
    )
    {
        int lineHeight=11;
        display.clearDisplay();
        u8g2.setFont(u8g2_font_6x12_t_cyrillic);  


        u8g2.setFont(u8g2_font_10x20_t_cyrillic);   
        u8g2.setCursor(0, 15);
        u8g2.print(String(String(jsonData["meters"]["value"].as<int>()) + " м."));

        u8g2.setFont(u8g2_font_6x12_t_cyrillic);  
        u8g2.setCursor(0, 16+lineHeight*1);
        u8g2.print(line1);

        
        u8g2.setCursor(0, 16+lineHeight*2);
        u8g2.print(line3);
     
        u8g2.setCursor(0, 16+lineHeight*3);
        u8g2.print(line4);
     
        u8g2.setCursor(0, 16+lineHeight*4);
        u8g2.print(line5);

         display.display();
    }
 

    lastDisplay[0] = line1;
    lastDisplay[1] = line2;
    lastDisplay[2] = line3;
    lastDisplay[3] = line4;
    lastDisplay[4] = line5;
}