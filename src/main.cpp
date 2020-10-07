#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <main.h>
#include <secrets.h>

#include <sim800.h>
sim800 gsm;

#include <SoftwareSerial.h> 
SoftwareSerial SIM800(18,19);

char     textSMS[161];
char     phoneSMS[13];
char     dateSMS[18];
uint16_t idSMS;
uint8_t  countSMS;
uint8_t  numSMS;
 
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

bool f = false;
unsigned long bot_lasttime;
time_t date_now = time(nullptr);

void botSetup()
{
  const String commands = F("["                   
    "{\"command\":\"id\",\"description\":\"Идентификатор канала\"},"
    "{\"command\":\"signal\",  \"description\":\"Уровень сигнала GSM модема\"},"
    "{\"command\":\"balance\",  \"description\":\"Запрос баланса\"}"
  "]");
  bot.setMyCommands(commands);
}

void handleNewMessages(int numNewMessages) 
{
    for (int i=0; i<numNewMessages; i++) 
    {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;

        if (text == "/start") 
        {
            bot.sendMessage(chat_id, "Добро пожаловать транспорт GSM SMS в Telegram.\n");
        }

        if (text == "/id") 
        {
            bot.sendMessage(chat_id, "Ваш идентификатор чата: " + chat_id + ".\n");
        }

        if (text == "/signal") 
        {
            bot.sendMessage(chat_id, "Уровень сигнала: " + String(gsm.signal()) + ".\n");
        }

        if (text == "/balance") 
        {
            bot.sendChatAction(chat_id, "typing");
            String balance = gsm.runUSSD("*100#", 30000);
            bot.sendMessage(chat_id, "Баланс: " + balance + ".\n");
        }
    }
}

void handleCompletedMessages(UniversalTelegramBot bot) 
{
    for(int i = 0; arr_vault[i].phone; i++)
    {
        char text[MAX_TEXT] = "";
        for (int j = 0; j < MAX_PART; j++)
        {
            strcat(text, arr_vault[i].text[j]); 
        }
        if (
            arr_vault[i].filled || 
            (arr_vault[i].last_update + MAX_WAITING_TIME) < date_now
        ) {
            bot.sendMessage(CHAT_ID, "Время: " + String(arr_vault[i].date) + "\nНомер: " + String(arr_vault[i].phone) + "\n" + String(text));
            arr_vault[i] = nil_vault;
        }
    }
}

void handleAddMessage(char text[161], char phone[13], char date[18], uint16_t id, uint8_t count, uint8_t num)
{
    // определяем индекс сообщения в хранилище
    int i;
    for(i = 0; arr_vault[i].phone; i++) 
    {
        if (arr_vault[i].phone == phone && arr_vault[i].id == id) 
        {
            break;
        }
    }
    // сохраняем сообщение
    arr_vault[i].id = id;
    arr_vault[i].phone = phone;
    arr_vault[i].date = date;
    arr_vault[i].count = count;
    arr_vault[i].last_update = date_now;
    strcpy(arr_vault[i].text[num], text);
    // проверяем получение полного сообщения
    int part = 0;
    for (int j = 0; j < MAX_PART; j++) 
    {
        part += (strcmp(arr_vault[i].text[j], nil_vault.text[0]) != 0);
    }
    arr_vault[i].filled = (arr_vault[i].count == part);
}

