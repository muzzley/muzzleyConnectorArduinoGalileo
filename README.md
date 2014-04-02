muzzleyConnectorArduinoGalileo
==============================

A library to connect [Intel® Galileo Arduino](http://arduino.cc/en/ArduinoCertified/IntelGalileo) boards to the cloud through Muzzley.

For a more detailed documentation, check [Muzzley documentation](http://www.muzzley.com/documentation).


## How to get it

Clone the lib from this git repository. If you prefer you can also just download it.
Beware if you are downloading the zip file, it will automatically append '-master' to the filename. In order to be able to include the lib in the Arduino IDE you'll have to remove the '-'.

## How to install it

You'll need to use the [Galileo Arduino IDE](https://communities.intel.com/community/makers/drivers), which is available for Windows, Linux and Mac.
In your IDE you can simply import the library (sketch > import library > add library) by adding the library folder or the zip file. Neither the lib folder neither the zip file must contain dashes or an error will be shown.
It is also possible to include the library by git cloning the library or unziping the zip folder into the Arduino IDE libraries folder.

Also keep your board firmware updated.

## Dependencies

This lib uses the following libs:

* [IntelGalileoWebsocketClient](https://github.com/v0od0oChild/IntelGalileoWebsocketClient)
* [ArduinoJsonParser](https://github.com/bblanchon/ArduinoJsonParser)

They come already included in the library package.

## API Documentation

The muzzley Arduino Galileo library offers a way for arduino developers to integrate their applications with the muzzley platform, bringing the arduino interaction to a whole new level.

The application is the endpoint that initiates an activity. The users connect to an activity and control the application. Per activity there's always a single application and zero or more users.

Check the quick start below for a complete example.

### Including the lib in your project

In the top of your sketch file you'll need to include the Muzzley.h file `#incude "Muzzley.h"`
and after creating a new Muzzley object `Muzzley muzzley;`
you are ready to start exploring the available methods.


### Available Methods

#### connectApp (void)

It is the method responsible for connecting your app to the Muzzley servers.
Requires an application token which you can create at the Muzzley apps page. If you are using a static activity you can also set it here.

void Muzzley::connectApp(char \*app_token, char \*activity_id)

###### Parameters

 * `app_token`: Muzzley application token.
 * `activity_id`: (optional) If you are using a static activity.


#### changeWidget (void)

Requests a participant to change his mobile device to a specific one.
For more detail in the available options please check [Muzzley widget documentation](http://www.muzzley.com/documentation/widgets.html)

void Muzzley::changeWidget(int participant_id, char \*widget_identifier, char \*widget_parameterization)

###### Parameters

 * `participant_id`: The id of the participant whose mobile device we want to change the interface.
 * `widget_identifier`: Type of widget (check widgets documentation for further information)
 * `widget_parameterization`: (optional) Widget options in a json char array.


#### sendSignal (void)

Sends a signal message to a specific participant.

void Muzzley::sendSignal(int participant_id, char\* namespace, char\* data, SignalCallback callback)

###### Parameters

 * `participant_id`: The id of the destination participant.
 * `namespace`: A namespace for your message.
 * `data`: The data you wish to send in a json char array.
 * `callback`: (optional) If you desire to have a callback for the message, you can declare a function to handle it and use this parameter.

###### Callback usage example

```
// My own signal callback
void myMessageCallback(bool success, JsonHashTable msg){
  // Do something when my message is replied
}

// Sending a message to a mobile device with callback
muzzley.sendSignal(1, "testme", "{\"key\":\"Test me\"}", myMessageCallback);
```

The method you implement for callback requires to be of the return type void with two parameters:

 * `success`: A boolean indicating wether the operation was successfull or not.
 * `msg`: A JsonHashTable object containing the replied message.


#### nextTick (void)

This method must be inside the sketch loop. It tracks the muzzley messages and cleans the deprecated rpc messages.

void Muzzley::nextTick()


### Application events

Muzzley notifies you when certain events occur, to be able to handle those notifications, it is required that you delegate the methods you want to use to handle each of them.

#### Activity created event

This event is related with the method createApp. It notices you that the muzzley activity was created and it is ready to be used.

In order to catch this event you need to delegate a method to be called when it occurs with the return type void and taking three parameters:

 * `activityId`: The key that participants should use to join the activity.
 * `qrCodeUrl`: An url to the QrCode source image, participants can use it to join the activity by scanning it.
 * `deviceId`: Your device id in the muzzley network.

```
// My activity created handler
void activityCreated(char* activityId, char* qrCodeUrl, char* deviceId) {
  Serial.println("--------- Activity Created -------");
  Serial.println(activityId);
  Serial.println(qrCodeUrl);
  Serial.println(deviceId);
}

// Tell the lib that this kind of events should be handled with the function declared above
muzzley.setActivityReadyHandler(activityCreated);
```

#### Participant joined event

When an activity is created, users can use their mobile devices to join it. Whenever a participant joins an activity a participant joined event is fired.

To be able to get this event you need to declare one handler for it with a void return type and receiving a struct of the type Participant.

```
struct Participant{
  int id;
  char profileId[];
  char name[];
  char photoUrl[];
  char deviceId[];
};
```

 * `id`: The id of the participant in the activity.
 * `profileId`: The participant profile id in the Muzzley cloud, unique per muzzley user.
 * `name`: The participant name.
 * `photoUrl`: The participant photo url.
 * `deviceId`: The device id of the participant, unique per device.


```
// My participant joined handler. Whenever a participant joins the activity this method is called
void participantJoined(Participant p){
  Serial.println("------ Participant joined -------");
  Serial.println(p.id);
  Serial.println(p.profileId);
  Serial.println(p.name);
  Serial.println(p.photoUrl);
  Serial.println(p.deviceId);
}

// Tell the lib that this kind of events should be handled with the function declared above
muzzley.setParticipantJoinHandler(participantJoined);
```

#### Participant widget changed event

This event relates to the changeWidget method. It is fired when a participant changes his widget. From this moment on it is possible to communicate with the specific widget.

To be able to get this event you need to declare one handler with a void return type and receiving a participant id as parameter.

```
// My widget changed handler
void widgetChanged(int participant_id){
  Serial.println("Widget has changed");
}

// Declare the method to handle widget changed events
muzzley.setParticipantWidgetChanged(widgetChanged);
```

#### Signaling message event

This event is fired whenever a participant sends a message to the galileo app.
In order to catch it you need to delegate a method to handle it.

The method returns char*, receiving as parameters:

 * `participant_id`: The id of the participant who sent the message.
 * `namespace`: The namespace or the type of the message.
 * `message`: A jsonHashTable object containing the json message.


```
// My message arrived event handler
char * onSignalingMessage(int participant_id, char* namespace, JsonHashTable message){
  Serial.println(participant_id);
  Serial.println(namespace);
  
  if(strcmp(namespace, "my_messages")==0){
    Serial.println("Received something from my messages");

    // If a callback is expected by the participant
    return("\"s\":true, \"d\":{\"test\":\"Galileo is calling you back!\"}"); 
  }
  
  if(strcmp(namespace, "my_other_messages")==0){
    Serial.println("Received something from my other messages");

    // If the participant is not expecting a callback
    return NULL; 
  }

  return NULL;
}

// Delegate the onSignalingMessage method has the handler to receive message from participants
muzzley.setSignalingMessagesHandler(onSignalingMessage);
```

If no callback is expected by the participant, you should return NULL, otherwise you should  return the response message as JSON where `s` is a boolean (required if not returning NULL) specifying if the operation was or not sucessfull. `d` is the data object you want to send back to the participant and it is optional.

#### Participant quit event

Fired whenever a participant quits

```
// Handle participant quit events
void onParticipantQuit(int participant_id){
  Serial.println("Participant quit");
}

// Delegate the method to handle the participant quit event
muzzley.setParticipantQuitHandler(onParticipantQuit);
```

#### Muzzley disconnect event

Fired when the connection with the Muzzley servers is lost

```
void onClose(){
  Serial.println("Connection to muzzley lost"); 
}

// Delegate the method to handle the muzzley disconnect event
muzzley.setOnCloseHandler(onClose);
```

### Extracting Json messages from the JsonHashTable

JsonHashTable is a type of object containing a JSON message.
To extract the values you can use the following methods:

 * `JsonHashTable::getString('key')`: Returns a char* value from the given key.
 * `JsonHashTable::getLong('key')`: Returns a long value from the given key.
 * `JsonHashTable::getDouble('key')`: Returns a double value from the given key.
 * `JsonHashTable::getBool('key')`: Returns a boolean value from the given key. 
 * `JsonHashTable::getArray('key')`: Returns a JsonArray object from the given key.
 * `JsonHashTable::getHashTable('key')`: Returns a nested JsonHashTable object from the given key.

 ##### Examples

Considering that `JsonHashtable my_json` contains `{"name": "Joe", "married": false, "age": 40}`
You can obtain its values the following way:

```
  char* person_name = my_json.getString("name");
  boolean is_married = my_json.getBoolean(married);
  int age = my_json.getLong("age");
```

Example with nested JSON:

Considering that `JsonHashtable my_json` contains `{"name": "Joe", "pet":{"name": "Sparky"}}`
You can obtain its values the following way:


```
  char* person_name = my_json.getString("name");
  JsonHashTable pet_json = my_json.getJsonHashTable("pet");
  char* pet_name = pet_json.getString("name");
```

For more information on how to extract the JSON values, check the [ArduinoJsonParser](https://github.com/bblanchon/ArduinoJsonParser) project.


## Muzzley connector usage example

This example allows you to use a light switch widget in your mobile device, and use it to change the pin 13 high/low values

```
#include <Muzzley.h>
#include <JsonObjectBase.h>
#include <JsonHashTable.h>
#include <JsonParser.h>
#include <JsonArray.h>
#include <Ethernet.h>


Muzzley muzzley;
int ledPin = 13;
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

/** When an activity is ready */
void activityCreated(char* activityId, char* qrCodeUrl, char* deviceId) {
  Serial.println("--------- Activity Created -------");
  Serial.println(activityId);
  Serial.println(qrCodeUrl);
  Serial.println(deviceId);
}

/** When a participant joins the activity */
void participantJoined(Participant p){
  Serial.println("------ Participant joined -------");
  Serial.println(p.id);
  Serial.println(p.profileId);
  Serial.println(p.name);
  Serial.println(p.photoUrl);
  Serial.println(p.deviceId);
  muzzley.changeWidget(p.id, "switch", "{\"isOn\":0}");
}


/** When a participant widget has changed */
void widgetChanged(int participant_id){
  Serial.println("Widget has changed");
  Serial.println(participant_id);

}

/** My custom signal callback */
void handleMySignalResponse(bool success, JsonHashTable msg){
  Serial.println("Received callback from webview");
  Serial.println(msg.getString("key"));
}


/** When a signal arrives */
char * onSignalingMessage(int participant_id, char* type, JsonHashTable message){
  
 /**
    From the Muzzley widget documentation
    The expected widget message is:
    
    {
      "w": "switch",
      "e": "press",
      "c": "switch",
      "v": 1
    }
 */
  if(message.containsKey("w")){
    if(strcmp("switch", message.getString("w")) == 0){
      Serial.println("Message from Switch widget");
      
      if(message.containsKey("v")){
        Serial.println(message.getLong("v"));
        
        if(message.getLong("v") == 1){
          digitalWrite(ledPin, HIGH);
          Serial.println("Setting value high");
        }else{
          digitalWrite(ledPin, LOW);
          Serial.println("Setting value low");
        }
      }    
    }
  }
  
  return NULL;
}

void onParticipantQuit(int pid){
  Serial.println("Participant quit");
}

void onClose(){
  Serial.println("Connection to muzzley lost"); 
}

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac);
  delay(2000);
  
  pinMode(ledPin, LOW);
  
  //Declare your event handlers
  muzzley.setActivityReadyHandler(activityCreated);
  muzzley.setParticipantJoinHandler(participantJoined);
  muzzley.setParticipantWidgetChanged(widgetChanged);
  muzzley.setSignalingMessagesHandler(onSignalingMessage);
  muzzley.setParticipantQuitHandler(onParticipantQuit);
  muzzley.setOnCloseHandler(onClose);
  pinMode(ledPin, OUTPUT);

  muzzley.connectApp("my_app_token");
}


void loop() {
  muzzley.nextTick();
}
```