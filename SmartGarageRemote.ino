#include <Servo.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
// #include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>;
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

WiFiClient espClient;
PubSubClient client(espClient);
char *device_id;
char *access_token;
char *SUB_TOPIC = "cmd/";
char *sub_topic = (char *)malloc(128);
char *PUB_TOPIC = "signal/";
char *pub_topic = (char *)malloc(128);
char *MQTT_HOST = "94.101.176.204";
int MQTT_PORT = 1883;

char *ssid;
char *pass;
bool has_ssid_pass = false;
bool has_ssid_pass_really = false;
bool connected_to_wifi = false;
bool connected_to_mqtt = false;

const char *ACCESS_POINT_SSID = "parking_remote";
const char *ACCESS_POINT_PASS = "testtest";
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(8080);

int doorPin = D0;
int checkPin = D1;

long startTime = 0;
long lastMetricSent = 0;

void setup()
{
  ESP.eraseConfig();
  Serial.begin(9600);
  delay(10);
  Serial.println("V1.0");
  setupAP();
  setupServer();
  pinMode(doorPin, OUTPUT);
  pinMode(checkPin, OUTPUT);
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
  Serial.printf("connecting to %s %s\n", ssid, pass);
  // if (!WiFi.config(local_ip, gateway, subnet)){Serial.println("STA Failed to configure");}
  WiFi.begin(ssid, pass);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(500);
    //    Serial.print(++i); Serial.print(' ');
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
  //  device_id = "A4:CF:12:F0:00:B3";
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
  if (data["signals"].containsKey("learn"))
  {
    int mode = data["signals"]["mode"];
    learn(mode);
  }
  else if (data["signals"].containsKey("operate"))
  {
    int mode = data["signals"]["mode"];
    operate(mode);
  }
  return;
}

bool connectToMqtt()
{
  //  Serial.printf("Connecting to mqtt %s:%d\n", MQTT_HOST, MQTT_PORT);
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(callback);
  if (!client.connect(device_id, device_id, access_token))
  {
    Serial.println("Failed to connect to mqtt!");
    delay(1000);
    return false;
  }
  sub_topic[0] = '\0';
  strcat(sub_topic, SUB_TOPIC);
  strcat(sub_topic, device_id);
  pub_topic[0] = '\0';
  strcat(pub_topic, PUB_TOPIC);
  strcat(pub_topic, device_id);
  client.subscribe(sub_topic);
  //  Serial.println("Connected to mqtt");
  return true;
}

void sendMetrics()
{
  DynamicJsonDocument doc(2048);
  doc["id"] = device_id;
  char Buf[2048];
  serializeJson(doc, Buf);
  client.publish(PUB_TOPIC, Buf);
  Serial.println("Sent metrics");
  delay(1000);
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
  server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "PONG"); });

  server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request)
            {
    if (request->hasArg("ssid") && request->hasArg("pass") && request->arg("ssid") != NULL && request->arg("pass") != NULL) {
      ssid = getCharArrayFromString(request->arg("ssid"));
      pass = getCharArrayFromString(request->arg("pass"));
      char* data = (char*)malloc(1024);
      data[0] = '\0';
      strcat(data, "{\"device_id\":\"");
      strcat(data, device_id);
      strcat(data, "\",\"access_token\":\"");
      strcat(data, access_token);
      strcat(data, "\"}");
      request->send(200, "text/plain", data);
      has_ssid_pass = true;
      connected_to_wifi = false;
      has_ssid_pass_really = true;
      free(data);
    } else {
      request->send(400, "text/plain", "Bad args");
    } });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    DynamicJsonDocument doc(2048);
    doc["last_ssid"] = ssid;
    doc["connected_to_wifi"] = connected_to_wifi;
    doc["connected_to_mqtt"] = connected_to_mqtt;
    doc["wifi_status"] = WiFi.status();
    char Buf[2048];
    serializeJson(doc, Buf);
    request->send(200, "text/plain", Buf); });
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hi! I am ESP8266."); });
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(404, "text/plain", "404: Not found"); });

  AsyncElegantOTA.begin(&server);

  server.begin();
  Serial.println("Setup server done");
}

void learn(int mode)
{
  // TODO: save in memory
}

void operate(int mode)
{
  // TODO: read from memory
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
  }
  if (!client.connected() && connected_to_mqtt)
  {
    Serial.println("mqtt disconnected");
    connected_to_mqtt = false;
  }

  if (millis() - lastMetricSent > 3000)
  {

    if (connected_to_mqtt)
      sendMetrics();
    lastMetricSent = millis();
  }
}