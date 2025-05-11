#include "esp32-hal.h"
#include <WiFi.h>
#include <TaskScheduler.h>
#include <Preferences.h>

#define BUTTON_DELAY 500

#define CONFIG_WAIT 2
#define CONFIG_FINISH 1
#define CONNECTING 3
#define CONNECT_FAIL 4
#define CONNECT_SUCCESS 5
#define FLAG_ADDRESS 0

String ssid;
String password;
int reset_button_pin;
Preferences preferences;

void beginWiFiIfNeeded();
Task wifiCheck(30000, TASK_FOREVER, &beginWiFiIfNeeded);

bool checkConnectFlag() {
  return preferences.getBool("flag", false);
}

void writeConnectFlag(int flag) {
  preferences.putBool("flag",flag);
}

void savePassword() {
  ssid = WiFi.SSID();
  password = WiFi.psk();
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
}

void resetButtonCheck() {
  if (!digitalRead(reset_button_pin)) {
    delay(BUTTON_DELAY);
    writeConnectFlag(false);
    ESP.restart();
  }
}

void networkInit(void (*notify)(int), Scheduler* runner, int reset_button_pin_P) {
  preferences.begin("wifi-conf", false); 
  pinMode(reset_button_pin_P, INPUT_PULLUP);
  reset_button_pin = reset_button_pin_P;
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
  }

  if (WiFi.status() != WL_CONNECTED) {
    (*notify)(CONNECTING);
    WiFi.disconnect(true, false);
    WiFi.setAutoReconnect(true);
    ssid = preferences.getString("ssid","");
    password = preferences.getString("password","");
    WiFi.begin(ssid.c_str(), password.c_str());
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