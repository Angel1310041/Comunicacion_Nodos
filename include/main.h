#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <RCSwitch.h>

// --- Constantes y Variables Globales ---
const int EEPROM_SIZE = 512;

// Estructura actualizada para los datos de la orden
struct ORDEN {
    int id;
    char tipo[30];     // puede ser más largo si necesitas
    char estado[15];   // "Activado", "Desactivado", etc.
};

extern ORDEN ordenActual;  // ✅ declarada como extern, definida en otro archivo
extern boolean debug;
extern boolean modoprog;
extern String Version;

// ¡Aquí es donde declaramos las variables de LoRa como extern!
extern String mensajePendiente;
extern bool enviarLoraPendiente;

// Definiciones de Pines
const int MQ6_PIN = 34;
const int LED_PIN = 25;
const int prog = 26;
const int BOTON_PRUEBA_PIN = 2;

// --- Objetos Globales ---
extern AsyncWebServer server;
extern RCSwitch Transmisorrf;
extern void cargarOrdenDesdeEEPROM();


// --- Declaraciones de Funciones ---
void imprimir(String m, String c = "");
void guardarOrdenEnEEPROM(int id, int tipo, int estado);
void leerOrdenDesdeEEPROM();
void entrarmodoprog();
void playNavidadMelody();
void cargarOrdenDesdeEEPROM();





#endif
