#ifndef COMUNICACION_H
  #define COMUNICACION_H
  #include "config.h"

  class ManejoComunicacion {
    public:
      static void inicializar();
      static String leerSerial();
      static String leerVecinal();
      static void initI2C();
      static void initUART();
      static void scannerI2C();
      static void escribirVecinal(String envioVecinal);
      static void procesarComando(String comandoRecibido);
      static void envioMsjLoRa(String comandoLoRa);
  };

#endif