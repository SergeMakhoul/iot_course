#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN A0
#define DHTTYPE DHT11

DHT_Unified dht(DHTPIN, DHTTYPE);

#define LightPin A1
#define PHPin A2

///////////////////////////////////////////
///////////////////////////////////////////

void printTemp(){
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(String(event.temperature));
    Serial.println(F("°C"));
  }
}

void printHumidity(){
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(String(event.relative_humidity));
    Serial.println(F("%"));
  }
}

void printLight(){
  float light = analogRead(LightPin);
  Serial.print(F("Light: "));
  Serial.print(100 * (1 - light / 1024));
  Serial.println(F("%"));
}

void printPH(){
  float ph = analogRead(PHPin);
  //ph = 3.94 + ph * 5.0 * 3.5 / 1024;
  Serial.print(F("PH: "));
  Serial.println(ph);
}

///////////////////////////////////////////
///////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  pinMode(LightPin, INPUT);
  pinMode(PHPin, INPUT);
  
  Serial.println("started\n");
  
  // Initialize dht temp/humidity device.
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  dht.humidity().getSensor(&sensor);

  delay(1000);
}

void loop() {

  printLight();
  printTemp();
  printHumidity();
  printPH();
  
  delay(10000);

}
