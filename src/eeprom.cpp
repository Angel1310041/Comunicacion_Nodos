#include <Arduino.h>
#include "eeprom.h"


Preferences eeprom;
const int EEPROM_SIZE = 512;
const int DIRECCION_INICIO_CONFIG = 0;


void ManejoEEPROM::leerTarjetaEEPROM() {
  eeprom.begin("EEPROM_PPAL", false);
  configLora.magic = eeprom.getInt("magic");
  String idLeido = eeprom.getString("IDLora");
  strcpy(configLora.IDLora, idLeido.c_str());
  configLora.Canal = eeprom.getInt("Canal");
  configLora.Pantalla = eeprom.getBool("Pantalla");
  configLora.UART = eeprom.getBool("UART");
  configLora.I2C = eeprom.getBool("I2C");
  configLora.WiFi = eeprom.getBool("WiFi");
  configLora.DEBUG = eeprom.getBool("DEBUG");
  String GPIOS = eeprom.getString("PinesGPIO");
  strcpy(configLora.PinesGPIO, GPIOS.c_str());
  String FLANCOS = eeprom.getString("FlancosGPIO");
  strcpy(configLora.FlancosGPIO, FLANCOS.c_str());
  eeprom.end();
}

void ManejoEEPROM::guardarTarjetaConfigEEPROM() {
  eeprom.begin("EEPROM_PPAL", false);
  eeprom.putInt("magic", configLora.magic);
  eeprom.putString("IDLora", configLora.IDLora);
  eeprom.putInt("Canal", configLora.Canal);
  eeprom.putBool("Pantalla", configLora.Pantalla);
  eeprom.putBool("UART", configLora.UART);
  eeprom.putBool("I2C", configLora.I2C);
  eeprom.putBool("WiFi", configLora.WiFi);
  eeprom.putBool("DEBUG", configLora.DEBUG);
  eeprom.putString("PinesGPIO", configLora.PinesGPIO);
  eeprom.putString("FlancosGPIO", configLora.FlancosGPIO);
  eeprom.end();

  imprimirSerial("Configuracion guardada: ");
  imprimirSerial("ID: " + String(configLora.IDLora));
  imprimirSerial("Canal: " + String(configLora.Canal));
  imprimirSerial("Pantalla: " + String(configLora.Pantalla));
  imprimirSerial("UART: " + String(configLora.UART));
  imprimirSerial("DEBUG: " + String(configLora.DEBUG));
  imprimirSerial("I2C: " + String(configLora.I2C));

  for (int i = 0; i < 6; ++i) {
    if (configLora.PinesGPIO[i] == 1) {
      imprimirSerial("o- Pin " + String(pinNames[i]) + " configurado como entrada", 'c');
    } else if (configLora.PinesGPIO[i] == 2) {
      imprimirSerial("o- Pin " + String(pinNames[i]) + " configurado como salida", 'c');
    } else {
      imprimirSerial("o- Pin " + String(pinNames[i]) + " no especificado", 'y');
    }
  }
}

void ManejoEEPROM::borrarTarjetaConfigEEPROM() {
    eeprom.begin("EEPROM_PPAL", false);
    eeprom.clear();
    eeprom.end();
    Serial.println("Configuración borrada. Reinicia el dispositivo o espera...");
    delay(1000);
    ESP.restart();
}

void ManejoEEPROM::tarjetaNueva() {
  ManejoEEPROM::leerTarjetaEEPROM();
  if (configLora.magic != 0xDEADBEEF) {
    imprimirSerial("Esta tarjeta es nueva, comenzando formateo...", 'c');
    configLora.magic = 0xDEADBEEF;
    strcpy(configLora.IDLora, "001");
    configLora.Canal = 0;
    configLora.Pantalla = false;
    configLora.UART = true;
    configLora.I2C = false;
    configLora.DEBUG = true;
    //strcpy(configLora.PinesGPIO, "IIIIII");
    //strcpy(configLora.FlancosGPIO, "NNNNNN");

    ManejoEEPROM::guardarTarjetaConfigEEPROM();

    imprimirSerial("\n\t\t\t<<< Tarjeta formateada correctamente >>>", 'g');
    imprimirSerial("Version de la tarjeta: " + Version);
  } else {
    imprimirSerial("\n\t\t\t<<< Tarjeta lista para utilizarse >>>", 'y');
  }
}
