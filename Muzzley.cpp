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
  _rpcs.removeExpiredTimeouts();
}

void Muzzley::removeParticipantById(int pos){
  if(pos >= 0 && pos <= _participants_count){
    for(int i = pos; i < _participants_count; i++){
      _participants[i] = _participants[i+1];
    }
    _participants_count--;
  }
}

void Muzzley::addParticipant(Participant p){
  _participants[_participants_count] = p;
  _participants_count++;
}

Participant Muzzley::getParticipantById(int id){
  Participant p;
  for(int i = 0; i < _participants_count; ++i){
    if(_participants[i].id == id){
      p = _participants[i];
    }
  }
  return p;
}



// Public Methods

Muzzley::Muzzley(){
  _static_activity = false;
  _participants_count = 0;
  strcpy(_server, "geoplatform.muzzley.com");
  _rpcs.registerEvent("on_connect", new Delegate<void, char*>(this, &Muzzley::onConnect));
  _rpcs.registerEvent("on_close", new Delegate<void, char*>(this, &Muzzley::onClose));
  _rpcs.registerEvent("on_handshake", new Delegate<void, char*>(this, &Muzzley::onHandshake));
  _rpcs.registerEvent("on_login_app", new Delegate<void, char*>(this, &Muzzley::onLoginApp));
  _rpcs.registerEvent("on_create_activity", new Delegate<void, char*>(this, &Muzzley::onCreateActivity));
  _rpcs.registerEvent("on_participant_join", new Delegate<void, char*>(this, &Muzzley::onParticipantJoin));
  _rpcs.registerEvent("on_participant_ready", new Delegate<void, char*>(this, &Muzzley::onParticipantReady));
  _rpcs.registerEvent("on_widget_ready", new Delegate<void, char*>(this, &Muzzley::onWidgetReady));
  _rpcs.registerEvent("on_signaling_message", new Delegate<void, char*>(this, &Muzzley::onSignalingMessage));
  _rpcs.registerEvent("on_participant_quit", new Delegate<void, char*>(this, &Muzzley::onParticipantQuit));
}



void Muzzley::setActivityReadyHandler(ActivityReady activity_ready){
  _activity_ready = activity_ready;
}

void Muzzley::setOnCloseHandler(OnClose on_close){
  _on_close = on_close;
}

void Muzzley::setParticipantJoinHandler(ParticipantJoined participant_joined){
  _participant_joined = participant_joined;
}

void Muzzley::setParticipantWidgetChanged(WidgetReady widget_ready){
  _widget_ready = widget_ready;
}

void Muzzley::setSignalingMessagesHandler(OnSignalingMessage on_signaling_message){
  _on_signaling_message = on_signaling_message;
}

void Muzzley::setParticipantQuitHandler(ParticipantQuit participant_quit){
  _on_participant_quit = participant_quit;
}

void Muzzley::connectApp(char *app_token, char *activity_id){
  strcpy(_app_token, app_token);
  if(activity_id != NULL){
    strcpy(_activity_id, activity_id);
    _static_activity = true;
  }
  _rpcs.connect(_server);
}


void Muzzley::changeWidget(int participant_id, char* widget, char* options){
  _rpcs.changeWidget(participant_id, widget, options);
}

void Muzzley::sendSignal(int participant_id, char* type, char* data, SignalCallback callback){
  int msg_type = 5;
  if(callback != NULL){
    UserCallback user_cb;
    sprintf(user_cb.m_cid, "%d",_rpcs.getCurrentCid());
    user_cb.cb = callback;
    _stored_cbs[_waiting_cbs] = user_cb;
    _waiting_cbs++;
    msg_type = 1;
  }
  _rpcs.sendSignal(participant_id, msg_type, type, data);
}


// Events

void Muzzley::onConnect(char* message){
  _rpcs.handshake();
}

void Muzzley::onClose(char* msg){
  if(_on_close != NULL) (*_on_close)();
}


