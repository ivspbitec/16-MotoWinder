#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <EEPROM.h>

#define STEP_PIN 2        // Пин для STEP драйвера A4988
#define DIR_PIN 3         // Пин для DIR драйвера A4988
#define BUTTON_PIN 4      // Пин для кнопки старт/стоп
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

unsigned long lastPressTime = 0;        // Время последнего нажатия на кнопку
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

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
U8G2_FOR_ADAFRUIT_GFX u8g2; // Создаем объект для работы с кириллицей

// Глобальная структура JSON
DynamicJsonDocument jsonData(1024);

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


    digitalWrite(DIR_PIN, HIGH); // Устанавливаем направление вращения
    digitalWrite(LED_PIN, HIGH); // Устанавливаем направление вращения

    // Настройка ШИМ на STEP_PIN
    ledcSetup(0, frequency, 8); // Канал 0, частота 1 кГц, разрешение 8 бит
    ledcAttachPin(STEP_PIN, 0); // Привязываем канал 0 к STEP_PIN

    // Initialize I2C communication on the correct pins
    Wire.begin(6, 7); // SDA = GPIO6, SCL = GPIO7
    EEPROM.begin(512);
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
    u8g2.setFont(u8g2_font_7x13_t_cyrillic); // Подключаем шрифт с кириллицей
    u8g2.setCursor(0, 20);
    u8g2.print(F("Старт..."));
    display.display();

    delay(500);
    // Инициализация JSON-структуры
    jsonData["time"]["label"] = "Время";
    jsonData["time"]["value"] = "00:00";
    jsonData["meters"]["label"] = "Метры";
    jsonData["meters"]["value"] = 0;
    jsonData["isrunning"]["label"] = "Вкл";
    jsonData["isrunning"]["value"] = "нет";
    jsonData["freq"]["label"] = "Част.";
    jsonData["freq"]["value"] = "0";

    // Начальный вывод на дисплей
    updateDisplay();
}

float calculateRevolutions(unsigned long lastUpdateTime, float curFrequency)
{

    unsigned long timeElapsed = millis() - lastUpdateTime;
    if (timeElapsed > 0)
    {
        float revolutions = curFrequency * (timeElapsed / 1000.0);
        totalRevolutions += revolutions / 800;
    }
    return totalRevolutions;
}

void updateMetersStep()
{
    static unsigned long lastTime = 0;
    if (millis() - lastTime > 500)
    {
        totalRevolutions = calculateRevolutions(lastTime, curFrequency);
        if (maxRevolutions && totalRevolutions >= maxRevolutions)
        {
            stopWinding();
        }
        lastTime = millis();
    }
}

void updateDisplayStep()
{

    static unsigned long lastTime = 0;
    if (millis() - lastTime > 500)
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
    EEPROM.write(0, maxRevolutions);
    EEPROM.commit();
    blinkLED(4);
}

/** Читаем из памяти */
void loadMem()
{
    EEPROM.get(0, maxRevolutions);
}

void onMemButtonPress()
{
    maxRevolutions = totalRevolutions;
    Serial.println("Сохраняем значение...");
    EEPROM.put(0, maxRevolutions);

    Serial.print("Текущее значение в EEPROM: ");
    Serial.println(EEPROM.read(0));
}

void checkButtonStep(uint8_t pin, void (*shortPressFunc)(), void (*longPressFunc)())
{
    int buttonState = digitalRead(pin);

    if (buttonState == LOW)
    {
        if (!lastPressTime)
        {
            lastPressTime = millis();
        }
    }
    else
    {
        if (lastPressTime > 0)
        {
            if (millis() - lastPressTime >= longPressDuration)
            {
                longPressFunc();
                //  blinkLED(2);
                // jsonData["isrunning"]["value"] = "2";
            }
            else
            {
                if (millis() - lastPressTime > 6)
                {
                    //   jsonData["isrunning"]["value"] = "1";
                    //  blinkLED(1);
                    shortPressFunc();
                }
            }
        }

        lastPressTime = 0;
    }
}

void checkButtonsStep()
{
    checkButtonStep(BUTTON_PIN, onButtonPress, onButtonLongPress);
    checkButtonStep(MEM_BUTTON_PIN, onMemButtonPress, onMemButtonLongPress);
}

void loop()
{

    checkButtonsStep();

    updateDisplayStep();
    updateMetersStep();

    // Чтение состояния кнопки
    /*if (digitalRead(BUTTON_PIN) == LOW) {
      if (millis() - lastPressTime > longPressDuration) {
        // Долгое нажатие - очищаем память
        clearMemory();
        memoryCleared = true;
        lastPressTime = millis();  // Обновляем время последнего нажатия
      }
    } else {

     if (millis() - lastPressTime < longPressDuration && !isRunning && !isPaused && !memoryCleared) {

        // Первое нажатие - запуск моталки или возобновление
        startWinding();
        lastPressTime = millis();
      } /*else if (isRunning) {
        // Пауза при повторном нажатии
        pauseWinding();
        lastPressTime = millis();
      }

      memoryCleared = false;  // Сбрасываем флаг после нажатия
    }
  */

    // Автоматическая остановка по достижению сохраненного времени
    /* if (isRunning && !isPaused && millis() - startTime >= currentRunTime) {
       stopWinding();
     }*/
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
    // Имитация сохранения данных (можно использовать EEPROM или другую память)
    // Однократное мигание встроенным светодиодом
    blinkLED(1);
}

void clearMemory()
{
    // Очищаем память о клубке
    currentRunTime = 0;
    // Трёхкратное мигание встроенным светодиодом
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

    // display.clearDisplay();

    if (line1 != lastDisplay[0])
    {
        u8g2.setCursor(0, 11);
        u8g2.print(line1);
    }

    if (line2 != lastDisplay[1])
    {
        u8g2.setCursor(0, 22);
        u8g2.print(line2);
    }

    if (line3 != lastDisplay[2])
    {
        u8g2.setCursor(0, 33);
        u8g2.print(line3);
    }

    if (line4 != lastDisplay[3])
    {
        u8g2.setCursor(0, 44);
        u8g2.print(line4);
    }

    if (line5 != lastDisplay[4])
    {
        u8g2.setCursor(0, 55);
        u8g2.print(line5);
    }

    display.display();

    lastDisplay[0] = line1;
    lastDisplay[1] = line2;
    lastDisplay[2] = line3;
    lastDisplay[3] = line4;
    lastDisplay[4] = line5;
}