void setup() 
{
    // выводим текст в монитор последовательного порта
    Serial.begin(9600);

    Serial.print( F("Подключение к Wifi  ... ") );
    Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.print("\nWiFi подключен. IP адрес: ");
    Serial.println(WiFi.localIP());

    // получаем текущее время
    Serial.print("Время получения: ");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    while (date_now < 24 * 3600)
    {
        Serial.print(".");
        delay(100);
        date_now = time(nullptr);
    }
    Serial.println(date_now);

    // инициируем GSM/GPRS модема и проверяем его готовность к работе
    Serial.print( F("Подготовка GSM модема к работе ... ") );
    gsm.begin(SIM800);

    // проверяем готовность GSM/GPRS модема к работе
    switch( gsm.status() )
    {                                                                   
      case GSM_OK          :                                                           break; // модуль готов к работе.
      case GSM_REG_NO      :                                                           break; // требуется время ...,      на данный момент модем не зарегистрирован в сети оператора связи.
      case GSM_SPEED_ERR   : Serial.println("UART ошибка установки скорости.");  f=1;  break; // модуль не может работать, не удалось согласовать скорость UART.
      case GSM_UNAVAILABLE : Serial.println("Модуль недоступен.");               f=1;  break; // модуль не может работать, он недоступен и не выполят AT-команды.
      case GSM_UNKNOWN     : Serial.println("Неизвестный статус модуля.");       f=1;  break; // модуль не может работать, его статус неизвестен и корректное выполнение AT-команд не гарантируется.
      case GSM_SLEEP       : Serial.println("Модуль в спящем режиме.");          f=1;  break; // модуль не может работать, он находится в режиме ограниченной функциональности.
      case GSM_SIM_NO      : Serial.println("Нет SIM-карты.");                   f=1;  break; // модуль не может работать, нет SIM-карты.
      case GSM_SIM_FAULT   : Serial.println("Неисправная SIM-карта.");           f=1;  break; // модуль не может работать, SIM-карта неисправна.
      case GSM_SIM_ERR     : Serial.println("SIM-карта не работает.");           f=1;  break; // модуль не может работать, SIM-карта не прошла проверку.
      case GSM_REG_FAULT   : Serial.println("Регистрация отклонена.");           f=1;  break; // модуль не может работать, оператор сотовой связи отклонил регистрацию модема в своей сети.
      case GSM_REG_ERR     : Serial.println("Статус регистрации не определен");  f=1;  break; // модуль не может работать, статус регистрации в сети оператора не читается.
      case GSM_SIM_PIN     : Serial.println("Введите PIN-код");                  f=1;  break; // требуется ввод PIN-кода
      case GSM_SIM_PUK     : Serial.println("Введите PUK-код");                  f=1;  break; // требуется ввод PUK1
      default              : /* неизвестное состояние */                         f=1;  break; // требуется ввод PIN2, требуется ввод PUK2
    }
    // ждём завершения регистрации модема в сети оператора связи
    while(gsm.status()==GSM_REG_NO){Serial.print("."); delay(1000);}
    // eсли модуль не может работать, то не даём скетчу выполняться дальше
    if(f){Serial.println( "Остановка работы программы." );} while(f){;}
    // eсли модуль может работать, то информируем об успешной инициализации
    Serial.println(" OK!");
    Serial.println( F("--------------------") );

    // установка кодировки для символов Кириллицы:
    gsm.TXTsendCodingDetect("п");

    // настраиваем Telegram бота
    botSetup();
} 
          
void loop ()
{
    if(millis()%1000<100)
    {
        delay(100);
        if(gsm.SMSavailable())
        {
            Serial.println( F("Новое SMS, читаю ...") );
            gsm.SMSread(textSMS, phoneSMS, dateSMS, idSMS, countSMS, numSMS);

            Serial.print  ( F("SMS ")                   ); Serial.print  ( numSMS );
            Serial.print  ( F(" из ")                   ); Serial.print  ( countSMS );
            Serial.print  ( F(", ID=")                  ); Serial.print  ( idSMS );
            Serial.print  ( F(", отправлен ")           ); Serial.print  ( dateSMS );
            Serial.print  ( F(" с номера ")             ); Serial.print  ( phoneSMS );
            Serial.println( F(", текстовое сообщение:") ); Serial.println( textSMS );
            Serial.println( F("--------------------")   );

            handleAddMessage(textSMS, phoneSMS, dateSMS, idSMS, countSMS, numSMS);
        }

        handleCompletedMessages(bot);

        int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        while(numNewMessages) 
        {
            Serial.println( F("Новое Telegram сообщение") );
            handleNewMessages(numNewMessages);
            numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }
    }
    
}
