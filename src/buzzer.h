// Buzzer.h
#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

const int buzzerPin = 10;          // Пин для буззера (GPIO8)
unsigned long previousMillis = 0; // Переменная для отслеживания времени
int toneDuration = 100;           // Продолжительность сигнала в миллисекундах
bool toneOn = false;              // Состояние буззера




int execAutoStopWindingTone=false;
void autoStopWindingTone() {
   if (!execAutoStopWindingTone) return; 
  static unsigned long lastTime = 0;
  static int step = 0;
  static int toneStep = 0;
  static int f = 2000;

  if (step % 2 == 0   && millis() - lastTime >= 100) { // Звук на 100 мс
    if (toneStep %2 == 0 ){
        f=4000;
    }else if (toneStep % 2 == 1 ){
        f=2000;
    }
    tone(buzzerPin, f);
    lastTime = millis();
    step++;
    toneStep++;
  
  } else if (step % 2 == 1 && millis() - lastTime >= 100) { // Пауза на 100 мс
    noTone(buzzerPin);
    lastTime = millis();
    step++;
  }
  if (step >= 60) {  // Повторяем 5 раз (всего 10 шагов)
    step = 0;
    execAutoStopWindingTone=false;
  }
}

int execMemoryClearTone=false;
void memoryClearTone() {
   if (!execMemoryClearTone) return;
  static unsigned long lastTime = 0;
  static int step = 0;

  if (step == 0 && millis() - lastTime >= 100) {
    tone(buzzerPin, 800);
    lastTime = millis();
    step = 1;
  } else if (step == 1 && millis() - lastTime >= 100) {
    noTone(buzzerPin);
    lastTime = millis();
    step = 2;
  } else if (step == 2 && millis() - lastTime >= 150) {
    tone(buzzerPin, 800);
    lastTime = millis();
    step = 3;
  } else if (step == 3 && millis() - lastTime >= 100) {
    noTone(buzzerPin);
    step = 0; // Возвращаемся к началу для следующего вызова
    execMemoryClearTone=false;
  }
}

int execKeyTone=false;
void keyTone() {
   if (!execKeyTone) return;
  
   static unsigned long lastTime = 0;
  static bool toneOn = false;

  if (!toneOn && millis() - lastTime >= 10) {
    tone(buzzerPin, 500);
    toneOn = true;
    lastTime = millis();
  } else if (toneOn && millis() - lastTime >= 500) {
    noTone(buzzerPin);
    toneOn = false;
    execKeyTone=false;
  }

}

void tonesStep(){
    memoryClearTone();
    autoStopWindingTone();
    keyTone();
}




#endif // BUZZER_H