#ifndef MUZZLEY_H_
#define MUZZLEY_H_

#include "WSClient.h"
#include "RpcManager.h"
#include "JsonHashTable.h"

struct Participant{
  int id;
  char profileId[36];
  char name[40];
  char photoUrl[60];
  char deviceId[40];
};

typedef void (*SignalCallback)(bool success, JsonHashTable myObj);

struct UserCallback{
  char m_cid[11];
  SignalCallback cb;
};

class Muzzley{

  public:
    Muzzley();
    void connectApp(char *app_token, char *activity_id = NULL);
    void disconnect();
    bool connected();
    void createActivity();
    void changeWidget(int participant_id, char* widget, char* options);
    void sendSignal(int participant_id, char* type, char* data, SignalCallback callback);
    void nextTick();
    typedef void (*ActivityReady)(char* activityId, char* qrCodeUrl, char* deviceId);
    typedef void (*ParticipantJoined)(Participant p);
    typedef void (*ParticipantQuit)(int participant_id);
    typedef void (*WidgetReady)(int participant_id);
    typedef char* (*OnSignalingMessage)(int participant_id, char* type, JsonHashTable message);
    typedef void (*OnClose)();
    void setActivityReadyHandler(ActivityReady activity_ready);
    void setParticipantJoinHandler(ParticipantJoined participant_joined);
    void setParticipantQuitHandler(ParticipantQuit participant_quit);
    void setParticipantWidgetChanged(WidgetReady widget_ready);
    void setSignalingMessagesHandler (OnSignalingMessage on_signaling_message);
    void setOnCloseHandler(OnClose on_close);
    void onClose(char* msg);

  private:
    void addParticipant(Participant p);
    Participant getParticipantById(int id);
    void removeParticipantById(int id);
    void handshake();
    void onConnect(char* msg);
    void onHandshake(char* msg);
    void onLoginApp(char* msg);
    void onCreateActivity(char* msg);
    void onParticipantJoin(char* msg);
    void onParticipantReady(char* msg);
    void onParticipantQuit(char* msg);
    void onWidgetReady(char* msg);
    void onSignalingMessage(char* msg);
    RpcManager _rpcs;
    int _participants_count;
    Participant _participants[30];
    char _device_id[40]; 
    char _app_token[30];
    char _activity_id[20];
    char _server[40];
    bool _static_activity;
    ActivityReady _activity_ready;
    ParticipantJoined _participant_joined;
    ParticipantQuit _on_participant_quit;
    WidgetReady _widget_ready;
    OnSignalingMessage _on_signaling_message;
    OnClose _on_close;
    UserCallback _stored_cbs[10];
    int _waiting_cbs;
};
#endif
