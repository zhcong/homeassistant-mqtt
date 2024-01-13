#include "network.h"
#include <Arduino.h>
#include <TaskScheduler.h>
#include <WS2812FX.h>
#include "homeassistant.h"

#define RADAR_PIN 33
#define POWER_LED_PIN 15
#define W_PIN 16
#define REST_BUTTON_PIN 18

char mqttHost[] = "192.168.1.1";
int mqttPort = 1883;
String name = "watchMan3";
String id = "watchMan3";
WS2812FX ws2812fx = WS2812FX(1, W_PIN, NEO_GRB + NEO_KHZ800);

bool radarStatus = false;
bool homeassistantSwitchStatus = false;
int heartCount = 0;
void radarCheck();

Task radarCheckTask(500, TASK_FOREVER, &radarCheck);

Scheduler runner;
Scheduler runner2;

void displayNet(int status) {
  if (CONFIG_FINISH == status) {
    Serial.println("CONFIG_FINISH...");
    ws2812fx.setMode(FX_MODE_STATIC);
    ws2812fx.setColor(100, 255, 0);
  }
  if (CONNECTING == status) {
    Serial.println("CONNECTING...");
    ws2812fx.setMode(FX_MODE_STATIC);
    ws2812fx.setColor(255, 100, 0);
  }
  if (CONFIG_WAIT == status) {
    Serial.println("CONFIG_WAIT...");
    ws2812fx.setColor(255, 255, 0);
    ws2812fx.setMode(FX_MODE_STATIC);
  }
  if (CONNECT_FAIL == status) {
    Serial.println("CONNECT_FAIL...");
    ws2812fx.setColor(255, 0, 0);
    ws2812fx.setMode(FX_MODE_STATIC);
  }
  if (CONNECT_SUCCESS == status) {
    Serial.println("CONNECT_SUCCESS...");
    ws2812fx.setMode(FX_MODE_BREATH);
    ws2812fx.setColor(0, 255, 0);
  }
  ws2812fx.service();
}

void onSwitchChange(bool status) {
  if (status) {
    Serial.println("-ON");
  } else {
    Serial.println("-OFF");
  }
}

void ws2812Init() {
  ws2812fx.init();
  ws2812fx.setBrightness(50);
  ws2812fx.setColor(255, 255, 255);
  ws2812fx.setSpeed(20);
  ws2812fx.setMode(FX_MODE_BREATH);
  ws2812fx.start();
}

void radarCheck() {
  int radarReadStatus = digitalRead(RADAR_PIN);
  if (radarStatus && radarReadStatus) {
    if (!homeassistantSwitchStatus) {
      homeassistantSwitch(true);
      homeassistantSwitchStatus = true;
    }
  }
  if (!radarStatus && !radarReadStatus) {
    if (homeassistantSwitchStatus) {
      homeassistantSwitch(false);
      homeassistantSwitchStatus = false;
    }
  }
  radarStatus = radarReadStatus;
  if (heartCount == 120) {
    homeassistantHeartSend();
    heartCount = 0;
  } else {
    heartCount++;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(RADAR_PIN, INPUT);
  // pinMode(POWER_LED_PIN, OUTPUT);
  // digitalWrite(POWER_LED_PIN, HIGH);

  ws2812Init();
  runner.init();
  runner2.init();
  Serial.println("start net...");
  networkInit(&displayNet, runner, REST_BUTTON_PIN);
  Serial.println("start homeassistant...");
  homeassistantInit(id, name, mqttHost, mqttPort, &onSwitchChange, runner, false, SWITCH_TYPE, "");
  Serial.println("over.");

  runner2.addTask(radarCheckTask);
  radarCheckTask.enable();
}

void loop() {
  runner.execute();
  runner2.execute();
  homeassistantLoop();
  ws2812fx.service();
  resetButtonCheck();
}
