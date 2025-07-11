#include "config.h"
#include "eeprom.h"

Preferences eeprom;
const int EEPROM_SIZE = 512;
const int DIRECCION_INICIO_CONFIG = 0;

void ManejoEEPROM::leerTarjetaEEPROM() {
  eeprom.begin("EEPROM_PPAL", false);
  tarjeta.magic = eeprom.getInt("magic");
  String idLeido = eeprom.getString("IDLora");
  strcpy(tarjeta.IDLora, idLeido.c_str());
  tarjeta.Canal = eeprom.getInt("Canal");
  tarjeta.Pantalla = eeprom.getBool("Pantalla");
  tarjeta.UART = eeprom.getBool("UART");
  tarjeta.I2C = eeprom.getBool("I2C");
  tarjeta.WiFi = eeprom.getBool("WiFi");
  String GPIOS = eeprom.getString("PinesGPIO");
  strcpy(tarjeta.PinesGPIO, GPIOS.c_str());
  String FLANCOS = eeprom.getString("FlancosGPIO");
  strcpy(tarjeta.FlancosGPIO, FLANCOS.c_str());
  eeprom.end();
}

void ManejoEEPROM::guardarTarjetaConfigEEPROM() {
  eeprom.begin("EEPROM_PPAL", false);
  eeprom.putInt("magic", tarjeta.magic);
  eeprom.putString("IDLora", tarjeta.IDLora);
  eeprom.putInt("Canal", tarjeta.Canal);
  eeprom.putBool("Pantalla", tarjeta.Pantalla);
  eeprom.putBool("UART", tarjeta.UART);
  eeprom.putBool("I2C", tarjeta.I2C);
  eeprom.putBool("WiFi", tarjeta.WiFi);
  eeprom.putString("PinesGPIO", tarjeta.PinesGPIO);
  eeprom.putString("FlancosGPIO", tarjeta.FlancosGPIO);
  eeprom.end();

  imprimirSerial("Configuracion guardada: ");
  imprimirSerial("ID: " + String(tarjeta.IDLora));
  imprimirSerial("Canal: " + String(tarjeta.Canal));
  imprimirSerial("Pantalla: " + String(tarjeta.Pantalla));
  imprimirSerial("UART: " + String(tarjeta.UART));
  imprimirSerial("I2C: " + String(tarjeta.I2C));

  for (int i = 0; i < 6; ++i) {
    if (tarjeta.PinesGPIO[i] == 1) {
      imprimirSerial("\tPin " + String(pinNames[i]) + " configurado como entrada", 'b');
    } else if (tarjeta.PinesGPIO[i] == 2) {
      imprimirSerial("\tPin " + String(pinNames[i]) + " configurado como salida", 'b');
    } else {
      imprimirSerial("\tPin " + String(pinNames[i]) + " no especificado", 'y');
    }
  }
}

void ManejoEEPROM::tarjetaNueva() {
  ManejoEEPROM::leerTarjetaEEPROM();
  if (tarjeta.magic != 0xDEADBEEF) {
    imprimirSerial("Esta tarjeta es nueva, comenzando formateo...", 'c');
    tarjeta.magic = 0xDEADBEEF;
    strcpy(tarjeta.IDLora, "001");
    tarjeta.Canal = 1;
    tarjeta.Pantalla = false;
    tarjeta.UART = true;
    tarjeta.I2C = false;
    strcpy(tarjeta.PinesGPIO, "IIIIII");
    strcpy(tarjeta.FlancosGPIO, "NNNNNN");

    ManejoEEPROM::guardarTarjetaConfigEEPROM();

    imprimirSerial("\n\t\t\t<<< Tarjeta formateada correctamente >>>", 'g');
    imprimirSerial("Version de la tarjeta: " + Version);
  } else {
    imprimirSerial("\n\t\t\t<<< Tarjeta formateada lista para utilizarse >>>", 'y');
  }
}
