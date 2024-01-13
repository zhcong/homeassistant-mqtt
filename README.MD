# homeassistant-mqtt
homeassistant client for switch and sensor, include mqtt and network, just for esp32 under arduino.
## usage
```c++
#include "homeassistant.h"
#include "network.h"

void setpup(){
    ...
    networkInit(&displayNet, runner, REST_BUTTON_PIN);
    homeassistantInit(id, name, mqttHost, mqttPort, &onSwitchChange, runner, false, SWITCH_TYPE, "");
}

void loop(){
    homeassistantLoop();
    resetButtonCheck();
}
```
for complete examples, see [example](examples/watchMan/watchMan.ino).

## ⚠attention
- you need a button to reset the wifi, and also cut this function.
- sensor type now support those:
``` C++
#define SWITCH_TYPE 0
#define SENEOR_TYPE 1
```
## 🫵I WANT YOU
there are still many things that haven't been done yet, like
- make those code with object-oriented.
- support more sensor type in homeassistant  

<b><i>become the contributor of this project.</i></b>