/**
Control onboard led using a mobile switch widget
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

/** When an activity is ready */
void activityCreated(char* activityId, char* qrCodeUrl, char* deviceId) {
  Serial.println("--------- Activity Created -------");
  Serial.println(activityId);
  Serial.println(qrCodeUrl);
  Serial.println(deviceId);
}

/** When a participant joins the activity */
void participantJoined(Participant p){
  Serial.println("------ Participant joined -------");
  Serial.println(p.id);
  Serial.println(p.profileId);
  Serial.println(p.name);
  Serial.println(p.photoUrl);
  Serial.println(p.deviceId);
  muzzley.changeWidget(p.id, "switch", "{\"isOn\":0}");
}


/** When a participant widget has changed */
void widgetChanged(int participant_id){
  Serial.println("Widget has changed");
  Serial.println(participant_id);

}

/** My custom signal callback */
void handleMySignalResponse(bool success, JsonHashTable msg){
  Serial.println("Received callback from webview");
  Serial.println(msg.getString("key"));
}


/** When a signal arrives */
char * onSignalingMessage(int participant_id, char* type, JsonHashTable message){
  
 /**
    From the Muzzley widget documentation
    The expected widget message is:
    
    {
      "w": "switch",
      "e": "press",
      "c": "switch",
      "v": 1
    }
 */
  if(message.containsKey("w")){
    if(strcmp("switch", message.getString("w")) == 0){
      Serial.println("Message from Switch widget");
      
      if(message.containsKey("v")){
        Serial.println(message.getLong("v"));
        
        if(message.getLong("v") == 1){
          digitalWrite(ledPin, HIGH);
          Serial.println("Setting value high");
        }else{
          digitalWrite(ledPin, LOW);
          Serial.println("Setting value low");
        }
      }    
    }
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
  
  pinMode(ledPin, LOW);
  
  //Declare your event handlers
  muzzley.setActivityReadyHandler(activityCreated);
  muzzley.setParticipantJoinHandler(participantJoined);
  muzzley.setParticipantWidgetChanged(widgetChanged);
  muzzley.setSignalingMessagesHandler(onSignalingMessage);
  muzzley.setParticipantQuitHandler(onParticipantQuit);
  muzzley.setOnCloseHandler(onClose);
  pinMode(ledPin, OUTPUT);

  muzzley.connectApp("my_app_token");
}


void loop() {
  muzzley.nextTick();
}