#include "RpcManager.h"
#include "WSClient.h"
#include "JsonHashTable.h"
#include "JsonParser.h"

void RpcManager::handleResponse(char* message){
  _last_heartbeat = micros();
  if (strcmp(message, "h") == 0){
    return;
  }
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
      }else if(strcmp(action, "activityTerminated") == 0){
        sprintf(msg, "{\"h\":{\"t\":4,\"cid\": \"%s\"},\"s\":true}", cid);
        _ws.send(msg);
        (*_on_activity_terminated)("Activity terminated");
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


bool RpcManager::isConnectionIdle(){
  unsigned long now = micros();
  
  if(((now < _last_heartbeat) && (4294967295-_last_heartbeat+now) >= 60000000) || ((now > _last_heartbeat) && (now-_last_heartbeat) >= 60000000)){
    return true;
  }
  return false;
}

bool RpcManager::isConnected(){
  return _ws.connected();
}


void RpcManager::reconnect(){
  disconnect("");
  delay(5000);
  connect(_server);
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
  _last_heartbeat = micros();
  _need_reconnection = true;
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
  if (strcmp(type, "on_login_user") == 0) {
    _on_login_user = d;
  }
  if (strcmp(type, "on_create_activity") == 0) {
    _on_create_activity = d;
  }
  if (strcmp(type, "on_activity_joined") == 0) {
    _on_join_activity = d;
  }
  if (strcmp(type, "on_activity_terminated") == 0) {
    _on_activity_terminated = d;
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
  sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"handshake\",\"d\":{\"protocolVersion\":\"1.2\",\"lib\":\"Galileo 2.1\"}}",_cid);
  makeRequest(msg, _on_handshake);
}

void RpcManager::loginApp(char *token){
  char msg[120];
  sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"loginApp\",\"d\":{\"token\":\"%s\"}}", _cid, token);
  makeRequest(msg, _on_login_app);
}

void RpcManager::loginUser(char *token){
  char msg[120];
  sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"loginUser\",\"d\":{\"token\":\"%s\"}}", _cid, token);
  makeRequest(msg, _on_login_user);
}

void RpcManager::createActivity(bool static_activity, char *activity){
  char msg[100];
  if(static_activity != NULL){
    sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"create\",\"d\":{\"activityId\":\"%s\"}}", _cid, activity);
  }else{
    sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"create\"}", _cid);
  }
  makeRequest(msg, _on_create_activity);
}


void RpcManager::joinActivity(char *activity){
  char msg[100];
  sprintf(msg, "{\"h\":{\"cid\": \"%d\", \"t\": 1},\"a\": \"join\",\"d\":{\"activityId\": \"%s\"}}", _cid, activity);
  makeRequest(msg, _on_join_activity);
}


void RpcManager::sendReadySignal(){
  char msg[100];
  sprintf(msg, "{\"h\":{\"cid\": \"%d\", \"t\": 1},\"a\": \"signal\",\"d\":{\"a\": \"ready\"}}", _cid);
  makeRequest(msg, _on_participant_ready);
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
  if(!_need_reconnection){
    char msg[400];
    char cont[360];
    char pid_hr[10];
    sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"", _cid);
    if(pid > 0 && pid != NULL){
      sprintf(pid_hr, "pid\":%d,", pid);
      strcat(msg, pid_hr);
    }
    sprintf(cont, "t\":%d},\"a\":\"signal\",\"d\":{\"a\":\"%s\",\"d\":%s}}", msg_type, type, data);
    strcat(msg, cont);
    makeRequest(msg);
  }
}


void RpcManager::respondToSignal(char* cid, int pid, char* response){
  if(!_need_reconnection){
    char msg[300];
    char pid_hdr[20];
    sprintf(msg, "{\"h\":{\"cid\":\"%s\"", cid);
    if(pid > 0 && pid != NULL){
      sprintf(pid_hdr, "\"pid\":%d", pid);
      strcat(msg, pid_hdr);
    }
    strcat(msg, ",\"t\":2},");
    strcat(msg, response);
    strcat(msg, "}");
    _ws.send(msg);
  }
}

void RpcManager::makeRequest(char *msg, Delegate<void, char*> *d){
  
  if(d != NULL){
    char correlation_id[11];
    sprintf(correlation_id, "%d",_cid);
    Rpc rpc;
    strcpy(rpc._correlation_id, correlation_id);
    rpc._callback = d;
    rpc._timer = micros();
    addRpc(rpc);
  }
  ++_cid;
  _ws.send(msg);
}


void RpcManager::connect(char* server){
  if(server != NULL){
    strcpy(_server, server);
  }
  _ws.connect(_server, 80, "/ws");
  if(_ws.connected()){
    _need_reconnection = false;
    (*_on_connect)(NULL);
  }else{
    _need_reconnection = true;
    (*_on_connect)("FAILED");
  }
}

void RpcManager::disconnect(char* msg = NULL){
  if(_ws.connected()){
    _ws.disconnect();
    (*_on_close)(msg);
  }
}


void RpcManager::removeExpiredTimeouts(){
  unsigned long now = micros();
  for(int i = 0; i < _rpcs_count; ++i){
    if(((now < _rpcs[i]._timer) && (4294967295-_rpcs[i]._timer+now) >= 15000000) || ((now > _rpcs[i]._timer) && (now-_rpcs[i]._timer) >= 15000000)){
      removeRpc(i);
    }
  }
}