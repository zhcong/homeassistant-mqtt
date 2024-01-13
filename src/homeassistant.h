#include "HWCDC.h"
#include "esp32-hal.h"
#include "mqtt.h"

#define SWITCH_TYPE 0
#define SENEOR_TYPE 1

String switchConfigTopicTemplate = "homeassistant/switch/%s/config";
String switchConfigTemplate = "{ \"unique_id\": \"%s\", \"name\": \"%s\", \"state_topic\": \"home/%s/state\", \"command_topic\": \"home/%s/set\", \"payload_on\": \"ON\", \"payload_off\": \"OFF\" }";

String sensorConfigTopicTemplate = "homeassistant/sensor/%s/config";
String sensorConfigTemplate = "{ \"unique_id\": \"%s\", \"name\": \"%s\",\"state_topic\": \"home/%s/state\", \"value_template\":\"{{value_json.value}}\",\"unit_of_measurement\":\"%s\"}";

String switchTopicStateTemplate = "home/%s/state";
String switchTopicSetTemplate = "home/%s/set";

char buffer[128];
String switchStateTopic;
String switchSetTopic;
String haConfigTopic = "";
String haConfig;
bool isResp;

void (*onSwitch)(bool);
void homeassistantHeartSend();
Task homeassistantHeartTask(60000, TASK_FOREVER, &homeassistantHeartSend);

void homeassistantSwitch(boolean payLoad) {
  if (payLoad) {
    mqttSend(switchStateTopic.c_str(), "ON");
  } else {
    mqttSend(switchStateTopic.c_str(), "OFF");
  }
}

void homeassistantSensor(float level) {
  sprintf(buffer, "{\"value\":%f}", level);
  mqttSend(switchStateTopic.c_str(), String(buffer));
}

void onMqttMessage(String topic, String value) {
  (*onSwitch)(value == "ON");
  if (isResp) {
    homeassistantSwitch(value == "ON");
  }
}

void homeassistantReg(String id, String name, int type, String unit) {
  if (SWITCH_TYPE == type) {
    sprintf(buffer, switchConfigTopicTemplate.c_str(), id);
    haConfigTopic = String(buffer);
    sprintf(buffer, switchConfigTemplate.c_str(), id, name, id, id);
    haConfig = String(buffer);
  } else if (SENEOR_TYPE == type) {
    sprintf(buffer, sensorConfigTopicTemplate.c_str(), id);
    haConfigTopic = String(buffer);
    sprintf(buffer, sensorConfigTemplate.c_str(), id, name, id, unit);
    haConfig = String(buffer);
  } else {
  }
  mqttSend(haConfigTopic.c_str(), haConfig);
}

void homeassistantInit(String id, String name, const char mqttHostP[], int mqttPortP, void (*onSwitchP)(bool), Scheduler* runnerP, bool isRespP, int type, String unit) {
  sprintf(buffer, switchTopicStateTemplate.c_str(), id);
  switchStateTopic = String(buffer);
  sprintf(buffer, switchTopicSetTemplate.c_str(), id);
  switchSetTopic = String(buffer);
  mqttInit(mqttHostP, mqttPortP, switchSetTopic.c_str(), &onMqttMessage, runnerP);
  homeassistantReg(id, name, type, unit);

  runnerP->addTask(homeassistantHeartTask);
  homeassistantHeartTask.enable();

  onSwitch = onSwitchP;
  isResp = isRespP;
}

void homeassistantHeartSend() {
  Serial.println(haConfigTopic);
  mqttSend(haConfigTopic.c_str(), haConfig);
}

void homeassistantLoop() {
  mqttLoop(switchSetTopic);
}