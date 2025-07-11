#ifndef EEPROM_H
  #define EEPROM_H
  #include "config.h"

  struct LoRaMem {
    uint32_t magic;
    char IDLora[3]; // 0 - 9 + A - Z
    int Canal; // 9
    bool Pantalla;
    bool UART;
    bool I2C;
    bool WiFi;
    char PinesGPIO[6];
    char FlancosGPIO[6];
  };

  struct Network {
    char ssid[32];
    char password[32];
  };

  extern LoRaMem tarjeta;
  extern Network redes[3];

  /*
  tarjeta.PinesGPIO -> 'I' = Inhabilitado, 'E' = Entrada, 'S' = Salida
  tarjeta.FlancosGPIO -> 'N' = No asignado, 'A' = ascendente, 'D' = Descendente
  */

  class ManejoEEPROM {
    public:
      // Lectura de la EEPROM
      static void leerTarjetaEEPROM();

      // Escritura de la EEPROM
      static void guardarTarjetaConfigEEPROM();
      static void tarjetaNueva();
  };

#endif