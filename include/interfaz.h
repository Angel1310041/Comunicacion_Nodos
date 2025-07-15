#ifndef INTERFAZ_H
  #define INTERFAZ_H

  #include "config.h"
  #include "hardware.h"
  #include "eeprom.h"
  #include "comunicacion.h"

  class Interfaz {
    public:
      static void entrarModoProgramacion();
  };

  void endpointsMProg(void *pvParameters);
#endif