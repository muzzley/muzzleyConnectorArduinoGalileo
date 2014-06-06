#include <Muzzley.h>
#include "WSClient.h"
#include "Callback.h"
#include "JsonObjectBase.h"
#include "JsonHashTable.h"
#include "JsonParser.h"
#include "RpcManager.h"


// Private Methods
void Muzzley::nextTick(){
  if(_rpcs.isConnectionIdle() && _reconnect && !_connecting){
    reconnect("Connection is Idle");
  }
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
  if(_participants_count == sizeof(_participants)-1){
    _participants_count = 0;
  }
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
  _reconnect = false;
  _connecting = false;
  _ready = false;
  _participants_count = 0;
  strcpy(_server, "geoplatform.muzzley.com");
  _rpcs.registerEvent("on_connect", new Delegate<void, char*>(this, &Muzzley::onConnect));
  _rpcs.registerEvent("on_close", new Delegate<void, char*>(this, &Muzzley::onClose));
  _rpcs.registerEvent("on_handshake", new Delegate<void, char*>(this, &Muzzley::onHandshake));
  _rpcs.registerEvent("on_create_activity", new Delegate<void, char*>(this, &Muzzley::onCreateActivity));
  _rpcs.registerEvent("on_activity_joined", new Delegate<void, char*>(this, &Muzzley::onActivityJoined));
  _rpcs.registerEvent("on_activity_terminated", new Delegate<void, char*>(this, &Muzzley::onActivityTerminated));
  _rpcs.registerEvent("on_participant_join", new Delegate<void, char*>(this, &Muzzley::onParticipantJoin));
  _rpcs.registerEvent("on_participant_ready", new Delegate<void, char*>(this, &Muzzley::onParticipantReady));
  _rpcs.registerEvent("on_widget_ready", new Delegate<void, char*>(this, &Muzzley::onWidgetReady));
  _rpcs.registerEvent("on_signaling_message", new Delegate<void, char*>(this, &Muzzley::onSignalingMessage));
  _rpcs.registerEvent("on_participant_quit", new Delegate<void, char*>(this, &Muzzley::onParticipantQuit));
}



void Muzzley::setActivityReadyHandler(ActivityReady activity_ready){
  _activity_ready = activity_ready;
}

