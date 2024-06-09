#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TCA9548A.h>
#include <INA226.h>
#include <ESPAsyncWebServer.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>
#include "esp_task_wdt.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
const int oneWireBus = 4;  // Sensor de temperatura
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
#define TCAADDR 0x70 // Dirección del TCA9548A (puede variar según la configuración de los pines A0-A2)
TCA9548A I2CMux;
INA226 INA(0x40);

const char* ssid_ap = "bateria_config";
const char* password_ap = "p1234566";

AsyncWebServer server(80);

String backend_ip = "";
String backend_port = "8000";  // Valor por defecto
String battery_name = "";
String wifi_ssid = "";
String wifi_password = "";

// Página de configuración HTML
const char* config_page = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8">
  <title>Configuración de la Batería</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f2f2f2;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
    }
    .container {
      background-color: #fff;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
      max-width: 400px;
      width: 100%;
      animation: fadeIn 1s ease-in-out;
    }
    .container h2 {
      text-align: center;
      color: #333;
    }
    .form-group {
      margin-bottom: 15px;
    }
    .form-group label {
      display: block;
      color: #666;
      margin-bottom: 5px;
    }
    .form-group input {
      width: calc(100% - 20px);
      padding: 10px;
      margin-left: 10px;
      border: 1px solid #ddd;
      border-radius: 5px;
      box-sizing: border-box;
    }
    .btn {
      display: block;
      width: 100%;
      padding: 10px;
      background-color: #4CAF50;
      color: #fff;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 16px;
    }
    .btn:hover {
      background-color: #45a049;
    }
    @keyframes fadeIn {
      from { opacity: 0; }
      to { opacity: 1; }
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>Configuración de la Batería</h2>
    <form action="/configure" method="post">
      <div class="form-group">
        <label for="backend_ip">IP del Backend:</label>
        <input type="text" id="backend_ip" name="backend_ip" required>
      </div>
      <div class="form-group">
        <label for="backend_port">Puerto del Backend:</label>
        <input type="text" id="backend_port" name="backend_port" value="8000" required>
      </div>
      <div class="form-group">
        <label for="battery_name">Nombre de la Batería:</label>
        <input type="text" id="battery_name" name="battery_name" required>
      </div>
      <div class="form-group">
        <label for="wifi_ssid">SSID de la WiFi:</label>
        <input type="text" id="wifi_ssid" name="wifi_ssid" required>
      </div>
      <div class="form-group">
        <label for="wifi_password">Contraseña de la WiFi:</label>
        <input type="password" id="wifi_password" name="wifi_password" required>
      </div>
      <input type="submit" value="Guardar" class="btn">
    </form>
  </div>
</body>
</html>)rawliteral";

// Definición del método para leer voltaje, consumo y temperatura
float voltage() {
  // Leer el voltaje del bus
  float v = INA.getBusVoltage();

  // Variables aleatorias de consumo y temperatura
  float consumption = random(50, 500) / 100.0;
  float temperature = readTemperature();

  // Mostrar los datos en la pantalla OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Voltaje: " + String(v) + " V");
  display.println("Consumo: " + String(consumption) + " A");
  display.println("Temperatura: " + String(temperature) + " C");
  display.display();
  
  // Esperar un segundo antes de la próxima lectura
  delay(500);
  return v;
}

void connectToWiFi() {
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a la red WiFi...");
    if (++retry_count > 40) { // Intentar conectarse por 20 segundos
      Serial.println("Error al conectar a la red WiFi. Reiniciando...");
      ESP.restart();
    }
  }
  Serial.println("Conectado a la red WiFi");
}

void sendDataToBackend() {
  // Enviar datos aleatorios al backend
  float consumption = random(50, 500) / 100.0;
  float v = voltage();
  float temp = readTemperature();

  // Formatear los datos en formato JSON
  String json_data = "{\"consumption\": " + String(consumption) + ", \"voltage\": " + String(v) + ", \"temperature\": " + String(temp) + "}";

  // Realizar la solicitud HTTP POST al backend
  HTTPClient http;
  String url = "http://" + backend_ip + ":" + backend_port + "/api/battery/update/" + battery_name + "/"; // Construir la URL completa

  Serial.println("Enviando datos al servidor: " + json_data);
  Serial.println("URL: " + url);

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(json_data);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Respuesta del servidor: " + response);
  } else {
    Serial.println("Error al enviar la solicitud: " + String(httpResponseCode));
  }

  http.end();

  // Alimentar al watchdog
  esp_task_wdt_reset();
}

void setup() {
  Serial.begin(115200);
  
  // Inicializar la comunicación I2C
  Wire.begin();

  // Inicializar el multiplexor TCA9548A para el canal 0 (pantalla OLED)
  I2CMux.begin(Wire);
  I2CMux.openChannel(0);

  // Inicializar la pantalla OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  // Esperar un momento para que se inicialice la pantalla
  delay(2000);

  // Borrar la pantalla
  display.clearDisplay();
  display.display();
  // INA226 en el pin1 y pantalla en el 0
  // Inicializar el INA226 en el canal 1 del multiplexor
  I2CMux.openChannel(1);
  if (!INA.begin()) {
    Serial.println("could not connect. Fix and Reboot");
  }
  
  INA.setMaxCurrentShunt(1, 0.002);
  INA.setBusVoltageConversionTime(7); // Ajuste opcional de tiempo de conversión de voltaje del bus
  
  // Iniciar el punto de acceso
  if (!WiFi.softAP(ssid_ap, password_ap)) {
    Serial.println("Error al iniciar el punto de acceso");
    return;
  }
  
  Serial.println("Punto de acceso iniciado");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Configurar el servidor web para la configuración inicial
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", config_page);
  });

  server.on("/configure", HTTP_POST, [](AsyncWebServerRequest *request){
    int params = request->params();
    for(int i = 0; i < params; i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->name() == "backend_ip"){
        backend_ip = p->value().c_str();
      }
      if(p->name() == "backend_port"){
        backend_port = p->value().c_str();
      }
      if(p->name() == "battery_name"){
        battery_name = p->value().c_str();
      }
      if(p->name() == "wifi_ssid"){
        wifi_ssid = p->value().c_str();
      }
      if(p->name() == "wifi_password"){
        wifi_password = p->value().c_str();
      }
    }
    request->send(200, "text/html", "Configuración recibida. Conectando a WiFi...");
    delay(3000);
    connectToWiFi();
  });

  server.begin();
}

void loop() {
  if (wifi_ssid != "" && wifi_password != "" && backend_ip != "" && backend_port != "" && battery_name != "") {
    // Asegurarse de que solo intente conectarse si no está ya conectado
    if (WiFi.status() != WL_CONNECTED) {
      connectToWiFi();
    } else {
      sendDataToBackend();
    }
  }

  // Alimentar al watchdog en el bucle principal
  esp_task_wdt_reset();
  delay(2000); // Esperar 2 segundos antes de enviar la próxima solicitud
}

float readTemperature() {
  // Solicitar al sensor DS18B20 leer la temperatura
  sensors.requestTemperatures();
  // Leer y devolver la temperatura en grados Celsius
  return sensors.getTempCByIndex(0); 
}
