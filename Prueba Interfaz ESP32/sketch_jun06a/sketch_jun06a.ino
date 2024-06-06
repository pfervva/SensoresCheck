#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include "esp_task_wdt.h"

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

void connectToWiFi() {
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a la red WiFi...");
    if (++retry_count > 20) { // Intentar conectarse por 20 segundos
      Serial.println("Error al conectar a la red WiFi. Reiniciando...");
      ESP.restart();
    }
  }
  Serial.println("Conectado a la red WiFi");
}

void sendDataToBackend() {
  // Enviar datos aleatorios al backend
  float consumption = random(50, 500) / 100.0;
  float voltage = random(112, 168) / 10.0;
  float temperature = random(200, 400) / 10.0;

  // Formatear los datos en formato JSON
  String json_data = "{\"consumption\": " + String(consumption) + ", \"voltage\": " + String(voltage) + ", \"temperature\": " + String(temperature) + "}";

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
