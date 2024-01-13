#include "esp32-hal.h"
#include <WiFi.h>
#include <EEPROM.h>
#include <TaskScheduler.h>

#define LENGTH(x) (strlen(x) + 1)  // length of char string
#define EEPROM_SIZE 100            // EEPROM size
#define BUTTON_DELAY 500

#define CONFIG_WAIT 2
#define CONFIG_FINISH 1
#define CONNECTING 3
#define CONNECT_FAIL 4
#define CONNECT_SUCCESS 5
#define FLAG_ADDRESS 0

String ssid;
String pss;
int reset_button_pin;

void beginWiFiIfNeeded();
Task wifiCheck(30000, TASK_FOREVER, &beginWiFiIfNeeded);

bool checkConnectFlag() {
  int flag = EEPROM.read(FLAG_ADDRESS);
  return flag;
}

void writeConnectFlag(int flag) {
  EEPROM.write(FLAG_ADDRESS, flag);
  EEPROM.commit();
}

void writeStringToFlash(const char* toStore, int startAddr) {
  int i = 0;
  for (; i < LENGTH(toStore); i++) {
    EEPROM.write(startAddr + i, toStore[i]);
  }
  EEPROM.write(startAddr + i, '\0');
  EEPROM.commit();
}


String readStringFromFlash(int startAddr) {
  char in[128];  // char array of size 128 for reading the stored data
  int i = 0;
  for (; i < 128; i++) {
    in[i] = EEPROM.read(startAddr + i);
  }
  return String(in);
}

void savePassword() {
  ssid = WiFi.SSID();
  pss = WiFi.psk();
  writeStringToFlash(ssid.c_str(), 1);  // storing ssid at address 0
  writeStringToFlash(pss.c_str(), 41);  // storing pss at address 40
}

void resetButtonCheck() {
  if (!digitalRead(reset_button_pin)) {
    delay(BUTTON_DELAY);
    writeConnectFlag(false);
    ESP.restart();
  }
}

void networkInit(void (*notify)(int), Scheduler* runner, int reset_button_pin_P) {
  EEPROM.begin(EEPROM_SIZE);
  pinMode(reset_button_pin_P, INPUT_PULLUP);
  if (!checkConnectFlag()) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.beginSmartConfig(SC_TYPE_ESPTOUCH_AIRKISS);
    while (!WiFi.smartConfigDone()) {
      (*notify)(CONFIG_WAIT);
      delay(500);
    }
    writeConnectFlag(true);
    savePassword();
    (*notify)(CONFIG_FINISH);
    reset_button_pin = reset_button_pin_P;
  }

  if (WiFi.status() != WL_CONNECTED) {
    (*notify)(CONNECTING);
    WiFi.disconnect(true, false);
    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(true);
    ssid = readStringFromFlash(1);  // Read SSID stored at address 0
    pss = readStringFromFlash(41);  // Read Password stored at address 40
    WiFi.begin(ssid.c_str(), pss.c_str());
  }

  int count = 30;
  while (WiFi.status() != WL_CONNECTED && count > 0) {
    count--;
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED) {
    (*notify)(CONNECT_FAIL);
    count = 30;
    while (count > 0) {
      count--;
      delay(1000);
      resetButtonCheck();
    }
    ESP.restart();
  } else {
    (*notify)(CONNECT_SUCCESS);
  }
  runner -> addTask(wifiCheck);
  wifiCheck.enable();
}

void beginWiFiIfNeeded() {
  if (WiFi.status() == WL_NO_SHIELD || WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_CONNECTION_LOST || WiFi.status() == WL_DISCONNECTED) {
    delay(10 * 1000);
    ESP.restart();
  }
}