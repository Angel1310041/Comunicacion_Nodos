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

 void tareaDetectarModoProgramacion(void *pvParameters);


void addOrUpdateNetwork(const String& ssid, const String& password);
void deleteNetwork(const String& ssid);
void clearNetworks();
void setPreferredNetwork(const String& ssid);
#endif