void Muzzley::setActivityTerminatedHandler(OnActivityTerminated handler){
  _activity_terminated = handler;
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

void Muzzley::setSignalingMessagesHandler(OnSignalingMessageWithParticipant on_signaling_message){
  _on_signaling_message_p = on_signaling_message;
}
void Muzzley::setSignalingMessagesHandler(OnSignalingMessageWithoutParticipant on_signaling_message){
  _on_signaling_message = on_signaling_message;
}

void Muzzley::setParticipantQuitHandler(ParticipantQuit participant_quit){
  _on_participant_quit = participant_quit;
}

void Muzzley::reconnect(char* msg){
  _rpcs.disconnect(msg);
  delay(5000);
  #ifdef DEBUG
    Serial.print("#DEBUG#  Reconnecting..");
  #endif
  _rpcs.connect();
}

void Muzzley::connectApp(char *app_token, char *activity_id){
  #ifdef DEBUG
    Serial.print("#DEBUG#  Connecting App (token: ");
    Serial.print(app_token);
    Serial.print(", activity: ");
    Serial.print(activity_id);
    Serial.print(")... ");
  #endif
  _app = true;
  _reconnect = true;
  strcpy(_app_token, app_token);
  if(activity_id != NULL){
    strcpy(_activity_id, activity_id);
    _static_activity = true;
  }
  _rpcs.registerEvent("on_login_app", new Delegate<void, char*>(this, &Muzzley::onLoginApp));
  _rpcs.connect(_server);
}


void Muzzley::connectUser(char *user_token, char *activity_id){
  #ifdef DEBUG
    Serial.print("#DEBUG#  Connecting as Participant (token: ");
    Serial.print(user_token);
    Serial.print(", activity: ");
    Serial.print(activity_id);
    Serial.println(")");
  #endif
  _reconnect = true;
  _app = false;
  strcpy(_app_token, user_token);
  strcpy(_activity_id, activity_id);
  _rpcs.registerEvent("on_login_user", new Delegate<void, char*>(this, &Muzzley::onLoginUser));
  #ifdef DEBUG
    Serial.print("#DEBUG#  Connecting to Muzzley server (");
    Serial.print(_server);
    Serial.print(").. ");
  #endif
  _rpcs.connect(_server);
}


void Muzzley::changeWidget(int participant_id, char* widget, char* options){
  _rpcs.changeWidget(participant_id, widget, options);
}

void Muzzley::sendSignal(int participant_id, char* type, char* data, SignalCallback callback){
  if(_ready){
    int msg_type = 5;
    #ifdef DEBUG
      Serial.print("#DEBUG#  Sending to participant ");
      Serial.print(participant_id);
      Serial.print(": ");
      Serial.println(data);
    #endif
    if(callback != NULL){
      UserCallback user_cb;
      sprintf(user_cb.m_cid, "%d",_rpcs.getCurrentCid());
      user_cb.cb = callback;
      if(_waiting_cbs == (sizeof(_stored_cbs)/16)){
        _waiting_cbs = 0;
      }
      _stored_cbs[_waiting_cbs] = user_cb;
      _waiting_cbs++;
      msg_type = 1;
    }
    _rpcs.sendSignal(participant_id, msg_type, type, data);
  }
}

void Muzzley::sendSignal(char* type, char* data, SignalCallback callback){
  if(_ready){
    int msg_type = 5;
    #ifdef DEBUG
      Serial.print("#DEBUG#  Sending App ");
    #endif
    if(callback != NULL){
      UserCallback user_cb;
      sprintf(user_cb.m_cid, "%d",_rpcs.getCurrentCid());
      user_cb.cb = callback;
      if(_waiting_cbs == (sizeof(_stored_cbs)/16)){
        _waiting_cbs = 0;
      }
      _stored_cbs[_waiting_cbs] = user_cb;
      _waiting_cbs++;
      msg_type = 1;
      #ifdef DEBUG
        Serial.print(" (with callback)");
      #endif
    }
    #ifdef DEBUG
      Serial.print(": ");
      Serial.println(data);
    #endif
    _rpcs.sendSignal(NULL, msg_type, type, data);
  }
}

void Muzzley::sendWidgetData(char* data){
  #ifdef DEBUG
    Serial.print("#DEBUG#  Sending Widget data to App: ");
    Serial.println(data);
  #endif 
  _rpcs.sendSignal(NULL, 5, "signal", data);
}


// Events

void Muzzley::onConnect(char* message){
  if(message != NULL){
    #ifdef DEBUG
      Serial.println("[failed]");
    #endif
    reconnect();
  } else {
    _connecting = true;
    #ifdef DEBUG
      Serial.println("[done]");
      Serial.print("#DEBUG#  Handshaking client.. ");
    #endif
    _rpcs.handshake();
  }
}

void Muzzley::onClose(char* msg){
  _connecting = false;
  #ifdef DEBUG
    Serial.print("#DEBUG#  Connection to Muzzley lost (");
    Serial.print(msg);
    Serial.println(")");
  #endif
  _ready = false;
  _participants_count = 0;
  _waiting_cbs = 0;
  if(_on_close != NULL){
    (*_on_close)(msg);
  } else {
    #ifdef DEBUG
      Serial.println("#DEBUG#  Warning: onClose event handler missing (check setOnCloseHandler method)");
    #endif
  }
}

void Muzzley::onActivityTerminated(char* msg){
  #ifdef DEBUG
    Serial.print("#DEBUG#  Activity terminated");
  #endif
  _connecting = true;
  _reconnect = false;
  _rpcs.disconnect("Closing connection..");
  if(_activity_terminated != NULL){
    (*_activity_terminated)();
  } else {
    #ifdef DEBUG
      Serial.println("#DEBUG#  Warning: onActivityTerminated event handler missing (check setActivityTerminatedHandler method)");
    #endif
  }
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
    #ifdef DEBUG
      Serial.println("[done]");
    #endif
    if(hashTable.containsKey("d")){
      JsonHashTable data = hashTable.getHashTable("d");
      if(data.containsKey("deviceId")){
        strcpy(_device_id, data.getString("deviceId"));
      }
    }
    if(_app == true){
      #ifdef DEBUG
        Serial.print("#DEBUG#  Logging in as App.. ");
      #endif
      _rpcs.loginApp(_app_token);
    }else{
      #ifdef DEBUG
        Serial.print("#DEBUG#  Logging in as Participant.. ");
      #endif
      _rpcs.loginUser(_app_token);
    }
  }else{
    #ifdef DEBUG
      Serial.println("[failed]");
    #endif
      reconnect();
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
    #ifdef DEBUG
      Serial.println("[done]");
      Serial.print("#DEBUG#  Creating activity.. ");
    #endif
    _rpcs.createActivity(_static_activity, _activity_id);
  }else{
    #ifdef DEBUG
      Serial.println("[failed]");
    #endif
   reconnect();
  }

}


