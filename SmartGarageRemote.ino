#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>;
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <RCSwitch.h>
#include <Preferences.h>

WiFiClient espClient;
PubSubClient client(espClient);
char *device_id;
char *access_token;
char *SUB_TOPIC = "/garage";
char *PUB_TOPIC = "/messages";
char *MQTT_HOST = "addd6ec8.us-east-1.emqx.cloud";
int MQTT_PORT = 15345;
char *mqttUsername = "arduino";
char *mqttPassword = "arduino";

String ssid = "NULL";
String pass = "NULL";
bool connected_to_wifi = false;
bool connected_to_mqtt = false;

RCSwitch receiver = RCSwitch();
RCSwitch transmitter = RCSwitch();

Preferences preferences;

const char *ACCESS_POINT_SSID = "parking_remote";
const char *ACCESS_POINT_PASS = "testtest";
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(8080);


int state = 0;
// 0 => normal (nothing)
// 1 => learn
// 2 => transmit
int selectedId = 0;

int resetButtonPin = D0;

void setup()
{
  ESP.eraseConfig();
  Serial.begin(9600);
  delay(10);
  Serial.println("V1.0");
  setupAP();
  setupServer();

  receiver.enableReceive(0);
  transmitter.enableTransmit(4);
  transmitter.setProtocol(1);
  transmitter.setPulseLength(350);
  transmitter.setRepeatTransmit(20);

  preferences.begin("my-app", false);

  ssid = preferences.getString("ssid", "NULL");
  pass = preferences.getString("password", "NULL");

  pinMode(resetButtonPin, INPUT);
  
  Serial.println("Setup done");
}

void setupAP()
{
  Serial.printf("AP name %s\n", ACCESS_POINT_SSID);
  Serial.print("Set config ");
  Serial.println(WiFi.softAPConfig(local_ip, gateway, subnet) ? "Successful" : "Failed!");
  Serial.print("Setup AP ");
  Serial.println(WiFi.softAP(ACCESS_POINT_SSID, ACCESS_POINT_PASS) ? "Successful" : "Failed!");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address");
  Serial.println(IP);
  Serial.println("Setup wifi done");
  Serial.println(getCharArrayFromString(WiFi.macAddress()));
  Serial.println();
  access_token = device_id = getCharArrayFromString(WiFi.macAddress());
}

bool connectToWifi()
{
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  ssid = preferences.getString("ssid", "NULL");
  pass = preferences.getString("pass", "NULL");

//  Serial.print("read ssid from memory: ");
//  Serial.println(ssid);
//  Serial.print("and password");
//  Serial.println(pass);

  if (ssid == "NULL" || pass == "NULL") {
    return false;
  }

  Serial.printf("connecting to %s %s\n", ssid, pass);
  WiFi.begin(ssid, pass);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(500);
    if (i == 20)
    {
      Serial.println("Failed to connect");
      WiFi.disconnect();
      return false;
    }
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());
  device_id = getCharArrayFromString(WiFi.macAddress());
  access_token = device_id;
  Serial.printf("MacAddress: %s\n", device_id);
  return true;
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.printf("Call back %s %s \n", topic, payload);
  DynamicJsonDocument data(length + 10);
  DeserializationError err = deserializeJson(data, payload);
  if (err)
  {
    Serial.print(("deserializeJson() failed: "));
  }
  if (!data.containsKey("mode") || !data.containsKey("id")) {
    sendError("Invalid json received thus ignored");
  }
  const char* mode = data["mode"];
  int id = data["id"];
  if (strcmp(mode, "learn") == 0) {
    selectedId = id;
    state = 1;
  } else if (strcmp(mode, "transmit") == 0) {
    selectedId = id;
    state = 2;
  } else { // cancel
    state = 0;
    selectedId = 0;
  }
  return;
}

bool connectToMqtt()
{
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);
  if (!client.connect(device_id, mqttUsername, mqttPassword))
  {
    Serial.println("Failed to connect to mqtt!");
    delay(1000);
    return false;
  }
  client.subscribe(SUB_TOPIC);
  //  Serial.println("Connected to mqtt");
  return true;
}

void sendMessage(char* message)
{
  DynamicJsonDocument doc(2048);
  doc["mode"] = "message";
  doc["message"] = message;
  char buf[2048];
  serializeJson(doc, buf);
  client.publish(PUB_TOPIC, buf);
  Serial.println("Sent message");
}

void sendError(char* message)
{
  DynamicJsonDocument doc(2048);
  doc["mode"] = "error";
  doc["message"] = message;
  char buf[2048];
  serializeJson(doc, buf);
  client.publish(PUB_TOPIC, buf);
  Serial.println("Sent message");
}


