
//Required Libraries
#include <ESP8266HTTPClient.h> //Inbuilt ESP8266 Library
#include <ESP8266WiFi.h> //Inbuilt ESP8266 Library
#include <ArduinoJson.h> //From https://arduinojson.org/

//set to true for more verbose output
#define DEBUG false

//set your wifi credentials here
const char* ssid = "ssid";
const char* password = "pass";

//Salesforce User Settings
const char* sfUsername = "user@domain.com";
const char* sfPassword = "pass";
const char* sfToken = "token";

//Client ID and Secret from your connected app
const char* sfClientKey = "clientKey"; //Client Secret
const char* sfClientId = "clientId";

/* SSL Certificate fingerprints, you need one for login.salesforce.com and one for your instance url (e.g eu1.salesforce.com or domain.my.salesforce.com)
   you can get this by executing: "echo | openssl s_client -connect <your instance url>:443 | openssl x509 -fingerprint -noout" on liux or macos, if you use windows you're on your own
   The ESP lacks the horsepower to do this on the fly */
const char*  sfLoginFingerprint = "0B:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx";
const char*  sfInstanceFingerprint = "0B:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx";

//vars to store SF auth token and SF instance url
String sfAuthToken = "";
String sfInstanceURL = "";

//vars to store client id and cookies
String cookies;
String clientId;

//http client to use for all connection
HTTPClient http;

// Login method, will return a JSON object with auth token and instance URL you can use to perform requests
void doLogin(String username, String password, String token, String clientId, String clientKey, String fingerprint) {

  //you can change this to test.salesforce.com if you need to
  http.begin("https://login.salesforce.com/services/oauth2/token", fingerprint);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postBody = "?&grant_type=password";
  postBody += "&client_id=" + clientId;
  postBody += "&client_secret=" + clientKey;
  postBody += "&username=" + username;
  postBody += "&password=" + password + token;

  int httpCode = http.POST(postBody);
  
  if (DEBUG) {
    Serial.print("http result:");
    Serial.println(httpCode);
  }

  String payload = http.getString();

  http.end();

  DynamicJsonBuffer jsonBuffer(http.getString().length());
  JsonObject& auth = jsonBuffer.parseObject(payload);

  if (DEBUG) {
    Serial.println("Response: ");
    Serial.println(payload);
  }

  if (httpCode == 200) {
    Serial.println("Successfully logged in!");
    String token = auth["access_token"];
    String url = auth["instance_url"];
    sfAuthToken = token;
    sfInstanceURL = url;
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

  DynamicJsonBuffer jsonBuffer(2048);
  JsonObject& handshake = jsonBuffer.createObject();
  
  handshake["channel"] = "/meta/handshake";
  handshake["version"] = "1.0";
  handshake["minimumVersion"] = "1.0";
  JsonArray& supportedConnectionTypes = handshake.createNestedArray("supportedConnectionTypes");
  supportedConnectionTypes.add("long-polling");

  http.begin(sfInstanceURL + "/cometd/40.0", sfInstanceFingerprint);
  http.addHeader("Authorization", "OAuth " + (String)sfAuthToken);
  http.addHeader("Content-Type", "application/json");
  http.collectHeaders(headerKeys, 1);
  
  String handshakePOST;
  handshake.printTo(handshakePOST);
  int httpCode = http.POST(handshakePOST);
  cookies = http.header("Set-Cookie");
  JsonObject& handshakeResponse = jsonBuffer.parseObject(cleanPayload(http.getString()));
  String cl = handshakeResponse["clientId"];

  clientId = cl;
  
  if(DEBUG) {
    Serial.println("HANDSHAKE JSON SENT:");
    Serial.println(handshakePOST);
    Serial.println("CLIENT ID RECIEVED");
    Serial.println(clientId);
    Serial.println("COOKIES RECIEVED");
    Serial.println(cookies);
  }

  JsonObject& connectObj = jsonBuffer.createObject();

  connectObj["channel"] = "/meta/connect";
  connectObj["clientId"] = clientId;
  connectObj["connectionType"] = "long-polling";
  
  String connectionPOST;
  connectObj.printTo(connectionPOST);
  
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

  JsonObject& subscribeObj = jsonBuffer.createObject();

  subscribeObj["channel"] = "/meta/subscribe";
  subscribeObj["clientId"] = clientId;
  subscribeObj["subscription"] = topic;

  String subscribePOST;
  subscribeObj.printTo(subscribePOST);  
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
  DynamicJsonBuffer jsonBuffer(2048);
  JsonObject& connectObj = jsonBuffer.createObject();
  connectObj["channel"] = "/meta/connect";
  connectObj["clientId"] = clientId;
  connectObj["connectionType"] = "long-polling";
  
  String connectionPOST;
  connectObj.printTo(connectionPOST);
  
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

//standard arduino setup method
void setup() {
  Serial.begin(115200);
  Serial.println();
  
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

  setupConnection("platform_event__e");
  
}

//standard arduino loop method
void loop() {

  //only output if we have data
  String pollResult = doPoll();
  if(pollResult.length() > 0) {
    Serial.println(pollResult);
  }

  delay(100);

}