void Muzzley::onLoginUser(char* message){

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
    #ifdef DEBUG
      Serial.println("[done]");
      Serial.print("#DEBUG#  Joining activity ");
      Serial.print(_activity_id);
      Serial.print(" .. ");
    #endif
    _rpcs.joinActivity(_activity_id);
  }else{
    #ifdef DEBUG
      Serial.println("[failed]");
    #endif
    reconnect();
  }

}

//Users only
void Muzzley::onActivityJoined(char* message){
  _connecting = false;
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);

  if(!hashTable.success()){
    return;
  }
  bool success = hashTable.getBool("s");

  if(success){
    JsonHashTable key_d = hashTable.getHashTable("d");
    JsonHashTable joiner = key_d.getHashTable("participant");

    Participant p;
    p.id = (int)joiner.getLong("id");
    strcpy(p.profileId, joiner.getString("profileId"));
    strcpy(p.name, joiner.getString("name"));
    strcpy(p.photoUrl, joiner.getString("photoUrl"));
    strcpy(p.deviceId, joiner.getString("deviceId"));
    if(joiner.containsKey("context")){
      strcpy(p.context, joiner.getString("context"));
    } else {
      strcpy(p.context, "");
    }
    addParticipant(p);
    _rpcs.sendReadySignal();
  } else {

    if(hashTable.containsKey("d")){
      JsonHashTable key_d = hashTable.getHashTable("d");

      if(key_d.containsKey("connectTo")){
        strcpy(_server,key_d.getString("connectTo"));
        #ifdef DEBUG
          Serial.println("[Requires handover]");
          Serial.print("#DEBUG#  Handing over to ");
          Serial.print(_server);
          Serial.print(" .. ");
        #endif
        _rpcs.connect(_server);
      }else{
        #ifdef DEBUG
          Serial.println("[failed]");
        #endif
        if(hashTable.containsKey("m")){
          char* error = hashTable.getString("m");
          reconnect(error);
        }else{
          reconnect(NULL);
        }
      }
    }else{
      #ifdef DEBUG
        Serial.println("[failed]");
      #endif

      if(hashTable.containsKey("m")){
        char* error = hashTable.getString("m");
        reconnect(error);
      }else{
        reconnect(NULL);
      }
    }
  }
}


