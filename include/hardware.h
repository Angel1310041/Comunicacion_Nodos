#ifndef HARDWARE_H
  #define HARDWARE_H
  #include "config.h"

  class Hardware {
    public:
      static void inicializar();
      static void tarjetaNueva();
      static void blinkPin(int pin, int times, int delayTime);
      static void manejoEstrobo(int pin, int freq, int delayTime);
      static void configurarPinesGPIO(char PinesGPIO[6], char Flancos[6]);
      static void manejarComandoPorFuente(const String& fuente);
  };

#endif