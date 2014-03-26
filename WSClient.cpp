#include <WSClient.h>

// Private methods


//Handshake
/** Read single line  */
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


/** Handshake Process */
bool WSClient::handshake(const char host[], const int port, const char path[]){
  char key[45]; 
  byte bytes[16];
  
  for(int i = 0; i < 16; i++) {
    bytes[i] = 255 * random();
  }
  base64Encode(bytes, 16, key, 45);

  _socket.print("GET ");
  _socket.print(path);
  _socket.print(" HTTP/1.1");
  _socket.println("");
  _socket.print("Host: ");
  _socket.print(host);
  _socket.print(":");
  _socket.print(port);
  _socket.println("");
  _socket.println("Upgrade: websocket");
  _socket.println("Connection: Upgrade");
  _socket.println("Origin: Galileo");
  _socket.print("Sec-WebSocket-Key: ");
  _socket.print(key);
  _socket.println("");
  _socket.println("Sec-WebSocket-Version: 13");
  _socket.println("");

  bool result = false;
  int maxAttempts = 300, attempts = 0;
  char line[128];
  char response[] = "HTTP/1.1 101";

  while(_socket.available() == 0 && attempts < maxAttempts) {
    delay(100);
    attempts++;
  }

  while(true) {
    readLine(line);
    if(strcmp(line, "") == 0) { break; }
    if(strncmp(line, response, 12) == 0) { result = true; }
  }
  if(!result) {
    Serial.println("Handshake Failed! Terminating");
    _socket.stop();
  }
  return result;
}


/** Read the next byte */
byte WSClient::getNextByte() {
  while(_socket.available() == 0);
  byte b = _socket.read();
  return b;
}



// Public methods

/**  Connects and handshakes with remote server */
void WSClient::connect(const char host[], const int port, const char path[]){
  _ready_status = CLOSED;
  if(_socket.connected()){
    disconnect();
  }
  _ready_status = CONNECTING;
  _host = host;
  _port = port;
  _path = path;
  if (_socket.connect(host, port)){
    if(handshake(host, port, path)){
      _ready_status = OPEN;
    }else{
      disconnect();
    }
  }
}

void WSClient::reconnect(){
  connect(_host, _port, _path);
}



/** Gracefully disconnect from server */
void WSClient::disconnect(){
  _socket.write((uint8_t) 0x87);
  _socket.write((uint8_t) 0x00);
  _socket.flush();
  delay(20);
  _socket.stop();
  _ready_status = CLOSED;
}

/** Checks if the connection is open  */
bool WSClient::connected(){
  bool connected = false;
  if(_ready_status == OPEN) connected = true;
  return connected;
}

/** Sends a message to the websocket server */
void WSClient::send(char data[]){   
  int size = strlen(data);
  // string type
  _socket.write(0x81);

  // NOTE: no support for > 16-bit sized messages
  if (size > 125){
    _socket.write(127);
    _socket.write((uint8_t) (size >> 56) & 255);
    _socket.write((uint8_t) (size >> 48) & 255);
    _socket.write((uint8_t) (size >> 40) & 255);
    _socket.write((uint8_t) (size >> 32) & 255);
    _socket.write((uint8_t) (size >> 24) & 255);
    _socket.write((uint8_t) (size >> 16) & 255);
    _socket.write((uint8_t) (size >> 8) & 255);
    _socket.write((uint8_t) (size ) & 255);
  } else {
    _socket.write((uint8_t) size);
  }

  for (int i=0; i<size; ++i){
    _socket.write(data[i]);
  }
}

/** Set the function to handle the received messages  */
void WSClient::addEventListener(Delegate<void, char*> *d){
  _on_message_delegate = d;
}


/** listen to incoming messages on the websocket  */
void WSClient::listen() {
/**
  if(!connected() && millis() > _retryTimeout) {
    _retryTimeout = millis() + RETRY_TIMEOUT;
    _reconnecting = true;
    reconnect();
    _reconnecting = false;
    return;
  }
*/

  if(!connected()){
    reconnect();
  }

  if(_socket.available() > 0) {
    byte hdr = getNextByte();
    bool fin = hdr & 0x80;
    Serial.print("WebSocketClient::monitor() - fin="); Serial.println(fin);
    int opCode = hdr & 0x0F;
    Serial.print("WebSocketClient::monitor() - op="); Serial.println(opCode);

    hdr = getNextByte();
    bool mask = hdr & 0x80;
    int len = hdr & 0x7F;
    if(len == 126) {
      len = getNextByte();
      len <<= 8;
      len += getNextByte();
    } else if (len == 127) {
      len = getNextByte();
      for(int i = 0; i < 7; i++) {
        len <<= 8;
        len += getNextByte();
      }
    }

    Serial.print("WebSocketClient::monitor() - len="); Serial.println(len);

    if(mask) { // skipping 4 bytes for now.
      for(int i = 0; i < 4; i++) { getNextByte(); }
    }

    if(mask) {
      Serial.println("DEBUG: Masking not yet supported (RFC 6455 section 5.3)");
      free(_packet);
      return;
    }

    if(!fin) {
      if(_packet == NULL) {
        _packet = (char*) malloc(len);
        for(int i = 0; i < len; i++) { _packet[i] = getNextByte(); }
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
            _packet[i] = getNextByte();
          }
        }
        free(temp);
      }
      return;
    }

    if(_packet == NULL) {
      _packet = (char*) malloc(len + 1);
      for(int i = 0; i < len; i++) { _packet[i] = getNextByte(); }
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
          _packet[i] = getNextByte();
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
      case 0x00:
        Serial.println("DEBUG: Unexpected Continuation OpCode");
        break;
        
      case 0x01:
        Serial.println("DEBUG: onMessage");
        Serial.println(_packet);
        if (_on_message_delegate != NULL) {
          (*_on_message_delegate)(_packet);
        }
        break;
        
      case 0x02:
        Serial.println("DEBUG: Binary messages not yet supported (RFC 6455 section 5.6)");
        break;
        
      case 0x09:
        Serial.println("DEBUG: onPing");
        _socket.write(0x8A);
        _socket.write(byte(0x00));
        break;
        
      case 0x0A:
        Serial.println("DEBUG: onPong");
        break;
        
      case 0x08:
        unsigned int code = ((byte)_packet[0] << 8) + (byte)_packet[1];
        Serial.println("DEBUG: onClose");
        disconnect();
        break;
    }

    free(_packet);
    _packet = NULL;
  }
}

/** Base64 encode */
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

  // Padding
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
