#ifndef RPC_MANAGER_H_
#define RPC_MANAGER_H_

#include "Callback.h"
#include "WSClient.h"
#include "JsonHashTable.h"

struct Rpc{
  char _correlation_id[11];
  unsigned long _timer;
  Delegate<void, char*> *_callback;
};

class RpcManager{

  private:
    void handleResponse(char* message);
    void handleCloseEvent(char* msg);
    void addRpc(Rpc rpc);
    void removeRpc(int pos);
    Rpc getRpcByCid(char* cid);
    Rpc _rpcs[30];
    char _server[45];
    int _rpcs_count;
    int _cid;
    bool _need_reconnection;
    bool _forced_disconnection;
    WSClient _ws;
    unsigned long _last_heartbeat;
    Delegate<void, char*> *_on_message;
    Delegate<void, char*> *_on_close;
    Delegate<void, char*> *_on_handshake;
    Delegate<void, char*> *_on_login_app;
    Delegate<void, char*> *_on_login_user;
    Delegate<void, char*> *_on_create_activity;
    Delegate<void, char*> *_on_join_activity;
    Delegate<void, char*> *_on_connect_to;
    Delegate<void, char*> *_on_activity_terminated;
    Delegate<void, char*> *_on_participant_join;
    Delegate<void, char*> *_on_participant_ready;
    Delegate<void, char*> *_on_participant_quit;
    Delegate<void, char*> *_on_signaling_message;
    Delegate<void, char*> *_on_action;
    Delegate<void, char*> *_on_widget_ready;
    Delegate<void, char*> *_on_disconnect;
    Delegate<void, char*> *_on_connect;

  public:

    RpcManager();
    void next();
    int getCurrentCid();
    bool isConnected();
    bool isConnectionIdle();
    void reconnect();
    void makeRequest(char* message, Delegate<void, char*> *d = NULL);
    void removeExpiredTimeouts();
    void registerEvent(char* type, Delegate<void, char*> *d);
    void connect(char* server = NULL);
    void disconnect(char* msg);
    void handshake();
    void loginApp(char* token);
    void loginUser(char* token);
    void createActivity(bool static_activity, char* activity);
    void joinActivity(char* activity);
    void changeWidget(int pid, char* widget, char* options);
    void sendSignal(int pid, int msg_type, char* type, char* data);
    void respondToSignal(char* cid, int pid, char* response);
    void sendReadySignal();
};


#endif
