#ifndef PANTALLA_H
#define PANTALLA_H

#include <Arduino.h>

// Solo aquí los argumentos por defecto
void pantallaControl(String comando = "", String linea1 = "", String linea2 = "", bool mostrarInfo = true);
extern const int nodeID;
#endif