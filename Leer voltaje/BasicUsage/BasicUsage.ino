#include <Wire.h>
#include <TCA9548A.h>
#include <INA226.h>

#define TCAADDR 0x70 // Dirección del TCA9548A (puede variar según la configuración de los pines A0-A2)

TCA9548A I2CMux; // Instancia de TCA9548A
INA226 INA(0x40); // Instancia de INA226

void setup() {
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("INA226_LIB_VERSION: ");
  Serial.println(INA226_LIB_VERSION);

  // Inicializar la comunicación I2C
  Wire.begin();

  // Inicializar el multiplexor TCA9548A y configurarlo en el canal 1 (INA226)
  I2CMux.begin(Wire);
  I2CMux.openChannel(1);

  if (!INA.begin()) {
    Serial.println("could not connect. Fix and Reboot");
  }

  INA.setMaxCurrentShunt(1, 0.002);
  INA.setBusVoltageConversionTime(7); // Ajuste opcional de tiempo de conversión de voltaje del bus
}

void loop() {
  // Leer el voltaje del bus y mostrarlo en el monitor serial
  Serial.print(INA.getBusVoltageConversionTime());
  Serial.print("\t");
  Serial.print(INA.getBusVoltage(), 4);
  Serial.println();
  delay(100);
}
