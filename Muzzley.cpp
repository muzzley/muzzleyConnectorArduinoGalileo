#include <Muzzley.h>
#include "WSClient.h"
#include "Callback.h"
#include "JsonObjectBase.h"
#include "JsonHashTable.h"
#include "JsonParser.h"
#include "RpcManager.h"


// Private Methods

void Muzzley::nextTick(){
  _rpcs.next();
}
/**
void RpcManager::removeParticipant(int pos){
  for(int i = pos; i < _rpcs_count; i++){
    _rpcs[i] = _rpcs[i+1];
    _rpcs_count--;
  }
}
*/
void Muzzley::addParticipant(Participant p){
  _participants[_participants_count] = p;
  _participants_count++;
}





// Public Methods

Muzzley::Muzzley(){
 // _on_connect = new Delegate<void, char*>(this, &Muzzley::onConnect);
  strcpy(_server, "geoplatform.muzzley.com");
  _rpcs.registerEvent("on_connect", new Delegate<void, char*>(this, &Muzzley::onConnect));
  _rpcs.registerEvent("on_handshake", new Delegate<void, char*>(this, &Muzzley::onHandshake));
  _rpcs.registerEvent("on_login_app", new Delegate<void, char*>(this, &Muzzley::onLoginApp));
  _rpcs.registerEvent("on_create_activity", new Delegate<void, char*>(this, &Muzzley::onCreateActivity));
  _rpcs.registerEvent("on_participant_join", new Delegate<void, char*>(this, &Muzzley::onParticipantJoin));
  _rpcs.registerEvent("on_participant_ready", new Delegate<void, char*>(this, &Muzzley::onParticipantReady));
}



void Muzzley::setActivityReadyHandler(ActivityReady activity_ready){
  _activity_ready = activity_ready;
}

void Muzzley::setParticipantJoinHandler(ParticipantJoined participant_joined){
  _participant_joined = participant_joined;
}


void Muzzley::connectApp(char *app_token, char *activity_id){
  strcpy(_app_token, app_token);
  if(activity_id != NULL){
    strcpy(_activity_id, activity_id);
    _static_activity = true;
  }
  _rpcs.connect(_server);
}




// Events

void Muzzley::onConnect(char* message){
  Serial.print("[Done]");
  _rpcs.handshake();
}


void Muzzley::onHandshake(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    //call Muzzley onError
    Serial.print("[Handshake Failed] - Error parsing Json");
    return;
  }
  if(!hashTable.containsKey("s")){
    Serial.println("[Handshake failed]");
  }
  char* success = hashTable.getString("s");
  if(strcmp(success, "true") == 0){
    JsonHashTable data = hashTable.getHashTable("d");
    strcpy(_device_id, data.getString("deviceId"));
    Serial.print("[Handshake Done]");
    _rpcs.loginApp(_app_token);
  }else{
    Serial.print("[Handshake Failed]");
  }
}


void Muzzley::onLoginApp(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    //call Muzzley onError
    Serial.print("[loginApp Failed] - Error parsing Json");
    return;
  }
  if(!hashTable.containsKey("s")){
    Serial.println("[LoginApp failed]");
  }
  char* success = hashTable.getString("s");
  if(strcmp(success, "true") == 0){
    Serial.print("[LoginApp Done]");
    _rpcs.createActivity(_static_activity, _activity_id);
  }else{
    Serial.print("[LoginApp Failed]");
  }

}


void Muzzley::onCreateActivity(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    //call Muzzley onError
    Serial.print("[CreateActivity Failed] - Error parsing Json");
    return;
  }
  if(!hashTable.containsKey("s")){
    Serial.println("[CreateActivity failed]");
  }
  char* success = hashTable.getString("s");
  if(strcmp(success, "true") == 0){
    Serial.print("[CreateActivity Done]");
    JsonHashTable key_d = hashTable.getHashTable("d");
    char* activityId = key_d.getString("activityId");
    char* qrCodeUrl = key_d.getString("qrCodeUrl");
    char* deviceId = key_d.getString("deviceId");
    (*_activity_ready)(activityId, qrCodeUrl, deviceId);
  }else{
    JsonHashTable key_d = hashTable.getHashTable("d");
    if(key_d.success() == true){
      if(key_d.containsKey("connectTo")){
        strcpy(_server,key_d.getString("connectTo"));
        Serial.println(_server);
        _rpcs.connect(_server);
      }else{
        Serial.println("ConnecTo not present");
      }
    }else{
     Serial.println("No d key present");
    }
  }
}


void Muzzley::onParticipantJoin(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    //call Muzzley onError
    Serial.print("Error parsing Json");
    return;
  }
  JsonHashTable key_d = hashTable.getHashTable("d");
  JsonHashTable joiner = key_d.getHashTable("participant");

  Participant p;
  p.id = joiner.getLong("id");
  strcpy(p.profileId, joiner.getString("profileId"));
  strcpy(p.name, joiner.getString("name"));
  strcpy(p.photoUrl, joiner.getString("photoUrl"));
  strcpy(p.deviceId, joiner.getString("deviceId"));
 addParticipant(p);
}



void Muzzley::onParticipantReady(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    //call Muzzley onError
    Serial.print("Error parsing Json");
    return;
  }

}

