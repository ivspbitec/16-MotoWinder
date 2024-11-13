// Buzzer.h
#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
public:
    Buzzer(int pin, int channel);

    void startAutoStopWindingTone();
    void startMemoryClearTone();
    void startKeyTone();
    void startLongKeyTone();
    void startMemKeyTone();
    void startLongMemKeyTone();    
    void startErrorTone(); // Объявление новой функции для сигнала ошибки
    
    void update();

private:
    int buzzerPin;
    int pwmChannel;
    /** Чстота keyTone*/
    int keyToneF = 1000;
    /** Продолжительность keyTone*/
    int keyToneD = 50;

    // Переменные для каждого тона
    bool execAutoStopWindingTone = false;
    bool execMemoryClearTone = false;
    bool execKeyTone = false;
    bool execErrorTone = false;

    unsigned long lastTime = 0;
    int step = 0;
    int toneStep = 0;
    int frequency = 2000;

    void autoStopWindingTone();
    void memoryClearTone();
    void keyTone();
    void errorTone();
};

#endif // BUZZER_H