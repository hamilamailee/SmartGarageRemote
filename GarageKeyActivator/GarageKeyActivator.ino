#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>;
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

WiFiClient espClient;
PubSubClient client(espClient);
char *device_id;
char *PUB_TOPIC = "/garage";
char *pub_topic = (char *)malloc(128);
char *MQTT_HOST = "addd6ec8.us-east-1.emqx.cloud";
int MQTT_PORT = 15345;
char *mqttUsername = "arduino";
char *mqttPassword = "arduino";

char *ssid = "FINAPPLE";
char *pass = "dooodeee";
bool has_ssid_pass = true;
bool has_ssid_pass_really = true;
bool connected_to_wifi = false;
bool connected_to_mqtt = false;

int keyPin = D1;
int lightPin = D2;
int signalId = 1;

const char *ACCESS_POINT_SSID = "parking_remote_key";
const char *ACCESS_POINT_PASS = "testtest";
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(8080);

void setup()
{
  ESP.eraseConfig();
  Serial.begin(9600);
  delay(10);
  connectToWifi();
  pinMode(keyPin, INPUT);
  pinMode(lightPin, OUTPUT);
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
  Serial.println("Access point setup done");
}

bool connectToWifi()
{
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
      has_ssid_pass = has_ssid_pass_really;
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
  Serial.printf("MacAddress: %s\n", device_id);
  return true;
}

bool connectToMqtt()
{
  client.setServer(MQTT_HOST, MQTT_PORT);
  if (!client.connect(device_id, mqttUsername, mqttPassword))
  {
    Serial.println("Failed to connect to mqtt!");
    delay(1000);
    return false;
  }
  return true;
}

void sendCommand(int id)
{
  DynamicJsonDocument doc(2048);
  doc["mode"] = "transmit";
  doc["id"] = id;
  char buf[2048];
  serializeJson(doc, buf);
  client.publish(PUB_TOPIC, buf);
  Serial.println("Sent command");
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
  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    if (request->hasArg("ssid") && request->hasArg("pass") && request->arg("ssid") != NULL && request->arg("pass") != NULL) {
      ssid = getCharArrayFromString(request->arg("ssid"));
      pass = getCharArrayFromString(request->arg("pass"));
      char* data = (char*)malloc(1024);
      data[0] = '\0';
      strcat(data, "Got wifi info!\0");
      request->send(200, "text/plain", data);
      has_ssid_pass = true;
      connected_to_wifi = false;
      has_ssid_pass_really = true;
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


void loop()
{
  client.loop();

  if (has_ssid_pass)
  {
    connected_to_wifi = connectToWifi();
    has_ssid_pass = false;
  }
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

  if (digitalRead(keyPin) == HIGH) {
    Serial.println("Transmission begin");
    sendCommand(signalId);
    digitalWrite(lightPin, HIGH);
    delay(1000);
    digitalWrite(lightPin, LOW);
    Serial.println("Transmission complete");
  }
}
