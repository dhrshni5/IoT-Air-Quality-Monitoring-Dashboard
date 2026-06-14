#include <WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>

// =========================
// WiFi Configuration
// =========================
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// =========================
// ThingSpeak Configuration
// =========================
unsigned long CHANNEL_ID = 3404697;
const char* WRITE_API_KEY = "AWXXFPL2RPT1N92U";

// =========================
// Pin Configuration
// =========================
#define DHTPIN 4
#define DHTTYPE DHT22

#define LED_PIN 27
#define BUZZER_PIN 26

// =========================
// Sensor Initialization
// =========================
DHT dht(DHTPIN, DHTTYPE);

WiFiClient client;

// =========================
// AQI Classification
// =========================
String classifyAQI(int value)
{
  if (value <= 100)
    return "GOOD";
  else if (value <= 200)
    return "MODERATE";
  else if (value <= 300)
    return "POOR";
  else
    return "HAZARDOUS";
}

// =========================
// Connect WiFi
// =========================
void connectWiFi()
{
  Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  dht.begin();

  randomSeed(millis());

  connectWiFi();

  ThingSpeak.begin(client);

  Serial.println("\n========================================");
  Serial.println(" IoT Air Quality Monitoring Dashboard ");
  Serial.println(" ESP32 + DHT22 + ThingSpeak");
  Serial.println("========================================\n");
}

void loop()
{
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity))
  {
    Serial.println("ERROR: Failed to read DHT22 sensor!");
    delay(2000);
    return;
  }

  int mq135Value = random(50, 401);

  String status = classifyAQI(mq135Value);

  String alertStatus = "NORMAL";

  // Alert Logic
  if (status == "GOOD")
  {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    alertStatus = "SAFE";
  }
  else if (status == "MODERATE")
  {
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(LED_PIN, LOW);

    noTone(BUZZER_PIN);

    alertStatus = "MODERATE WARNING";
  }
  else if (status == "POOR")
  {
    digitalWrite(LED_PIN, HIGH);

    tone(BUZZER_PIN, 1000);
    delay(300);
    noTone(BUZZER_PIN);

    alertStatus = "AIR QUALITY WARNING";
  }
  else
  {
    digitalWrite(LED_PIN, HIGH);

    tone(BUZZER_PIN, 2000);

    alertStatus = "DANGER";
  }

  // Serial Output
  Serial.println("========================================");

  Serial.print("Temperature (C): ");
  Serial.println(temperature);

  Serial.print("Humidity (%): ");
  Serial.println(humidity);

  Serial.print("MQ135 Value: ");
  Serial.println(mq135Value);

  Serial.print("Air Quality Status: ");
  Serial.println(status);

  Serial.print("Alert Status: ");
  Serial.println(alertStatus);

  // =========================
  // ThingSpeak Upload
  // =========================

  ThingSpeak.setField(1, mq135Value);
  ThingSpeak.setField(2, temperature);
  ThingSpeak.setField(3, humidity);
  ThingSpeak.setField(4, status);
  ThingSpeak.setField(5, alertStatus);

  int response = ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);

  if (response == 200)
  {
    Serial.println("ThingSpeak Upload Successful");
  }
  else
  {
    Serial.print("ThingSpeak Error Code: ");
    Serial.println(response);
  }

  Serial.println("========================================\n");

  // ThingSpeak minimum update interval
  delay(20000);
}