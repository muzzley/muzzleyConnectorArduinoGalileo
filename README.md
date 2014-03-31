IntelGalileoWebsocketClient
==============================

Websocket client ported for [INTEL GALILEO](http://arduino.cc/en/ArduinoCertified/IntelGalileo) boards.

This library supports Sec-WebSocket-Version: 13 and binary **multiple** binary frame messaging.

Install the library to "libraries" folder in your Sketchbook folder. 



### Usage Example
``` galileo
#include <Ethernet.h>
#include <Base64.h>
#include <WSClient.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
WSClient client;

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);
  delay(1000);
  client.connect("echo.websocket.org", 80, "/");
  Serial.println("Handshaked");
  client.setMessageHandler(onMessage);
  client.send("Hello world");
}

void loop() {
  delay(50);
  client.listen();
}

void onMessage(WSClient client, String data) {
  Serial.println("--- Got message ----");
  Serial.println(data);
}
```



### Credits
Special thanks to:
  - krohling [ArduinoWebsocketClient](https://github.com/krohling/ArduinoWebsocketClient)
  - djsb [arduino-websocketclient](https://github.com/djsb/arduino-websocketclient)
  - adamvr [Base64](https://github.com/adamvr/arduino-base64)


This library is general purpose but was made to support the muzzley galileo library (soon available on github). Check [Muzzley](http://www.muzzley.com) for more details.