#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Arduino.h>

// Wi-Fi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// MQTT Broker
const char* mqtt_server = "YOUR_MQTT_BROKER_IP";
const int mqtt_port = 1883;
const char* mqtt_user = "YOUR_MQTT_USERNAME";
const char* mqtt_password = "YOUR_MQTT_PASSWORD";

// MQTT Topics
const char* mqtt_topic_temp = "smartcradle/temperature";
const char* mqtt_topic_humidity = "smartcradle/humidity";
const char* mqtt_topic_cry = "smartcradle/cry";

// Pin Definitions
const int soundSensorPin = A0; // Analog pin for sound sensor
const int DHTPin = D4; // Digital pin for DHT11 sensor
const int rgbLedRedPin = D5; // Digital pin for RGB LED - Red
const int rgbLedGreenPin = D6; // Digital pin for RGB LED - Green
const int rgbLedBluePin = D7; // Digital pin for RGB LED - Blue

// Sensor objects
DHT dht(DHTPin, DHT11);

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println(message);

  // Blink RGB LED when cry is detected
  if (String(topic) == mqtt_topic_cry && message == "Baby crying") {
    for (int i = 0; i < 3; i++) {
      digitalWrite(rgbLedRedPin, HIGH);
      delay(100);
      digitalWrite(rgbLedRedPin, LOW);
      digitalWrite(rgbLedGreenPin, HIGH);
      delay(100);
      digitalWrite(rgbLedGreenPin, LOW);
      digitalWrite(rgbLedBluePin, HIGH);
      delay(100);
      digitalWrite(rgbLedBluePin, LOW);
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic_cry);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  pinMode(rgbLedRedPin, OUTPUT);
  pinMode(rgbLedGreenPin, OUTPUT);
  pinMode(rgbLedBluePin, OUTPUT);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Publish temperature and humidity readings to MQTT broker
  if (!isnan(temperature)) {
    client.publish(mqtt_topic_temp, String(temperature).c_str());
  }
  if (!isnan(humidity)) {
    client.publish(mqtt_topic_humidity, String(humidity).c_str());
  }

  // Read sound sensor for cry detection
  int soundValue = analogRead(soundSensorPin);
  if (soundValue > 500) {
    client.publish(mqtt_topic_cry, "Baby crying");
    // Simulate soothing lullaby
    tone(9, 262, 1000); // Play note C4 for 1 second
    delay(1000);
    tone(9, 330, 1000); // Play note E4 for 1 second
    delay(1000);
    tone(9, 392, 1000); // Play note G4 for 1 second
    delay(1000);
    noTone(9); // Stop playing
  }

  delay(10000); // Adjust delay as needed
}
