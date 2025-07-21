#ifndef COMUNICACION_H
  #define COMUNICACION_H
  #include "config.h"

  class ManejoComunicacion {
    public:
      static void initRFReceiver();
      static void leerRFReceiver();
      static void inicializar();
      static String leerSerial();
      static String leerVecinal();
      static void initI2C();
      static void initUART();
      static int scannerI2C();
      static void escribirVecinal(String envioVecinal);
      static void procesarComando(const String &comandoRecibido, String &respuesta);
      static void envioMsjLoRa(String comandoLoRa);
  };


#endif