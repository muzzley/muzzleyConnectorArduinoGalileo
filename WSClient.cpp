#include <WSClient.h>

char handshake_line1a[] PROGMEM = "GET ";
char handshake_line1b[] PROGMEM = " HTTP/1.1";
char handshake_line2[] PROGMEM = "Upgrade: websocket";
char handshake_line3[] PROGMEM = "Connection: Upgrade";
char handshake_line4[] PROGMEM = "Host: ";
char handshake_line5[] PROGMEM = "Origin: GalileoWSClient";
char handshake_line6[] PROGMEM = "Sec-WebSocket-Version: 13";
char handshake_line7[] PROGMEM = "Sec-WebSocket-Key: ";
char handshake_success_response[] PROGMEM = "HTTP/1.1 101";

PROGMEM const char *WSClientStringTable[] =
{
  handshake_line1a,
  handshake_line1b,
  handshake_line2,
  handshake_line3,
  handshake_line4,
  handshake_line5,
  handshake_line6,
  handshake_line7,
  handshake_success_response
};

void getStringTableItem(char* buffer, int index) {
  strcpy(buffer, WSClientStringTable[index]);
}

void WSClient::readLine(char* buffer) {
  char character;
  int i = 0;
  while(_socket.available() > 0 && (character = _socket.read()) != '\n') {
    if(character != '\r' && character != -1) {
      buffer[i++] = character;
    }
  }
  buffer[i] = 0x0;
}

bool WSClient::handshake(const char host[], const int port, const char path[]){

  char buffer[45];
  getStringTableItem(buffer, 0);
  _socket.print(buffer);
  _socket.print(path);
  getStringTableItem(buffer, 1);
  _socket.println(buffer);
  getStringTableItem(buffer, 2);
  _socket.println(buffer);
  getStringTableItem(buffer, 3);
  _socket.println(buffer);
  getStringTableItem(buffer, 4);
  _socket.print(buffer);
  _socket.println(host);
  getStringTableItem(buffer, 5);
  _socket.println(buffer);
  getStringTableItem(buffer, 6);
  _socket.println(buffer);
  
  byte bytes[16];
  for(int i = 0; i < 16; i++) {
    bytes[i] = 255 * random();
  } 
  getStringTableItem(buffer, 7);
  _socket.print(buffer);
   base64Encode(bytes, 16, buffer, 45);
  _socket.println(buffer);
  _socket.println("");

  bool result = false;
  int maxAttempts = 300, attempts = 0;
  char line[128];
  char response[12];
  getStringTableItem(response, 8);

  while(_socket.available() == 0 && attempts < maxAttempts) {
    delay(100);
    attempts++;
  }

  while(true) {
    readLine(line);
    if(strcmp(line, "") == 0) { break; }
    if(strncmp(line, response, 12) == 0) { result = true; }
  }
  return result;
}

byte WSClient::getNext() {
  while(_socket.available() == 0);
  byte b = _socket.read();
  return b;
}

void WSClient::connect(const char host[], const int port, const char path[]){
  if(_socket.connected()){
    disconnect();
  }
  _host = host;
  _port = port;
  _path = path;
  if (_socket.connect(host, port)){
    if(!handshake(host, port, path)){
      disconnect();
    }
  }
}

void WSClient::reconnect(){
  if(_socket.connected()){
    disconnect();
  }
  connect(_host, _port, _path);
}

void WSClient::disconnect(){
  _socket.write((uint8_t) 0x87);
  _socket.write((uint8_t) 0x00);
  _socket.flush();
  _socket.stop();
  if(_on_close_cb != NULL)(*_on_close_cb)(NULL);
}

bool WSClient::connected(){
  return _socket.connected();
}

void WSClient::send(char* data){
  int len = strlen(data);
  _socket.write(0x81);
  if(len > 125) {
    _socket.write(0xFE);
    _socket.write(byte(len >> 8));
    _socket.write(byte(len & 0xFF));
  } else {
    _socket.write(0x80 | byte(len));
  }
  for(int i = 0; i < 4; i++) {
    _socket.write((byte)0x00);
  }
  _socket.print(data);
}

