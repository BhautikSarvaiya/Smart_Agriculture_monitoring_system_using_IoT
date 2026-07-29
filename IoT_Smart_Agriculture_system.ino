#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//====================== WiFi ======================
const char* ssid = "Wokwi-GUEST";
const char* password = "";

//================== ThingsBoard ===================
const char* mqtt_server = "thingsboard.cloud";
const char* accessToken = "f8dh1reSSlPvR92wSeeV";

WiFiClient espClient;
PubSubClient client(espClient);

//====================== OLED ======================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

//====================== DHT22 =====================
#define DHTPIN 15
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

//====================== Pins ======================
#define SOIL_PIN 34
#define RELAY_PIN 26
#define LED_PIN 27

float temperature = 0;
float humidity = 0;
int soilRaw = 0;
int soilPercent = 0;
bool pumpStatus = false;

//----------------------WiFi+MQTT-------------------------

void connectWiFi() {

  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP : ");
  Serial.println(WiFi.localIP());

}

void reconnectMQTT() {

  while (!client.connected()) {

    Serial.print("Connecting to ThingsBoard... ");

    if (client.connect("ESP32Client", accessToken, NULL)) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

void sendTelemetry() {

  String payload = "{";
  payload += "\"temperature\":";
  payload += String(temperature);
  payload += ",";
  payload += "\"humidity\":";
  payload += String(humidity);
  payload += ",";
  payload += "\"soilMoisture\":";
  payload += String(soilPercent);
  payload += ",";
  payload += "\"pump\":";
  payload += pumpStatus ? "true" : "false";
  payload += "}";

  client.publish("v1/devices/me/telemetry",
                 payload.c_str());

  Serial.println(payload);

}

//--------------------------------------

void setup() {

  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  Wire.begin(21,22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED Failed");
    while (true);
  }

  display.clearDisplay();
  display.display();

  connectWiFi();

  client.setServer(mqtt_server, 1883);
  client.setBufferSize(512);

}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  // Read Sensors
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  soilRaw = analogRead(SOIL_PIN);

  // Convert ADC to %
  soilPercent = map(soilRaw, 0, 4095, 100, 0);

  // Automatic Pump Control
  if (soilPercent < 30) {

    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    pumpStatus = true;

  } else {

    digitalWrite(RELAY_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    pumpStatus = false;

  }

  // OLED Display
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);
  display.println("SMART AGRICULTURE");

  display.setCursor(0,15);
  display.print("Temp : ");
  display.print(temperature);
  display.println(" C");

  display.setCursor(0,28);
  display.print("Hum  : ");
  display.print(humidity);
  display.println(" %");

  display.setCursor(0,41);
  display.print("Soil : ");
  display.print(soilPercent);
  display.println(" %");

  display.setCursor(0,54);

  if (pumpStatus)
    display.print("Pump : ON");
  else
    display.print("Pump : OFF");

  display.display();

  // Serial Monitor
  Serial.println("----------------------");
  Serial.print("Temperature : ");
  Serial.println(temperature);

  Serial.print("Humidity : ");
  Serial.println(humidity);

  Serial.print("Soil Moisture : ");
  Serial.println(soilPercent);

  Serial.print("Pump : ");
  Serial.println(pumpStatus ? "ON" : "OFF");

  // Send data to ThingsBoard
  sendTelemetry();

  delay(2000);

}
