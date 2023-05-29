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
const char *server = "http://13.49.102.193:8000/";
const char *endpoint = "api/embedded/create/";

struct readings {
  float temperature;
  float humidity;
  float rain;
  float soil;
  String light;
  bool pumpStatus = false;
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

  StaticJsonDocument<200> doc;
     doc["temperature"] = sensorR.temperature;
     doc["humidity"] = sensorR.humidity;
     doc["light"] = sensorR.light;
     doc["rainfall"] = sensorR.rain;
     doc["soil_moisture"] = sensorR.soil;
     doc["pump_on"] = sensorR.pumpStatus;
     String jsonData;
     serializeJson(doc, jsonData);

     
// Send the data to the server
  SendData(jsonData);
//  Serial.println(jsonData);
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
   float lightValue = map(analogRead(lm_apin), 0, 1500, 0, 1023);
  
  Serial.print("Light Sensor Value: ");
  Serial.print(lightValue);

  // We'll have a few threshholds, qualitatively determined
  if (lightValue < 100) {
    sensorR.light = "Very bright";
    Serial.println(" - Very bright");
  } else if (lightValue < 200) {
    sensorR.light = "Bright";
    Serial.println(" - Bright");
  } else if (lightValue < 500) {
    sensorR.light = "Light";
    Serial.println(" - Light");
  } else if (lightValue < 800) {
    sensorR.light = "Dim";
    Serial.println(" - Dim");
  } else {
    sensorR.light = "Dark";
    Serial.println(" - Dark");
  }
}

void ReadSoilMoisture()
{
  sensorR.soil = map(analogRead(mh_apin), 0, 4095, 0, 1023);
  
    
  Serial.print("Soil Moisture Value: ");
  Serial.print(sensorR.soil);
  float soilMoisture[2];
  getData(server, soilMoisture);
  

  if (sensorR.soil < soilMoisture[0]) {
    Serial.print(" - Soil is too wet");
    digitalWrite(relay_dpin,LOW);
    sensorR.pumpStatus = false;
    Serial.println(" - PUMP OFF");
    Serial.println(soilMoisture[0]);
  }
  else if (sensorR.soil >= soilMoisture[0] && sensorR.soil< soilMoisture[1])
  {
    Serial.print(" - Soil moisture is perfect");
    digitalWrite(relay_dpin,LOW);
    sensorR.pumpStatus = false;
    Serial.println(" - PUMP OFF");
    Serial.println(soilMoisture[0],soilMoisture[1]);
  }
  else
  {
    Serial.print(" - Soil is too dry");
    digitalWrite(relay_dpin,HIGH);
    sensorR.pumpStatus = true;
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
  //String payload = "data\"={" + data + "}";
//  Serial.println(data);

  Serial.println();
  Serial.println("Sending data to " + url);
//  Serial.println(payload);
//  Serial.println();

  // Send the POST request with the payload to the API endpoint
  int httpCode = http.POST(data);

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

void getData(const char *server, float *values)
{
  HTTPClient http;
  String url = String(server) + "/api/select/";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();
    StaticJsonDocument<256> doc;
    deserializeJson(doc, payload);

    float minValue = doc["soil_moisture_min"];
    float maxValue = doc["soil_moisture_max"];

    values[0] = minValue;
    values[1] = maxValue;
  }

  http.end();
}
