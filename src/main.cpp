#include <Arduino.h>

#define STEP_PIN 2   // Пин для STEP драйвера A4988
#define DIR_PIN 3    // Пин для DIR драйвера A4988
#define BUTTON_PIN 4 // Пин для кнопки
#define LED_PIN 8    // Пин для светодиода
#define STEPS_PER_REV 200 // Количество шагов на один оборот двигателя

void setup() { 
    // Инициализация пинов
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP); // Включаем внутренний подтягивающий резистор
    pinMode(LED_PIN, OUTPUT);

    digitalWrite(DIR_PIN, HIGH); // Устанавливаем направление вращения двигателя
     Serial.println("Turn on!");
      
}

void loop() {
    // Проверяем, нажата ли кнопка
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Включаем светодиод при нажатии кнопки
        digitalWrite(LED_PIN, LOW);
        Serial.println("Кнопка нажата!");

        // Запускаем двигатель на один оборот
        for (int i = 0; i < STEPS_PER_REV*10; i++) {
            digitalWrite(STEP_PIN, HIGH);
            delayMicroseconds(300); // Настраиваем задержку для управления скоростью
            digitalWrite(STEP_PIN, LOW);
            delayMicroseconds(300);
        }
    } else {
        // Выключаем светодиод, когда кнопка не нажата
        digitalWrite(LED_PIN, HIGH);
    }
}
