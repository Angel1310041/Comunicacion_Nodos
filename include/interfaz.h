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
  //De define para el manejo del modo programacion por boton fisico
void modoprogporbotonfisico();
void servidorModoProgramacion();
void iniciarServidorConfiguracion(void *pvParameters);
void configurarServidorAPI();
void mostrarMensaje(const String& titulo, const String& mensaje, int delayMs);
void configurarDisplay(bool estado);


// Declaraciones de funciones API REST
void apiObtenerConfiguracion();
void apiGuardarConfiguracion();
void apiEjecutarComando();
void apiBuscarDispositivos();
void apiObtenerEstado();
void apiIntercambiarDatos();

// Función para registrar todos los endpoints en el servidor
void registrarEndpointsAPI();

// Función auxiliar para obtener el tipo de encriptación como String
String getEncryptionType(wifi_auth_mode_t encryptionType);

#endif