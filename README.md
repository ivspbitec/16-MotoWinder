*ESP32 C3 super mini 17HS4410 controller

Ничего специально делать не нужно, даже на кнопки нажимать на микроконтроллере ESP32 C3 Super mini для прошивки достаточно подключить USB Type C с передачей данных и запустить VSCode с PlatformIO и скетчем
Для меня подошла плата 

board = esp32-c3-m1i-kit

единственный нюанс который обнаружился возмоно из за сомнительного сопоставления плат - это инверсность встроенного голубого диода, что никак не вляет на остальную работу пинов 