char *getCharArrayFromString(String str)
{
  int str_len = str.length() + 1;
  char *chr = (char *)malloc(str_len);
  str.toCharArray(chr, str_len);
  return chr;
}

void setupServer()
{
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(200, "text/plain", "PONG");
  });

  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    if (request->hasArg("ssid") && request->hasArg("pass") && request->arg("ssid") != NULL && request->arg("pass") != NULL) {

      ssid = request->arg("ssid");
      pass = request->arg("pass");
      preferences.putString("ssid", ssid);
      preferences.putString("pass", pass);
      Serial.print("Got ssid: ");
      Serial.println(ssid);
      Serial.print("Got password: ");
      Serial.println(pass);
      char* data = (char*)malloc(1024);
      data[0] = '\0';
      strcat(data, "{\"device_id\":\"");
      strcat(data, device_id);
      strcat(data, "\",\"access_token\":\"");
      strcat(data, access_token);
      strcat(data, "\"}");
      request->send(200, "text/plain", data);
      connected_to_wifi = false;
      free(data);
    } else {
      request->send(400, "text/plain", "Bad args");
    }
  });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    DynamicJsonDocument doc(2048);
    doc["last_ssid"] = ssid;
    doc["connected_to_wifi"] = connected_to_wifi;
    doc["connected_to_mqtt"] = connected_to_mqtt;
    doc["wifi_status"] = WiFi.status();
    char Buf[2048];
    serializeJson(doc, Buf);
    request->send(200, "text/plain", Buf);
  });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    request->send(200, "text/plain", "Hi! I am ESP8266.");
  });
  server.onNotFound([](AsyncWebServerRequest * request)
  {
    request->send(404, "text/plain", "404: Not found");
  });

  AsyncElegantOTA.begin(&server);

  server.begin();
  Serial.println("Setup server done");
}


void learn()
{
  Serial.print("Learning for id = ");
  Serial.println(selectedId);
  if (selectedId < 1 || selectedId > 5) {
    sendError("id must be between 1 and 5");
  } else {
    // do the actual learning
    if (receiver.available()) {
      int value = receiver.getReceivedValue();
      if (value == 0) {
        Serial.print("Unknown encoding");
      } else {
        unsigned long receivedValue = receiver.getReceivedValue();
        unsigned int bitLength = receiver.getReceivedBitlength();
        unsigned int protocol = receiver.getReceivedProtocol();
        Serial.print("Received ");
        Serial.print( receivedValue );
        Serial.print(" / ");
        Serial.print( bitLength );
        Serial.print("bit ");
        Serial.print("Protocol: ");
        Serial.println(protocol);

        // persist in memory
        char key[10];
        sprintf(key, "value%d", selectedId);
        preferences.putULong(key, receivedValue);

        sprintf(key, "bitLength%d", selectedId);
        preferences.putUInt(key, bitLength);

        Serial.println("Learning complete");
        sendMessage("Learned");
        state = 0;
      }
      receiver.resetAvailable();
    }
  }
}

void transmit()
{
  Serial.print("Transmitting for id = ");
  Serial.println(selectedId);

  if (selectedId < 1 || selectedId > 5) {
    sendError("id must be between 1 and 5");
  } else {
    char key[10];
    sprintf(key, "value%d", selectedId);
    unsigned long value = preferences.getULong(key, 0);
    sprintf(key, "bitLength%d", selectedId);
    unsigned int bitLength = preferences.getUInt(key, 0);

    Serial.print("Read values from memory -> value: ");
    Serial.print(value);
    Serial.print(" , bitLength: ");
    Serial.println(bitLength);

    transmitter.send(value, bitLength);
    Serial.println("Transmitted");
    sendMessage("Sent");
  }
  state = 0;
}

void checkForResetButton()
{
  if (digitalRead(resetButtonPin) == HIGH) {
    // reset network info
    Serial.println("Resetting");
    preferences.remove("ssid");
    preferences.remove("pass");
    WiFi.disconnect();
    setupAP();     
  }
}

void loop()
{
  checkForResetButton();
  
  client.loop();

  connected_to_wifi = connectToWifi();

  if (connected_to_wifi && !connected_to_mqtt)
  {
    Serial.println("Connecting to mqtt");
    connected_to_mqtt = connectToMqtt();
    Serial.println("Connected to mqtt");
  }
  if (!client.connected() && connected_to_mqtt)
  {
    Serial.println("mqtt disconnected");
    connected_to_mqtt = false;
  }


  if (state == 0) {
    // nothing yet
  } else if (state == 1) { // learn
    learn();
  } else if (state == 2) {
    transmit();
  } else {
    state = 0;
  }

}
