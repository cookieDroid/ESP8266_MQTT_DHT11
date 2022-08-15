#include <ESP8266WiFi.h>
#include <time.h>

#include "DHTesp.h"       // Click here to get the library: http://librarymanager/All#DHTesp
#include <PubSubClient.h> //https://github.com/knolleary/pubsubclient
#include <ArduinoJson.h>  //https://github.com/bblanchon/ArduinoJson


//sensor configuration
DHTesp dht;

//wifi configuration
const char* ssid = "bala"; //replace with actual wifi ID
const char* password = "bala1234"; //replace with actual password

//mqtt configuration
const char* mqtt_server = "broker.emqx.io"; //replace with actual mqtt IP
const char* sensorData = "esp8266/device01"; //replace with required topic

//install mosquitto in windows https://mosquitto.org/download/
//run the below command in terminal
//mosquitto_sub -h broker.emqx.io -t esp8266/device01 -v

//create wifi & mqtt clients
WiFiClient espClient;
PubSubClient client(espClient);

//set variables to save mqtt data
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (100)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Humidity (%)\tTemperature (C)\t");
  dht.setup(17, DHTesp::DHT11); // Connect DHT sensor to GPIO 17 
  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish("test/topic", "hello world");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop()
{
  delay(100);
  
  //get sensor data
  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();

  //connect to mqtt if disconnnected
  if (!client.connected()) {
    reconnect();
  }

  //display data in serial monitor for debugging purpose
  Serial.print(humidity, 1);
  Serial.print("\t\t");
  Serial.print(temperature, 1);
  Serial.print("\t\t");
  Serial.println();

  //get the current datetime in UTC format so that time can be converted to any timezone
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // UTC
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    yield();
    delay(500);
    Serial.print(F("."));
    now = time(nullptr);
  }
  Serial.println(F(""));
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
  String tmpBuf2 = asctime(&timeinfo);
  tmpBuf2.replace("\n","");


  //serialize the data in json format
  StaticJsonDocument<256> doc;
  doc["sensor"] = "DHT11";
  doc["time"] = tmpBuf2;
  doc["Humidity"] = humidity;
  doc["Temperature"]= temperature;
  serializeJson(doc, msg);

  //publish the data
  client.publish(sensorData, msg);
  client.loop();
  delay(1000);
}
