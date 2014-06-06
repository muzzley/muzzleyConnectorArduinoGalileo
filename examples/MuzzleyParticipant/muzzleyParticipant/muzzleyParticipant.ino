/**
  Use your Galileo as a Muzzley client, connected to a specific activity
*/

#include <Muzzley.h>
#include <JsonObjectBase.h>
#include <JsonHashTable.h>
#include <JsonParser.h>
#include <JsonArray.h>
#include <Ethernet.h>


Muzzley muzzley;
int ledPin = 13;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

/** When a participant joins the activity */
void participantJoined(Participant p){
  Serial.println("------ Participant joined -------");
  Serial.println(p.id);
  Serial.println(p.profileId);
  Serial.println(p.name);
  Serial.println(p.photoUrl);
  Serial.println(p.deviceId);
  Serial.println(p.context);
}


/** My custom signal callback */
void handleMySignalResponse(bool success, JsonHashTable msg){
  Serial.println("Received callback from webview");
  Serial.println(msg.getString("key"));
}

void onActivityTerminated(){
  Serial.println("Activity terminated..you need to manually reconnect");
  //Muzzley.reconnect()..
}

/** When a signal arrives */
char * onSignalingMessage(char* type, JsonHashTable message){
 
  if(message.containsKey("mykey")){
    char * mykey = message.getString("mykey");
    Serial.println(mykey);
  }
  
  return NULL;
}


void onClose(char* msg){
  Serial.print("Connection to muzzley lost: ");
  Serial.println(msg);
}

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);
  delay(2000);
  
  //Declare your event handlers
  muzzley.setParticipantJoinHandler(participantJoined);
  muzzley.setActivityTerminatedHandler(onActivityTerminated);
  muzzley.setSignalingMessagesHandler(onSignalingMessage);
  muzzley.setOnCloseHandler(onClose);

  muzzley.connectUser("guest", "55930c");
}

unsigned long ref_time = millis();


void sendBoardExecutionTime(){
  char signal[100];
  sprintf(signal, "{\"measure\":\"running_since\",\"value\":\"%d\",\"units\":\"milliseconds\"}", millis());
  muzzley.sendSignal("galileo_1", signal, handleMySignalResponse);
}

void loop() {
  muzzley.nextTick();
  
  //Sends Muzzley app the time since the program started every 5 seconds aprox..
  if(millis()>=(ref_time+10000)){
    sendBoardExecutionTime();
    ref_time = millis();
  }
}