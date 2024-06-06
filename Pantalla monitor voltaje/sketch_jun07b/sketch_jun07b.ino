#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TCA9548A.h>
#include <INA226.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define TCAADDR 0x70 // Dirección del TCA9548A (puede variar según la configuración de los pines A0-A2)
TCA9548A I2CMux;

INA226 INA(0x40);

void setup() {
  Serial.begin(9600);

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
}

void loop() {
  // Leer el voltaje del bus
  float voltage = INA.getBusVoltage();

  // Mostrar el voltaje en la pantalla OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Voltaje: " + String(voltage) + " V");
  Serial.print(voltage);
  display.display();

  // Esperar un segundo antes de la próxima lectura
  delay(500);
}
