IntelGalileoWebsocketClient
==============================

Websocket client ported for [INTEL GALILEO](http://arduino.cc/en/ArduinoCertified/IntelGalileo) boards.

This library supports Sec-WebSocket-Version: 13 and binary **multiple** binary frame messaging.

Install the library to "libraries" folder in your Sketchbook folder. 



### Usage Example
``` galileo
#include <JsonObjectBase.h>
#include <Muzzley.h>
#include <WSClient.h>
#include <Callback.h>
#include <JsonHashTable.h>
#include <JsonParser.h>
#include <RpcManager.h>
#include <JsonArray.h>
#include <Ethernet.h>


byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };


Muzzley muzzley;

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);
  delay(2000);
  muzzley.setActivityReadyHandler(activityCreated);
  muzzley.setParticipantJoinHandler(participantJoined);
  
  muzzley.connectApp("muzzlionaire");
}

void loop() {
  muzzley.nextTick();
}


void activityCreated(char* activityId, char* qrCodeUrl, char* deviceId) {
  Serial.println("--------- Activity Created -------");
  Serial.println(activityId);
  Serial.println(qrCodeUrl);
  Serial.println(deviceId);
}

void participantJoined(Participant p){
  Serial.println("------ Participant joined -------");
  Serial.println(p.id);
  Serial.println(p.profileId);
  Serial.println(p.name);
  Serial.println(p.photoUrl);
  Serial.println(p.deviceId);
}
```



### Credits
Special thanks to:
  - krohling [ArduinoWebsocketClient](https://github.com/krohling/ArduinoWebsocketClient)
  - djsb [arduino-websocketclient](https://github.com/djsb/arduino-websocketclient)
  - adamvr [Base64](https://github.com/adamvr/arduino-base64)


This library is general purpose but was made to support the muzzley galileo library (soon available on github). Check [Muzzley](http://www.muzzley.com) for more details.
