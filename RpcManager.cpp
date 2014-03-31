#include "RpcManager.h"
#include "WSClient.h"
#include "JsonHashTable.h"
#include "JsonParser.h"

void RpcManager::handleResponse(char* message){
  if (strcmp(message, "h") == 0) return;
  char msg[strlen(message)];
  strcpy(msg, message);
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  JsonHashTable header = hashTable.getHashTable("h");
  char* type = header.getString("t");
  char* cid = header.getString("cid");
  char* action = NULL;
  JsonHashTable user_data;
  char* user_action;
  if(hashTable.containsKey("a")){
    action = hashTable.getString("a");
  }
  switch(type[0]){
    case '1':
      user_data = hashTable.getHashTable("d");
      user_action = user_data.getString("a");
      if (strcmp(user_action, "ready") == 0){
        (*_on_participant_ready)(msg);
      }else{
        (*_on_signaling_message)(msg);
      }
    break;

    case '2':
      if(type == NULL || cid == NULL){
        return;
      }
      for(int i = 0; i < _rpcs_count; ++i){
        if( strcmp(_rpcs[i]._correlation_id,cid) == 0){
          (*_rpcs[i]._callback)(msg);
          removeRpc(i);
          return;
        } 
      }
      (*_on_signaling_message)(msg);
    break;

    case '3':
      if (strcmp(action, "participantJoined") == 0){
        (*_on_participant_join)(msg);
        sprintf(msg, "{\"h\":{\"cid\":\"%s\",\"t\":4},\"s\":true}", cid);
        _ws.send(msg);
      }else if(strcmp(action, "participantQuit") == 0){
        (*_on_participant_quit)(msg);
        sprintf(msg, "{\"h\":{\"t\":4,\"cid\": \"%s\"},\"s\":true}", cid);
        _ws.send(msg);
      }
    break;

    case '5':
      if (strcmp(action, "signal") == 0){
        (*_on_signaling_message)(msg);
      }
    break;

    default:
    break;
  };
}

void RpcManager::handleCloseEvent(char* msg){
  if(_on_close != NULL)(*_on_close)(msg);
}


void RpcManager::removeRpc(int pos){
  for(int i = pos; i < _rpcs_count; i++){
    _rpcs[i] = _rpcs[i+1];
    _rpcs_count--;
  }
}



void RpcManager::addRpc(Rpc rpc){
  _rpcs[_rpcs_count] = rpc;
  _rpcs_count++;
}

RpcManager::RpcManager(){
  _rpcs_count = 0;
  _cid = 1;
  _on_message = new Delegate<void, char*>(this, &RpcManager::handleResponse);
  _ws.addEventListener("on_message", _on_message);
}

int RpcManager::getCurrentCid(){
  return _cid;
}

void RpcManager::registerEvent(char* type, Delegate<void, char*> *d){
  if (strcmp(type, "on_connect") == 0) {
    _on_connect = d;
  }
  if (strcmp(type, "on_handshake") == 0) {
    _on_handshake = d;
  }
  if (strcmp(type, "on_login_app") == 0) {
    _on_login_app = d;
  }
  if (strcmp(type, "on_create_activity") == 0) {
    _on_create_activity = d;
  }
  if (strcmp(type, "on_participant_join") == 0) {
    _on_participant_join = d;
  }
  if (strcmp(type, "on_participant_ready") == 0) {
    _on_participant_ready = d;
  }
  if (strcmp(type, "on_widget_ready") == 0) {
    _on_widget_ready = d;
  }
  if (strcmp(type, "on_signaling_message") == 0) {
    _on_signaling_message = d;
  }
  if (strcmp(type, "on_participant_quit") == 0) {
    _on_participant_quit = d;
  }
  if (strcmp(type, "on_close") == 0) {
    _on_close = d;
  }
}

void RpcManager::next(){
  _ws.getNextPacket();
}

void RpcManager::handshake(){
  char msg[92];
  sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"handshake\",\"d\":{\"protocolVersion\":\"1.2\",\"lib\":\"Galileo 1.0\"}}",_cid);
  makeRequest(msg, _on_handshake);
}

void RpcManager::loginApp(char *token){
  char msg[120];
  sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"loginApp\",\"d\":{\"token\":\"%s\"}}", _cid, token);
  makeRequest(msg, _on_login_app);
}


void RpcManager::createActivity(bool static_activity, char *activity){
  char msg[50];
  if(static_activity){
    sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"create\",\"d\":{\"activityId\":\"%s\"}}", _cid, activity);
  }else{
    sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"create\"}", _cid);
  }
  makeRequest(msg, _on_create_activity);
}


void RpcManager::changeWidget(int pid, char* widget, char* options){
  char msg[300];
  sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"pid\":%d,\"t\":1},\"a\":\"signal\",\"d\":{\"a\":\"changeWidget\",\"d\":{\"widget\":\"%s\"", _cid, pid, widget);
  if(options != NULL){
    strcat(msg,",\"params\":");
    strcat(msg, options);
  }
    strcat(msg,"}}}");
  makeRequest(msg, _on_widget_ready);
}

void RpcManager::sendSignal(int pid, int msg_type, char* type, char* data){
  char msg[400];
  sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"pid\":%d,\"t\":%d},\"a\":\"signal\",\"d\":{\"a\":\"%s\",\"d\":%s}}", _cid, pid, msg_type, type, data);
  makeRequest(msg);
}

void RpcManager::respondToSignal(char* cid, int pid, char* response){
  char msg[300];
  sprintf(msg, "{\"h\":{\"cid\":\"%s\",\"pid\":%d,\"t\":2},%s}", cid, pid, response);
  _ws.send(msg);
}

void RpcManager::makeRequest(char *msg, Delegate<void, char*> *d){
  
  if(d != NULL){
    char correlation_id[11];
    sprintf(correlation_id, "%d",_cid);
    Rpc rpc;
    strcpy(rpc._correlation_id, correlation_id);
    rpc._callback = d;
    rpc._timer = millis();
    addRpc(rpc);
  }

  ++_cid;
  _ws.send(msg);
}


void RpcManager::connect(char* server){
  _ws.connect(server, 80, "/ws");
  if(_ws.connected() == true){
    (*_on_connect)(NULL);
  }else{
    delay(1000);
    connect(server);
  }
}


void RpcManager::removeExpiredTimeouts(){
  for(int i = 0; i < _rpcs_count; ++i){
    if(millis() - _rpcs[i]._timer >= 15000){
      removeRpc(i);
    }
  }
}
