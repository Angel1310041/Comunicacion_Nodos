#ifndef EEPROM_H
  #define EEPROM_H
  #include "config.h"

struct LoRaConfig {
  uint32_t magic;
  char IDLora[4];
  int Canal;
  bool Pantalla;
  bool displayOn; 
  bool UART;
  bool I2C;
  bool WiFi;
  bool DEBUG;
  char PinesGPIO[7];
  char FlancosGPIO[7];
};

extern LoRaConfig configLora;

  struct Network {
    char ssid[32];
    char password[32];
  };

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

  void guardarConfig();
void cargarConfig();
void borrarConfig();

#endif