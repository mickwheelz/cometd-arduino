# CometD for Arduino

This is an arduio sketch (designed for use with ESP8266 based boards) that you can use to subscribe to Salesforce platform events via CometD. To learn more about platform events, see this [trailhead](https://trailhead.salesforce.com/en/content/learn/modules/platform_events_basics).

This could also be used as a generic CometD client with some modifiction

## Instructions

You will need to set some values in the sketch before flashing to your device. This has been tested with an ESP8266, however ESP32 devices should work as well.

You can enable or disable debugging (more verbose output) by setting the DEBUG definition to true or false e.g
```
#define DEBUG true
```

You set your WiFi network credentials (SSID and password) in the ssid and password variables
```
const char* ssid = "ssid";
const char* password = "pass";
```

You set your salesforce credentials (username, password, token) in the sf variables e.g;
```
const char* sfUsername = "user@domain.com";
const char* sfPassword = "pass";
const char* sfToken = "token";
```
You will need a connected app created in salesforce (see [here](https://developer.salesforce.com/docs/atlas.en-us.api_rest.meta/api_rest/intro_defining_remote_access_applications.htm) for instructions on this) and you set the client key and secret in the below variables 
```
const char* sfClientKey = "clientKey"; //Client Secret
const char* sfClientId = "clientId";
```

Finally, you will need to get the SSL fingerprints for your instance, and the login url
You can get this by executing: `echo | openssl s_client -connect <your instance url>:443 | openssl x509 -fingerprint -noout` on linux or macOS

Once you have these, they are stored in the below variables
```
const char*  sfLoginFingerprint = "0B:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx";
const char*  sfInstanceFingerprint = "0B:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx:xx";
```

You can define the topic you want to listen for by passing it in to the setupConnection method e.g
```
setupConnection("platform_event__e");
```

Once setup and connected, this sketch will push the content of any platform event it recieves in JSON to the serial port, from here you can customise the logic to do whatever you like.
