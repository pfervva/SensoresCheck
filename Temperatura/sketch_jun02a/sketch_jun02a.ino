#include <WiFi.h>
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = "Iphone_Pfer";
const char* password = "p1234566";

const char* backend_url = "http://172.20.10.14:8000/api/battery/update/";
const int oneWireBus = 4;  // Sensor de temperatura

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a la red WiFi...");
  }
  Serial.println("Conectado a la red WiFi");

  // Inicializar el generador de números aleatorios
  randomSeed(analogRead(0));
}

void loop() {
  float consumption = readConsumption(); // Leer el consumo de los sensores
  float voltage = readVoltage(); // Leer el voltaje
  float temperature = readTemperature(); // Leer la temperatura

  // Formatear los datos en formato JSON
  String json_data = "{\"consumption\": " + String(consumption) + ", \"voltage\": " + String(voltage) + ", \"temperature\": " + String(temperature) + "}";
  
  // Realizar la solicitud HTTP POST al backend
  HTTPClient http;
  http.begin(backend_url);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(json_data);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Respuesta del servidor: " + response);
  } else {
    Serial.println("Error al enviar la solicitud");
  }
  
  http.end();
  
  delay(1000); // Esperar 5 segundos antes de enviar la próxima solicitud
}

float readConsumption() {
  // Generar un valor aleatorio de consumo entre 0.5 y 5.0 amperios
  return random(50, 500) / 100.0;
}

float readVoltage() {
  // Generar un valor aleatorio de voltaje entre 11.2 y 16.8 voltios
  return random(112, 168) / 10.0;
}

float readTemperature() {
// Solicitar al sensor DS18B20 leer la temperatura
  sensors.requestTemperatures();
  // Leer y devolver la temperatura en grados Celsius
  return sensors.getTempCByIndex(0); 
}
