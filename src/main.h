#include <Arduino.h>

#define MAX 50
#define MAX_PART 8
#define MAX_TEXT 2000
#define MAX_WAITING_TIME 600 // 10 минут

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0; // для UTC +3.00: 3 * 60 * 60: 10800
const int   daylightOffset_sec = 0; // летнее время

typedef struct Vault
{
  uint16_t id;
  char     *phone;
  char     *date;
  uint8_t  count;
  char     text[MAX_PART][161];
  time_t   last_update;
  bool     filled;
} Vault;

Vault arr_vault[MAX];
Vault nil_vault;
