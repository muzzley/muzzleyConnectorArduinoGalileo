#include <JsonObjectBase.h>
#include <Muzzley.h>
#include <WSClient.h>
#include <Callback.h>
#include <JsonHashTable.h>
#include <JsonParser.h>
#include <RpcManager.h>
#include <JsonArray.h>



#include <Ethernet.h>


Muzzley muzzley;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };


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
  muzzley.changeWidget(p.id, "webview", "{\"uuid\":\"123456-17f7-4652-ab00-b3a815017b01\"}");
  //muzzley.changeWidget(p.id, "gamepad");
}



void handleMySignalResponse(bool success, JsonHashTable msg){
  Serial.println("Received callback from webview");
  Serial.println(msg.getString("key"));
}

void widgetChanged(int participant_id){
  Serial.println("Widget has changed");
  Serial.println(participant_id);

}


char * onSignalingMessage(int participant_id, char* type, JsonHashTable message){
  Serial.println(participant_id);
  Serial.println(type);
  
  if(strcmp(type, "gogo")==0){
    Serial.println("Received message from webview");
    Serial.println("Responding to webview");
    return("\"s\":true, \"d\":{\"test\":\"Galileo says hi!\"}"); 
  }
  
  if(strcmp(type, "go")==0){
    Serial.println("Received message from webview");
    Serial.println(message.getString("key"));
    return NULL; 
  }
  
  if(strcmp(type, "triggerme")==0){
    Serial.println("Received message from webview");
    Serial.println("Request to send something");
    muzzley.sendSignal(participant_id, "testme", "{\"key\":\"la vai chourico\"}", handleMySignalResponse);
    return NULL; 
  }
  
  
  if(strcmp(type, "action")==0){
    //widget data 
    Serial.println("WIDGET DATA");
    char* wvar = message.getString("w");
    Serial.println(wvar);
  }else{
    //custom signaling data 
    //Serial.println("CUSTOM SIGNAL MESSAGE");
    //return "{s:true, m: , d:{}}";
    //return NULL;
  }
  return NULL;
}

void onParticipantQuit(int pid){
  Serial.println("Participant quit");
}

void onClose(){
  Serial.println("Connection to muzzley lost"); 
}

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);
  delay(2000);
  muzzley.setActivityReadyHandler(activityCreated);
  muzzley.setParticipantJoinHandler(participantJoined);
  muzzley.setParticipantWidgetChanged(widgetChanged);
  muzzley.setSignalingMessagesHandler(onSignalingMessage);
  muzzley.setParticipantQuitHandler(onParticipantQuit);
  muzzley.setOnCloseHandler(onClose);
  
  muzzley.connectApp("my_token");
}


void loop() {
  muzzley.nextTick();
}