void Muzzley::onHandshake(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    return;
  }
  if(!hashTable.containsKey("s")){
    return;
  }
  char* success = hashTable.getString("s");
  if(strcmp(success, "true") == 0){
    JsonHashTable data = hashTable.getHashTable("d");
    strcpy(_device_id, data.getString("deviceId"));
    _rpcs.loginApp(_app_token);
  }
}


void Muzzley::onLoginApp(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    return;
  }
  if(!hashTable.containsKey("s")){
    return;
  }
  char* success = hashTable.getString("s");
  if(strcmp(success, "true") == 0){
    _rpcs.createActivity(_static_activity, _activity_id);
  }else{
  }

}


void Muzzley::onCreateActivity(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    return;
  }
  if(!hashTable.containsKey("s")){
  }
  char* success = hashTable.getString("s");
  if(strcmp(success, "true") == 0){
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
        _rpcs.connect(_server);
      }else{
      }
    }else{
    }
  }
}


void Muzzley::onParticipantJoin(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    return;
  }
  JsonHashTable key_d = hashTable.getHashTable("d");
  JsonHashTable joiner = key_d.getHashTable("participant");

  Participant p;
  p.id = (int)joiner.getLong("id");
  strcpy(p.profileId, joiner.getString("profileId"));
  strcpy(p.name, joiner.getString("name"));
  strcpy(p.photoUrl, joiner.getString("photoUrl"));
  strcpy(p.deviceId, joiner.getString("deviceId"));
  addParticipant(p);
}


void Muzzley::onParticipantReady(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  JsonHashTable headers = hashTable.getHashTable("h");
  int pid = (int)headers.getLong("pid");
  Participant p = getParticipantById(pid);
  (*_participant_joined)(p);
}

void Muzzley::onWidgetReady(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    return;
  }
  char* success = hashTable.getString("s");
  if(strcmp(success, "true") == 0){
    JsonHashTable headers = hashTable.getHashTable("h");
    int pid = (int)headers.getLong("pid");
    (*_widget_ready)(pid);
  }
}


void Muzzley::onParticipantQuit(char* msg){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(msg);
  if(!hashTable.success()){
    return;
  }

  JsonHashTable headers = hashTable.getHashTable("d");
  int pid = (int)headers.getLong("participantId");
  for(int i = 0; i < _participants_count; ++i){
    if(_participants[i].id == pid){
      removeParticipantById(i);
    }
  }
  (*_on_participant_quit)(pid);
}



void Muzzley::onSignalingMessage(char* msg){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(msg);
  if(!hashTable.success()){
    return;
  }
  JsonHashTable headers = hashTable.getHashTable("h");
  int pid = (int)headers.getLong("pid");
  int request_type = (int) headers.getLong("t");
  char* cid = headers.getString("cid");
  JsonHashTable key_d = hashTable.getHashTable("d");
  if(request_type == 1 || request_type == 5){

    char message_type[30];
    JsonHashTable message;
    if(key_d.containsKey("a")){
      strcpy(message_type, key_d.getString("a"));
    }else{
      strcpy(message_type, "action");
    }
    if(key_d.containsKey("d")){
      message = key_d.getHashTable("d");
    }else{
      message = key_d;
    }
    char* response = (*_on_signaling_message)(pid, message_type, message);
    if(request_type == 1){
      _rpcs.respondToSignal(cid, pid, response);
      return;
    }
  }else if(request_type == 2){
    for(int i = 0; i < _waiting_cbs; ++i){
      if( strcmp(_stored_cbs[i].m_cid, cid) == 0){
        (*_stored_cbs[i].cb)(hashTable.getBool("s"), key_d);
        for(int j = i; j < _waiting_cbs; j++){
          _stored_cbs[j] = _stored_cbs[j+1];
          _waiting_cbs--;
        }
        return;
      } 
    }
  }
}
