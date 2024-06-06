#include <WiFi.h>
#include <WebServer.h>

const char *ssid = "MiRedWiFi";
const char *password = "MiClaveWiFi";

WebServer server(80);

// Pin para controlar el relé
const int relayPin = 33;

void setup() {
  Serial.begin(115200);
  
  // Configurar el pin del relé como salida
  pinMode(relayPin, OUTPUT);
  
  // Iniciar conexión WiFi
  WiFi.softAP(ssid, password);
  
  // Configurar rutas del servidor web
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Control del Relé</title>
<style>
body {
  font-family: Arial, sans-serif;
  text-align: center;
}
.container {
  margin-top: 50px;
}
.button {
  padding: 10px 20px;
  font-size: 20px;
  border: none;
  border-radius: 5px;
  cursor: pointer;
}
.button-on {
  background-color: #4CAF50;
  color: white;
}
.button-off {
  background-color: #f44336;
  color: white;
}
.status {
  margin-top: 20px;
  font-size: 24px;
}
</style>
</head>
<body>
<div class="container">
  <h1>Control del Relé</h1>
  <button id="btn-on" class="button button-on" onclick="toggleRelay(true)">Encender</button>
  <button id="btn-off" class="button button-off" onclick="toggleRelay(false)">Apagar</button>
  <p class="status" id="status">Estado: Apagado</p>
</div>
<script>
function toggleRelay(turnOn) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      updateStatus(turnOn);
    }
  };
  if (turnOn) {
    xhttp.open("GET", "/on", true);
  } else {
    xhttp.open("GET", "/off", true);
  }
  xhttp.send();
}
function updateStatus(turnOn) {
  var statusElement = document.getElementById("status");
  if (turnOn) {
    statusElement.innerHTML = "Estado: Encendido";
  } else {
    statusElement.innerHTML = "Estado: Apagado";
  }
}
</script>
</body>
</html>
    )rawliteral");
  });

  server.on("/on", HTTP_GET, []() {
    digitalWrite(relayPin, HIGH); // Encender el relé
    server.send(200, "text/plain", "Relé encendido");
  });

  server.on("/off", HTTP_GET, []() {
    digitalWrite(relayPin, LOW); // Apagar el relé
    server.send(200, "text/plain", "Relé apagado");
  });

  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  server.handleClient();
}
