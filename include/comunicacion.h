#ifndef COMUNICACION_H
  #define COMUNICACION_H
  #include "config.h"


  
struct Activacion {
    int pin;                   // Pin a controlar (IO2 o IO3)
    int tiempoDesactivacion;   // Tiempo en milisegundos
    bool activo;               // Estado actual
    TimerHandle_t timer;       // Timer asociado
};

static Activacion activaciones[2] = {
    {PIN_IO2, 0, false, NULL},
    {PIN_IO3, 0, false, NULL}
};

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