void WSClient::addEventListener(char* event_type, Delegate<void, char*> *callback){
  if(strcmp(event_type, "on_message")==0){
    _on_message_cb = callback;
  } else if(strcmp(event_type, "close")==0){
    _on_close_cb = callback;
  }
}

void WSClient::getNextPacket() {
  if(!connected()){
    reconnect();
  }

  if(_socket.available() > 0) {
    byte hdr = getNext();
    bool fin = hdr & 0x80;
    int opCode = hdr & 0x0F;
    hdr = getNext();
    bool mask = hdr & 0x80;
    int len = hdr & 0x7F;
    if(len == 126) {
      len = getNext();
      len <<= 8;
      len += getNext();
    } else if (len == 127) {
      len = getNext();
      for(int i = 0; i < 7; i++) {
        len <<= 8;
        len += getNext();
      }
    }

    if(mask) {
      for(int i = 0; i < 4; i++) { getNext(); }
    }

    if(mask) {
      free(_packet);
      return;
    }

    if(!fin) {
      if(_packet == NULL) {
        _packet = (char*) malloc(len);
        for(int i = 0; i < len; i++) { _packet[i] = getNext(); }
        _packetLength = len;
        _opCode = opCode;
      } else {
        int copyLen = _packetLength;
        _packetLength += len;
        char *temp = _packet;
        _packet = (char*)malloc(_packetLength);
        for(int i = 0; i < _packetLength; i++) {
          if(i < copyLen) {
            _packet[i] = temp[i];
          } else {
            _packet[i] = getNext();
          }
        }
        free(temp);
      }
      return;
    }

    if(_packet == NULL) {
      _packet = (char*) malloc(len + 1);
      for(int i = 0; i < len; i++) { _packet[i] = getNext(); }
      _packet[len] = 0x0;
    } else {
      int copyLen = _packetLength;
      _packetLength += len;
      char *temp = _packet;
      _packet = (char*) malloc(_packetLength + 1);
      for(int i = 0; i < _packetLength; i++) {
        if(i < copyLen) {
          _packet[i] = temp[i];
        } else {
          _packet[i] = getNext();
        }
      }
      _packet[_packetLength] = 0x0;
      free(temp);
    }
    
    if(opCode == 0 && _opCode > 0) {
      opCode = _opCode;
      _opCode = 0;
    }

    switch(opCode) {
      case 0x01:
        if (_on_message_cb != NULL) {
          (*_on_message_cb)(_packet);
        }
        break;
        
      case 0x09:
        _socket.write(0x8A);
        _socket.write(byte(0x00));
        break;
        
      case 0x08:
        unsigned int code = ((byte)_packet[0] << 8) + (byte)_packet[1];
        disconnect();
        break;
    }
    free(_packet);
    _packet = NULL;
  }
}

size_t WSClient::base64Encode(byte* src, size_t srclength, char* target, size_t targsize) {
  size_t datalength = 0;
  char input[3];
  char output[4];
  size_t i;

  while (2 < srclength) {
    input[0] = *src++;
    input[1] = *src++;
    input[2] = *src++;
    srclength -= 3;

    output[0] = input[0] >> 2;
    output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
    output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
    output[3] = input[2] & 0x3f;

    if(datalength + 4 > targsize) { return (-1); }

    target[datalength++] = b64Alphabet[output[0]];
    target[datalength++] = b64Alphabet[output[1]];
    target[datalength++] = b64Alphabet[output[2]];
    target[datalength++] = b64Alphabet[output[3]];
  }

  if(0 != srclength) {
    input[0] = input[1] = input[2] = '\0';
    for (i = 0; i < srclength; i++) { input[i] = *src++; }

    output[0] = input[0] >> 2;
    output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
    output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

    if(datalength + 4 > targsize) { return (-1); }

    target[datalength++] = b64Alphabet[output[0]];
    target[datalength++] = b64Alphabet[output[1]];
    if(srclength == 1) {
      target[datalength++] = '=';
    } else {
      target[datalength++] = b64Alphabet[output[2]];
    }
    target[datalength++] = '=';
  }
  if(datalength >= targsize) { return (-1); }
  target[datalength] = '\0';
  return (datalength);
}
