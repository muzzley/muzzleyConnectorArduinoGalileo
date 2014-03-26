#include "RpcManager.h"
#include "WSClient.h"
#include "JsonHashTable.h"
#include "JsonParser.h"


// Private

void RpcManager::handleResponse(char* message){
  if (strcmp(message, "h") == 0) return;
  char msg[500];
  strcpy(msg, message);
  JsonParser<32> parser;
  JsonHashTable hashTable = parser.parseHashTable(message);
  JsonHashTable header = hashTable.getHashTable("h");
  char* type = header.getString("t");
  char* cid = header.getString("cid");
Serial.println(cid);
  if (strcmp(type, "2") == 0){
    if(type == NULL || cid == NULL){
      return;
    }
    for(int i = 0; i < _rpcs_count; ++i){
      if( strcmp(_rpcs[i]._correlation_id,cid) == 0){
        (*_rpcs[i]._callback)(msg);
        removeRpc(i);
      }
    }  
  }else{
    if (strcmp(type, "3") == 0){
      if (strcmp(hashTable.getString("a"), "participantJoined") == 0){
        (*_on_participant_join)(msg);
 Serial.println(cid);
        sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":4},\"s\":true}", cid);
        Serial.println(msg);
        _ws.send(msg);
      }
    }
  }
}

/** 
  //Verify if the muzzData is a response
  if (muzzData.h.t  === MESSAGE_TYPE_RESPONSE){
    if (!muzzData || !muzzData.h || typeof muzzData.h.cid === 'undefined') {
      // No Correlation Id defined, nothing to do here...
      return;
    }

    var correlationId = muzzData.h.cid;

    if (correlationId in requests) {
      var entry = requests[correlationId];
      clearTimeout(entry.timeout);
      delete requests[correlationId];

      //Check if the message is an error and is not a redirect (connectTo)
      var isRedirect = muzzData.d && muzzData.d.connectTo;
      if (muzzData.s === false && !isRedirect) {
        var errMsg = muzzData.m || 'Unknown error';
        return entry.callback(new Error(errMsg), muzzData);
      }
      return entry.callback(null, muzzData);
    }
  }else{
    next(muzzData);
  }*/


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

// Public
RpcManager::RpcManager(){
  _rpcs_count = 0;
  _cid = 1;
  _on_message = new Delegate<void, char*>(this, &RpcManager::handleResponse);
  _ws.addEventListener(_on_message);
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

}


void RpcManager::next(){
  _ws.listen();
}


void RpcManager::handshake(){
  Serial.print("Handshaking...");
  char msg[126];
  sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"handshake\",\"d\":{\"protocolVersion\":\"1.2\",\"lib\":\"Galileo 1.0\"}}",_cid);
  makeRequest(msg, _on_handshake);
}

void RpcManager::loginApp(char *token){
  Serial.print("LoginApp...");
  char msg[126];
  sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"loginApp\",\"d\":{\"token\":\"%s\"}}", _cid, token);
  makeRequest(msg, _on_login_app);
}


void RpcManager::createActivity(bool static_activity, char *activity){
  Serial.print("CreateActivity...");
  char msg[126];
  if(static_activity){
    sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"create\",\"d\":{\"activityId\":\"%s\"}}", _cid, activity);
  }else{
    sprintf(msg, "{\"h\":{\"cid\":\"%d\",\"t\":1},\"a\":\"create\"}", _cid);
  }
  makeRequest(msg, _on_create_activity);
}



void RpcManager::makeRequest(char *msg, Delegate<void, char*> *d){
  
  if(d != NULL){
    char correlation_id[11];
    sprintf(correlation_id, "%d",_cid);
    Rpc rpc;
    strcpy(rpc._correlation_id, correlation_id);
    rpc._callback = d;
    addRpc(rpc);
  }

  ++_cid;
  Serial.println(msg);
  _ws.send(msg);
}


void RpcManager::connect(char* server){
  Serial.print("Connecting...");
  _ws.connect(server, 80, "/ws");
  if(_ws.connected() == true){
    Serial.print("[Done]\n");
    (*_on_connect)("");
  }else{
    Serial.print("[Failed]\n");
    delay(1000);
    connect(server);
  }
}

