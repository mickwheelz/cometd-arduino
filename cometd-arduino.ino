//Required Libraries
#include <ESP8266HTTPClient.h> //Inbuilt ESP8266 Library
#include <ESP8266WiFi.h> //Inbuilt ESP8266 Library
#include <ArduinoJson.h> //From https://arduinojson.org/
#include <Wire.h>

//set to true for more verbose output
#define DEBUG true

//set your wifi credentials here
const char* ssid = "dd-wrt";
const char* password = "buttsecks";

//Salesforce User Settings
const char* sfUsername = "mick.wheelz@gmail.com.iotdemo";
const char* sfPassword = "InItial95!";
const char* sfToken = "YENekR1U0Z97ICk97Ce7JzC3";

//Client ID and Secret from your connected app
const char* sfClientKey = "1324746716392609438"; //Client Secret
const char* sfClientId = "3MVG95NPsF2gwOiPdmdKANHxqeDtBLK6ealCif5bBHnQSCCOwRFmClncttsFnfW1yNo61asREG1xG3TVJg5F0"; 
const char* sfLoginFingerprint = "0B:33:19:AC:6D:9E:C1:5F:08:AB:93:17:2A:FE:F9:E0:90:69:C7:9A";
const char*  sfInstanceFingerprint = "BA:69:22:06:CA:B4:1D:B9:0D:01:AB:1F:98:BB:1F:53:37:64:95:98";

//vars to store SF auth token and SF instance url
String sfAuthToken;
String sfInstanceURL;

//vars to store client id and cookies
String cookies;
String clientId;

const long interval = 100;
unsigned long previousMillis = 0;

//http client to use for all connection
HTTPClient http;

