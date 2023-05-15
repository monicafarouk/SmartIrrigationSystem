#include <WiFi.h>
#include <DHTesp.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define dht_apin 21   // Temp&Humi Sensor Pin D33
#define mh_apin A6    // Soil Sensor Pin D34
#define lm_apin A7    // Light Sensor Pin D35
#define m44_apin A4   // Rain Sensor Pin D32
#define relay_dpin 33

DHTesp dht;

const char* ssid     = "AirboxFC15";
const char* password = "Ag@1234=A+G";

// Define the server and API endpoint
const char *server = "https://agricalitresystem.azurewebsites.net/";
const char *endpoint = "api/embedded/create/";

struct readings {
  float temperature;
  float humidity;
  float rain;
  float soil;
  float light;
};

readings sensorR;

void setup() {

  Serial.begin(9600);                   // Start the Serial communication to send messages to the computer
  
  pinMode(dht_apin,INPUT);
  pinMode(mh_apin,INPUT);
  pinMode(lm_apin,INPUT);
  pinMode(m44_apin,INPUT);
  pinMode(relay_dpin,OUTPUT);
  
  delay(10);

  WiFi.mode(WIFI_STA);                    // WiFi mode Station (end device)
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);

  while (WiFi.status() != WL_CONNECTED)
  {                                     // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print('.');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  dht.setup(dht_apin, DHTesp::DHT11);
}

void loop()
{
  ReadDHT();
  ReadLight();
  ReadSoilMoisture();
  ReadRain();

  // Build the JSON payload
  String payload = "{\"humidity\":" + String(sensorR.humidity) + ",";
  payload += "\"temperature\":" + String(sensorR.temperature) + ",";
  payload += "\"light\":" + String(sensorR.light) + ",";
  payload += "\"soil_moisture\":" + String(sensorR.soil) + ",";
  payload += "\"rain\":" + String(sensorR.rain) + "}";

  // Send the data to the server
  SendData(payload);
  
  Serial.println();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void ReadDHT()
{
  delay(2000);
  
  sensorR.temperature = dht.getTemperature();
  sensorR.humidity = dht.getHumidity();

  Serial.print("Humidity: ");
  Serial.print(sensorR.humidity);
  Serial.print("%");

  Serial.print("  |  ");

  Serial.print("Temperature: ");
  Serial.print(sensorR.temperature);
  Serial.println("°C");
}

void ReadLight()
{
  sensorR.light = map(analogRead(lm_apin), 0, 1500, 0, 1023);
  
  Serial.print("Light Sensor Value: ");
  Serial.print(sensorR.light);

  // We'll have a few threshholds, qualitatively determined
  if (sensorR.light < 100) {
    Serial.println(" - Very bright");
  } else if (sensorR.light < 200) {
    Serial.println(" - Bright");
  } else if (sensorR.light < 500) {
    Serial.println(" - Light");
  } else if (sensorR.light < 800) {
    Serial.println(" - Dim");
  } else {
    Serial.println(" - Dark");
  }
}

void ReadSoilMoisture()
{
  sensorR.soil = map(analogRead(mh_apin), 0, 4095, 0, 1023);
  
  Serial.print("Soil Moisture Value: ");
  Serial.print(sensorR.soil);

  if (sensorR.soil < 500) {
    Serial.print(" - Soil is too wet");
    digitalWrite(relay_dpin,LOW);
    Serial.println(" - PUMP OFF");
  } else if (sensorR.soil >= 500 && sensorR.soil < 750) {
    Serial.print(" - Soil moisture is perfect");
    digitalWrite(relay_dpin,LOW);
    Serial.println(" - PUMP OFF");
  } else {
    Serial.print(" - Soil is too dry");
    digitalWrite(relay_dpin,HIGH);
    Serial.println(" - PUMP ON");
  }
}

void ReadRain()
{
  sensorR.rain = digitalRead(m44_apin);

  Serial.print("Rain Sensor Value: ");
  Serial.print(sensorR.rain);

  if (sensorR.rain) {
    Serial.println(" - Clear");
  } else {
    Serial.println(" - Raining");
  }
}

void SendData(String data) {
  
  HTTPClient http; // Create an HTTP client object
  
  // Build the URL for the API endpoint
  String url = String(server) + String(endpoint);// Send data to the server

  // Begin the HTTP connection and set the URL
  http.begin(url);

  // Set the HTTP request headers
  http.addHeader("Content-Type", "application/json");

  // Create the JSON payload with the data
  String payload = "{\"data\": " + data + "}";

  Serial.println();
  Serial.println("Sending data to " + url);
//  Serial.println(payload);
//  Serial.println();

  // Send the POST request with the payload to the API endpoint
  int httpCode = http.POST(payload);

  if (httpCode == HTTP_CODE_OK)
  {
    Serial.println("Data sent successfully");
  }
  else
  {
    Serial.println("Error sending data: " + String(httpCode));
  }

  // Close the HTTP connection
  http.end();
}

float getdata()
{
  HTTPClient http;
  String url =  String(server) + String(endpoint);
  http.begin(url);
  int httpCode = http.GET();
  float soilMoisture = -1;
  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    soilMoisture = payload.toFloat();
  }
  http.end();
  return soilMoisture;
}