void Muzzley::onCreateActivity(char* message){
  _connecting = false;
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  if(!hashTable.success()){
    return;
  }
  if(!hashTable.containsKey("s")){
  }
  char* success = hashTable.getString("s");
  if(strcmp(success, "true") == 0){
    _ready = true;
    JsonHashTable key_d = hashTable.getHashTable("d");
    char* activityId = key_d.getString("activityId");
    char* qrCodeUrl = key_d.getString("qrCodeUrl");
    char* deviceId = key_d.getString("deviceId");
    #ifdef DEBUG
      Serial.println("[done]");
    #endif

    if(_activity_ready != NULL){
      (*_activity_ready)(activityId, qrCodeUrl, deviceId);
    }
  }else{
    if(hashTable.containsKey("d")){
      JsonHashTable key_d = hashTable.getHashTable("d");
      if(key_d.containsKey("connectTo")){
        strcpy(_server,key_d.getString("connectTo"));
        _rpcs.connect(_server);
      }else{
        if(hashTable.containsKey("m")){
          char* error = hashTable.getString("m");
          reconnect(error);
        }else{
          reconnect();
        }
      }
    }else{
      if(hashTable.containsKey("m")){
        char* error = hashTable.getString("m");
        reconnect(error);
      }else{
        reconnect();
      }
    }
  }
}

//App only
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
  if(joiner.containsKey("context")){
    strcpy(p.context, joiner.getString("context"));
  } else {
    strcpy(p.context, "");
  }
  addParticipant(p);
}


void Muzzley::onParticipantReady(char* message){
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  JsonHashTable headers = hashTable.getHashTable("h");
  Participant p;
  if(headers.containsKey("pid")){
    int pid = (int)headers.getLong("pid");
    p = getParticipantById(pid);
  } else {
    p = _participants[_participants_count-1];
  }
  _ready = true;
  #ifdef DEBUG
    Serial.println("[done]");
  #endif

  if(_participant_joined != NULL){
    (*_participant_joined)(p);
  } else {
    #ifdef DEBUG
      Serial.println("#DEBUG#  Warning: onJoin event handler missing (check setParticipantJoinHandler method)");
    #endif
  }
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
    if(_widget_ready != NULL){
      (*_widget_ready)(pid);
    }
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
  if(_on_participant_quit != NULL){
    (*_on_participant_quit)(pid);
  }
}



void Muzzley::onSignalingMessage(char* msg){
  #ifdef DEBUG
    Serial.print("#DEBUG#  Received: ");
    Serial.println(msg);
  #endif
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(msg);
  if(!hashTable.success()){
    return;
  }

  if(hashTable.containsKey("h")){
    JsonHashTable headers = hashTable.getHashTable("h");
    int pid;
    if(headers.containsKey("pid")){
      pid = (int)headers.getLong("pid");
    }else{ pid = NULL; }
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
      char* response;
      if(_app){
        response = (*_on_signaling_message)(pid, message_type, message);
      }else{
        response = (*_on_signaling_message_p)(message_type, message);
      }
      if(request_type == 1){
        #ifdef DEBUG
          Serial.print("#DEBUG#  Replying: ");
          Serial.println(response);
        #endif
        _rpcs.respondToSignal(cid, pid, response);
        return;
      }
    }else if(request_type == 2){
      for(int i = 0; i < _waiting_cbs; ++i){
        if( strcmp(_stored_cbs[i].m_cid, cid) == 0){
          if(_stored_cbs[i].cb != NULL){
            (*_stored_cbs[i].cb)(hashTable.getBool("s"), key_d);
          }
          /**for(int j = i; j < _waiting_cbs; j++){
            _stored_cbs[j] = _stored_cbs[j+1];
            _waiting_cbs--;
          }*/
          return;
        } 
      }
    }
  } else {
    #ifdef DEBUG
      Serial.print("#DEBUG#  The received message is unknown");
      Serial.println(msg);
    #endif
  }
}

void Muzzley::disconnect(){
  _reconnect = false;
  _rpcs.disconnect("Disconnect requested by user");
}