// Login method, will return a JSON object with auth token and instance URL you can use to perform requests
void doLogin(const char* username, const char* password, const char* token, const char* clientId, const char* clientKey, const char* fingerprint) {

  http.begin("https://login.salesforce.com/services/oauth2/token", fingerprint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postBody = "?&grant_type=password";
  postBody += "&client_id=" + (String)clientId;
  postBody += "&client_secret=" + (String)clientKey;
  postBody += "&username=" + (String)username;
  postBody += "&password=" + (String)password + (String)token;

  int httpCode = http.POST(postBody);
  
  if (DEBUG) {
    Serial.print("http result:");
    Serial.println(httpCode);
  }

  String payload = http.getString();

  http.end();
  StaticJsonBuffer<1024> jsonBuffer;
  
  JsonObject& auth = jsonBuffer.parseObject(payload);

  if (DEBUG) {
    Serial.println("Response: ");
    Serial.println(payload);
  }
  
  if (httpCode == 200) {
    Serial.println("Successfully logged in!");
    sfAuthToken = auth["access_token"].asString();
    sfInstanceURL = auth["instance_url"].asString();
  }
  else {
    Serial.println("An error occured, not logged in!");
  }

}

String cleanPayload(String payload) {
  return payload.substring(1, payload.length() - 1);
}

void setupConnection(String topic) {

  Serial.println("Establishing CometD Connection...");
  topic = "/event/" + topic;  
  const char * headerKeys[] = {"Set-Cookie"};


  const int BUFFER_SIZE = 4096;
  
  /*StaticJsonBuffer<BUFFER_SIZE> jsonBufferHandshake;

  JsonObject& handshake = jsonBufferHandshake.createObject();
  
  handshake["channel"] = "/meta/handshake";
  handshake["version"] = "1.0";
  handshake["minimumVersion"] = "1.0";
  JsonArray& supportedConnectionTypes = handshake.createNestedArray("supportedConnectionTypes");
  supportedConnectionTypes.add("long-polling"); */

  http.begin(sfInstanceURL + "/cometd/40.0", sfInstanceFingerprint);
  http.addHeader("Authorization", "OAuth " + (String)sfAuthToken);
  http.addHeader("Content-Type", "application/json");
  http.collectHeaders(headerKeys, 1);
  
  String handshakePOST = "[{\"channel\": \"/meta/handshake\",\"version\": \"1.0\",\"minimumVersion\": \"1.0\",\"supportedConnectionTypes\": [\"long-polling\"]}]";
  //handshake.printTo(handshakePOST);
  int httpCode = http.POST(handshakePOST);
  cookies = http.header("Set-Cookie").c_str();
  
  StaticJsonBuffer<1024> jsonBufferHandshakeResponse;
  String payload = cleanPayload(http.getString());
  JsonObject& handshakeResponse = jsonBufferHandshakeResponse.parseObject(payload);
  clientId = handshakeResponse["clientId"].asString();
  
  if(DEBUG) {
    Serial.println("HANDSHAKE JSON SENT:");
    Serial.println(handshakePOST);
    Serial.println("PAYLOAD RECIEVED");
    Serial.println(payload);
    Serial.println("CLIENT ID RECIEVED");
    Serial.println(clientId);
    Serial.println("COOKIES RECIEVED");
    Serial.println(cookies);
  }

  /*StaticJsonBuffer<BUFFER_SIZE> jsonBufferConnect;

  JsonObject& connectObj = jsonBufferConnect.createObject();

  connectObj["channel"] = "/meta/connect";
  connectObj["clientId"] = clientId;
  connectObj["connectionType"] = "long-polling"; */
  
  String connectionPOST = "[{\"channel\": \"/meta/connect\",\"clientId\":\"" + clientId + "\",\"connectionType\":\"long-polling\"}]";
  //connectObj.printTo(connectionPOST);
  
  http.addHeader("Cookie", cookies);
  int conHttp = http.POST(connectionPOST);
  String connectionRES = http.getString();
  
  Serial.println("Handshake Complete");
  
  if(DEBUG) {
    Serial.println("CONNECT JSON SENT:");
    Serial.println(connectionPOST);
    Serial.println("RESPONSE RECIEVED");
    Serial.println(connectionRES);
  }

  /*StaticJsonBuffer<BUFFER_SIZE> jsonBufferSubscribe;

  JsonObject& subscribeObj = jsonBufferSubscribe.createObject();

  subscribeObj["channel"] = "/meta/subscribe";
  subscribeObj["clientId"] = clientId;
  subscribeObj["subscription"] = topic;*/

  String subscribePOST = "[{\"channel\": \"/meta/subscribe\",\"clientId\":\"" + clientId + "\",\"subscription\":\"" + topic +"\"}]";
  //subscribeObj.printTo(subscribePOST);  
  
  http.addHeader("Cookie", cookies);
  int subHttp = http.POST(subscribePOST);
  String subscribeRES = http.getString();

  if(DEBUG) {
    Serial.println("SUBSCRIBE JSON SENT:");
    Serial.println(subscribePOST);
    Serial.println("RESPONSE RECIEVED");
    Serial.println(subscribeRES);
  }

  Serial.println("Connected and subscribed to topic " + topic);

}

String doPoll() {
  /*DynamicJsonBuffer jsonBuffer(2048);
  /JsonObject& connectObj = jsonBuffer.createObject();
  connectObj["channel"] = "/meta/connect";
  connectObj["clientId"] = clientId;
  connectObj["connectionType"] = "long-polling";
  
  String connectionPOST;
  connectObj.printTo(connectionPOST);*/
  String connectionPOST = "[{\"channel\": \"/meta/connect\",\"clientId\":\"" + clientId + "\",\"connectionType\":\"long-polling\"}]";

  http.addHeader("Cookie", cookies);
  int conHttp = http.POST(connectionPOST);
  String pollRES = http.getString();
  
  if(DEBUG) {
    Serial.println(clientId);
    Serial.println("POLL SENT");
    Serial.println(connectionPOST);
    Serial.println("RESPONSE");
    Serial.println(pollRES);
  }
  return pollRES;
}

bool insertSObject(String objectName, JsonObject& object) {
  
  bool insertSuccess = false;
  String reqURL = (String)sfInstanceURL + "/services/data/v40.0/sobjects/" + (String)objectName;

  if(DEBUG) {
    Serial.println("Instance URL: " + (String)sfInstanceURL);
    Serial.println("Auth Token: " + (String)sfAuthToken);
    Serial.println("Request URL: " + reqURL);
    Serial.println("JSON Sent: ");
    object.printTo(Serial);
    Serial.println();
  }

  HTTPClient http;
  http.begin(reqURL, sfInstanceFingerprint);
  http.addHeader("Authorization", "Bearer " + (String)sfAuthToken);
  http.addHeader("Content-Type", "application/JSON");
  
  String objectToInsert;
  object.printTo(objectToInsert);
    
  int httpCode = http.POST(objectToInsert);
  
  http.end();

  if(httpCode == 201) {
    insertSuccess = true;
  }

  if (DEBUG) {
    String payload = http.getString();
    Serial.println("HTTP Code:");
    Serial.println(httpCode);
    Serial.println("HTTP Response:");
    Serial.println(payload);
  }

  return insertSuccess;
}

JsonObject& buildPlatformEvent(String watts) {


  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  
  root["Device_ID__c"] = ESP.getChipId();
  root["Reading__c"] = watts;
  
  return root;

}

//standard arduino setup method
void setup() {
  Serial.begin(115200);
  Wire.begin(D1,D2); // join i2c bus (address optional for master)
  Serial.println("Chip ID");
  Serial.println(ESP.getChipId());
  
  WiFi.begin(ssid, password);

  Serial.println("Waiting for WiFi Connection");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.println("WiFi Connected!");

  //once online, login to salesforce
  doLogin(sfUsername, sfPassword, sfToken, sfClientId, sfClientKey, sfLoginFingerprint);

  setupConnection("Smart_Device_Event__e");
  
}

String getWatts() {
  String wattsVal;
  Wire.requestFrom(9, 8); 
  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read();
    if(c != 255) {
      wattsVal += c;
    }
  }
  return wattsVal;
}

//standard arduino loop method
void loop() {

  String watts = getWatts();
  if(watts.length() > 0){
  Serial.println(watts);
    JsonObject& event = buildPlatformEvent(watts);
    bool insertSuccess = insertSObject("Smart_Meter_Reading__e", event); 
  }
  else {
    Serial.println("no watts");
  }

   String pollResult = doPoll();
   if(pollResult.length() > 0) {
    Serial.println(pollResult);
   }

/*    StaticJsonBuffer<1024> jsonBuffer;
    StaticJsonBuffer<1024> jsonOutputBuffer;

    
    String pollResult = doPoll();
    if(pollResult.length() > 0) {
    //Serial.println(pollResult);
    JsonObject& root = jsonBuffer.parseObject(cleanPayload(pollResult));
    String data = root["data"];
    JsonObject& dataObj = jsonBuffer.parseObject(data);
    String payload = dataObj["payload"];
    JsonObject& payloadObj = jsonBuffer.parseObject(payload);
    
    if(payloadObj["Device_ID__c"] == ESP.getChipId()) { 
      if(DEBUG){
        Serial.println("Dev Id match");
      }
      JsonObject& output = jsonOutputBuffer.createObject();
      String pinName = payloadObj["Pin_ID__c"];
      int pinValue = payloadObj["Value__c"];
      output["pin"] = pinName;
      output["value"] = pinValue;       
      output.printTo(Serial);
      }
    }*/

  delay(1000);
}
