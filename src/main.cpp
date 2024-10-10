#include <Arduino.h>

#define STEP_PIN 2    // Пин для STEP драйвера A4988
#define DIR_PIN 3     // Пин для DIR драйвера A4988
#define BUTTON_PIN 4  // Пин для кнопки
#define STEPS_PER_REV 800  // Количество шагов на один оборот двигателя
#define LED_PIN 8 // Встроенный светодиод на ESP32

int frequency = 0;         // Начальная частота ШИМ (частота шагов)
int maxFrequency = 20000;      // Максимальная частота ШИМ для полной скорости
int accelDelay = 15;          // Задержка для плавного изменения скорости (мс)
int accelStep = 200;          // Задержка для плавного изменения скорости (мс)
bool isRunning = false;       // Флаг состояния работы моталки
unsigned long startTime = 0;  // Время начала намотки
unsigned long currentRunTime = 0; // Сохраненное время намотки
unsigned long lastPressTime = 0;  // Время последнего нажатия на кнопку
bool isPaused = false;        // Флаг паузы
unsigned long longPressDuration = 3000; // Время для долгого нажатия (3 секунды)
bool memoryCleared = false;   // Флаг очистки памяти


void clearMemory();
void startWinding();
void pauseWinding();
void stopWinding();
void accelerateMotor();
void decelerateMotor();
void saveToMemory();
void blinkLED(int times);

void setup() {
  // Настройка пинов
  pinMode(DIR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Кнопка с подтяжкой
  pinMode(LED_PIN, OUTPUT);       // Встроенный светодиод

  digitalWrite(DIR_PIN, HIGH);        // Устанавливаем направление вращения

  digitalWrite(LED_PIN, HIGH);        // Устанавливаем направление вращения

  // Настройка ШИМ на STEP_PIN
  ledcSetup(0, frequency, 8);  // Канал 0, частота 1 кГц, разрешение 8 бит
  ledcAttachPin(STEP_PIN, 0);  // Привязываем канал 0 к STEP_PIN
}

void loop() {
    if (digitalRead(BUTTON_PIN) == LOW  && !isRunning) {
        startWinding();
        blinkLED(3);
    }

    if (digitalRead(BUTTON_PIN) == LOW  && isRunning) {
        stopWinding();
    }

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

void startWinding() {
 /*  if (currentRunTime == 0) {
    // Если нет сохраненного значения, начинаем новый отсчет
    startTime = millis();
    currentRunTime = 0;
  } else {
    // Восстанавливаем намотку по сохраненному времени
    startTime = millis();
  }*/
  isRunning = true;
  accelerateMotor();
}

void pauseWinding() {
  isPaused = true;
  currentRunTime = millis() - startTime;  // Сохраняем текущее время намотки
  decelerateMotor();
  isRunning = false;
  saveToMemory();  // Сохраняем в память
}

void stopWinding() {
  decelerateMotor();
  isRunning = false;
  isPaused = false;
}

void saveToMemory() {
  // Имитация сохранения данных (можно использовать EEPROM или другую память)
  // Однократное мигание встроенным светодиодом
  blinkLED(1);
}

void clearMemory() {
  // Очищаем память о клубке
  currentRunTime = 0;
  // Трёхкратное мигание встроенным светодиодом
  blinkLED(3);
} 

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, LOW);  // Включаем светодиод
    delay(200);                       // Задержка 200 мс
    digitalWrite(LED_PIN, HIGH);   // Выключаем светодиод
    delay(200);                       // Задержка 200 мс
  }
}

void accelerateMotor() {
  for (int freq = frequency; freq <= maxFrequency; freq += accelStep) {
    ledcWriteTone(0, freq);  // Изменяем частоту ШИМ для плавного разгона
    delay(accelDelay);
  }
}

void decelerateMotor() {
  for (int freq = maxFrequency; freq >= frequency; freq -= accelStep) {
    ledcWriteTone(0, freq);  // Изменяем частоту ШИМ для плавного торможения
    delay(accelDelay);
  }
  ledcWrite(0, 0);  // Полная остановка ШИМ
}
