#ifndef MUZZLEY_H_
#define MUZZLEY_H_

#include "WSClient.h"
#include "RpcManager.h"

struct Participant{
  long id;
  char profileId[36];
  char name[40];
  char photoUrl[60];
  char deviceId[40];
};


class Muzzley{

  public:
    Muzzley();
    void connectApp(char *app_token, char *activity_id = NULL);
    void disconnect();
    bool connected();
    void createActivity();
    void nextTick();
    typedef void (*ActivityReady)(char* activityId, char* qrCodeUrl, char* deviceId);
    typedef void (*ParticipantJoined)(Participant p);
    void setActivityReadyHandler(ActivityReady activity_ready);
    void setParticipantJoinHandler(ParticipantJoined participant_joined);


  private:
    void read();
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
    
};
#endif
