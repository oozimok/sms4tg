# sms4tg — транспорт GSM SMS в Telegram
Позволяет реализовать транспорт GSM SMS в сеть Telegram. 
В настоящий момент находится в стадии пре-альфа, однако, уже способен принимать длинные SMS-сообщений в формате PDU и доставлять склеенные сообщения в Telegram.

Транспорт написан в среде PlatformIO с использованием библиотек:
 - GSM/GPRS SIM800 (основана на библиотеке GSM/GPRS Shield A6, http://iarduino.ru)
 - ArduinoJson
 - UniversalTelegramBot

Для сборки необходимо переименовать файл `secrets.example.h` в `secrets.h` и указать в нём настройки WiFi подключения и Telegram бота.