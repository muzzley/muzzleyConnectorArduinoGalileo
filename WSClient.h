#ifndef WSCLIENT_H_
#define WSCLIENT_H_

#include "Callback.h"
#include <Ethernet.h>

typedef enum {CLOSED, CONNECTING, OPEN} Status;

class WSClient{
  public:
    void connect(const char host[], const int port = 80, const char path[] = "/");
    void reconnect();
    bool connected();
    void disconnect();
    void addEventListener(Delegate<void, char*> *d);
    void send(char data[]);
    void listen();
  private:
    void readLine(char* buffer);
    bool handshake(const char host[], const int port, const char path[]);
    size_t base64Encode(byte* src, size_t srclength, char* target, size_t targetsize);
    byte getNextByte();
    Delegate<void, char*> *_on_message_delegate;
    EthernetClient _socket;
    Status _ready_status;
    char* _packet;
    unsigned int _packetLength;
    byte _opCode;
    const char* _host;
    int _port;
    const char* _path;
};

const char b64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
#endif
