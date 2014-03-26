#ifndef RPC_MANAGER_H_
#define RPC_MANAGER_H_

#include "Callback.h"
#include "WSClient.h"
#include "JsonHashTable.h"

struct Rpc{
  char _correlation_id[11];
  int _timer = 15;
  Delegate<void, char*> *_callback;
};

class RpcManager{

  private:
    void handleResponse(char* message);
    void addRpc(Rpc rpc);
    void removeRpc(int pos);
    Rpc getRpcByCid(char* cid);
    Rpc _rpcs[15];
    int _rpcs_count;
    int _cid;
    WSClient _ws;
    Delegate<void, char*> *_on_message;
    Delegate<void, char*> *_on_handshake;
    Delegate<void, char*> *_on_login_app;
    Delegate<void, char*> *_on_create_activity;
    Delegate<void, char*> *_on_connect_to;
    Delegate<void, char*> *_on_participant_join;
    Delegate<void, char*> *_on_participant_ready;
    Delegate<void, char*> *_on_participant_quit;
    Delegate<void, char*> *_on_signaling_message;
    Delegate<void, char*> *_on_action;
    Delegate<void, char*> *_on_change_widget;
    Delegate<void, char*> *_on_disconnect;
    Delegate<void, char*> *_on_connect;

  public:

    RpcManager();
    void next();
    void makeRequest(char* message, Delegate<void, char*> *d = NULL);
    void removeExpiredTimeouts();
    void registerEvent(char* type, Delegate<void, char*> *d);
    void connect(char* server);
    void disconnect();
    void handshake();
    void loginApp(char* token);
    void createActivity(bool static_activity, char* activity);
};


#endif
