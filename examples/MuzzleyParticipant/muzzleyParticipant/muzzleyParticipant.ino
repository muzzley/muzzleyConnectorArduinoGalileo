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
}


/** My custom signal callback */
void handleMySignalResponse(bool success, JsonHashTable msg){
  Serial.println("Received callback from webview");
  Serial.println(msg.getString("key"));
}


/** When a signal arrives */
char * onSignalingMessage(int participant_id, char* type, JsonHashTable message){
  
  if(message.containsKey("mykey")){
    char * mykey = message.getString("mykey");
    Serial.println(mykey);
    
    if( strcmp(mykey,"with_cb") == 0 ){
      Serial.println("With CB");
      return ("\"s\":true, \"d\":{\"test\":\"Galileo says hi!\"}");
    }
    
    if( strcmp(mykey,"without_cb") == 0 ){
      Serial.println("Without CB");
      return NULL;
    }
    
    if( strcmp(mykey,"touch_to_send") == 0 ){
      Serial.println("Sending Hi without callback");
      muzzley.sendSignal("testme", "{\"key\":\"Test me\"}", NULL);
      return NULL;
    }
    
    if( strcmp(mykey,"touch_to_send_with_cb") == 0 ){
      Serial.println("ending Hi with callback");
      muzzley.sendSignal("callme", "{\"key\":\"call me\"}", handleMySignalResponse);
    }
  }
  
  return NULL;
}


void onClose(){
  Serial.println("Connection to muzzley lost"); 
}

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);
  delay(2000);
  
  //Declare your event handlers
  muzzley.setParticipantJoinHandler(participantJoined);
  muzzley.setSignalingMessagesHandler(onSignalingMessage);
  muzzley.setOnCloseHandler(onClose);

  muzzley.connectUser("guest", "06c893");
}


void loop() {
  muzzley.nextTick();
}