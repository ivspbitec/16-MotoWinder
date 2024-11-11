// Buzzer.cpp
#include "Buzzer.h"

Buzzer::Buzzer(int pin, int channel)
    : buzzerPin(pin), pwmChannel(channel) {
    pinMode(buzzerPin, OUTPUT);
    ledcAttachPin(buzzerPin, pwmChannel);
    ledcSetup(pwmChannel, 1000, 8); // Установка частоты по умолчанию 1000 Гц, разрешение 8 бит
    ledcWrite(pwmChannel, 0); // Останавливаем звук
}

void Buzzer::startAutoStopWindingTone() {
    execAutoStopWindingTone = true;
    step = 0;
    toneStep = 0;
    frequency = 2000;
    lastTime = millis();
}

void Buzzer::startMemoryClearTone() {
    execMemoryClearTone = true;
    step = 0;
    lastTime = millis();
}

void Buzzer::startKeyTone() {
    execKeyTone = true;
    keyToneD=50;
    keyToneF=1000;
    lastTime = millis();
}

void Buzzer::startLongKeyTone() {
    execKeyTone = true;
    keyToneD=200;
    keyToneF=1000;    
    lastTime = millis();
}

void Buzzer::startMemKeyTone() {
    execKeyTone = true;
    keyToneD=50;
    keyToneF=1500;
    lastTime = millis();
}

void Buzzer::startLongMemKeyTone() {
    execKeyTone = true;
    keyToneD=200;
    keyToneF=1500;    
    lastTime = millis();
}

void Buzzer::autoStopWindingTone() {
    if (!execAutoStopWindingTone) return;

    unsigned long currentMillis = millis();
    if (step % 2 == 0 && currentMillis - lastTime >= 50) {
        frequency = (toneStep % 2 == 0) ? 4000 : 2000;
        ledcWriteTone(pwmChannel, frequency); // Задать частоту через PWM канал
        lastTime = currentMillis;
        step++;
        toneStep++;
    }
    else if (step % 2 == 1 && currentMillis - lastTime >= 50) {
        ledcWrite(pwmChannel, 0); // Останавливаем звук
        lastTime = currentMillis;
        step++;
    } 

    if (step >= 60) {  // Завершаем после 60 шагов
        step = 0;
        execAutoStopWindingTone = false;
    }
}

void Buzzer::memoryClearTone() {
    if (!execMemoryClearTone) return;

    unsigned long currentMillis = millis();
    if (step == 0 && currentMillis - lastTime >= 100) {
        ledcWriteTone(pwmChannel, 800);
        lastTime = currentMillis;
        step = 1;
    }
    else if (step == 1 && currentMillis - lastTime >= 100) {
        ledcWrite(pwmChannel, 0);
        lastTime = currentMillis;
        step = 2;
    }
    else if (step == 2 && currentMillis - lastTime >= 150) {
        ledcWriteTone(pwmChannel, 800);
        lastTime = currentMillis;
        step = 3;
    }
    else if (step == 3 && currentMillis - lastTime >= 100) {
        ledcWrite(pwmChannel, 0);
        step = 0;
        execMemoryClearTone = false;
    }
}

void Buzzer::keyTone() {

     
    if (!execKeyTone) return;

    unsigned long currentMillis = millis();
    if (step == 0) {
        ledcWriteTone(pwmChannel, keyToneF);
        lastTime = currentMillis;
        step = 1;
    }
    else if (step == 1 && currentMillis - lastTime >= keyToneD) {
        ledcWrite(pwmChannel, 0);
        execKeyTone = false;
        step = 0;
    }
}

void Buzzer::update() {
    autoStopWindingTone();
    memoryClearTone();
    keyTone();
}
