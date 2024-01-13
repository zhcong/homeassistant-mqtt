#include <ArduinoMqttClient.h>
#include <TaskScheduler.h>

char mqtt_buffer[256] = { 0 };

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

void connectMQTTClientIfNeeded();
Task mqttCheck(120000, TASK_FOREVER, &connectMQTTClientIfNeeded);

void (*onMqttMessageHandle)(String, String);

void onMqttMessageHandleCall(int messageSize) {
  int len = 0;
  while (mqttClient.available() && len < 256) {
    mqtt_buffer[len++] = (char)mqttClient.read();
  }
  mqtt_buffer[len] = 0;
  String value = String(mqtt_buffer);
  (*onMqttMessageHandle)(mqttClient.messageTopic(), value);
}

void mqttInit(const char mqttHostP[], int mqttPortP, const char mqttTopicP[], void (*onMqttMessageP)(String, String), Scheduler *runnerP) {
  if (!mqttClient.connect(mqttHostP, mqttPortP)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1)
      ;
  }
  mqttClient.onMessage(onMqttMessageHandleCall);
  mqttClient.subscribe(mqttTopicP);

  runnerP->addTask(mqttCheck);
  mqttCheck.enable();

  onMqttMessageHandle = onMqttMessageP;
}

void connectMQTTClientIfNeeded() {
  if (!mqttClient.connected()) {
    delay(10 * 1000);
    ESP.restart();
  }
}

void mqttSend(const char topic[], String value) {
  mqttClient.beginMessage(topic);
  mqttClient.print(value);
  mqttClient.endMessage();
}

void mqttDisable() {
  mqttClient.stop();
  mqttCheck.disable();
}

void mqttLoop(String topic) {
  if (mqttClient.messageTopic() == topic) {
    int len = 0;
    while (mqttClient.available() && len < 256) {
      mqtt_buffer[len++] = (char)mqttClient.read();
    }
    mqtt_buffer[len] = 0;
    String value = String(mqtt_buffer);
  }
  mqttClient.poll();
}