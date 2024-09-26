#include <Arduino.h>

 

#define STEP_PIN 2    // Пин для STEP драйвера A4988
#define DIR_PIN 3     // Пин для DIR драйвера A4988
#define BUTTON_PIN 4  // Пин для кнопки
#define LED_PIN 8     // Пин для светодиода
#define STEPS_PER_REV 800  // Количество шагов на один оборот двигателя

int minDelay = 30;       // Минимальная задержка (микросекунды) между шагами (максимальная скорость)
int maxDelay = 150;      // Максимальная задержка (микросекунды) между шагами (минимальная скорость)
int accelerationSteps = 6400; // Количество шагов для плавного ускорения и замедления

void setup() {
    // Инициализация пинов
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Включаем внутренний подтягивающий резистор
    pinMode(LED_PIN, OUTPUT);

    // Инициализация последовательного порта
    Serial.begin(19200);
    Serial.println("START!");

    digitalWrite(DIR_PIN, HIGH);  // Устанавливаем направление вращения двигателя
}




// Функция для одного шага двигателя
void stepMotor(int delayTime) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(delayTime);  // Задержка после импульса
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(delayTime);  // Задержка перед следующим шагом
}

// Функция для расчета квадратичной задержки
int getQuadraticDelay(int step, int totalSteps, int startDelay, int endDelay) {
    // Квадратичное изменение задержки
    float progress = (float)step / totalSteps;  // Нормализованный шаг (от 0 до 1)
    int delayTime = startDelay + (endDelay - startDelay) * (progress * progress);  // Квадратичное изменение
 
    return delayTime;
}

// Функция для управления шаговым двигателем с плавным ускорением и торможением
void runMotorWithAccelerationAndDeceleration(int steps) {
    int currentDelay; // Переменная для хранения текущей задержки

    // Плавное ускорение
    for (int i = 0; i < accelerationSteps; i++) {
        currentDelay = getQuadraticDelay(i, accelerationSteps, maxDelay, minDelay);
        stepMotor(currentDelay);
    }

    // Движение с постоянной скоростью
    for (int i = 0; i < (steps - 2 * accelerationSteps); i++) {
        stepMotor(minDelay);  // Минимальная задержка на этапе максимальной скорости
    }

    // Плавное торможение
    for (int i = 0; i < accelerationSteps; i++) {
        currentDelay = getQuadraticDelay(i, accelerationSteps, minDelay, maxDelay);
        stepMotor(currentDelay);
    }
}



void loop() {
    // Проверяем, нажата ли кнопка
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Выводим сообщение в порт
        Serial.println("Кнопка нажата!");

        // Включаем светодиод при нажатии кнопки
        digitalWrite(LED_PIN, HIGH);

        // Запускаем двигатель на один оборот с плавным ускорением и торможением
        runMotorWithAccelerationAndDeceleration(STEPS_PER_REV*150);
    } else {
        // Выключаем светодиод, когда кнопка не нажата
        digitalWrite(LED_PIN, LOW);
    }
}
