// interfaz.h
#ifndef INTERFAZ_H
#define INTERFAZ_H

#include <Arduino.h>
#include <WiFi.h>

//extern AsyncWebServer server;
extern const char* ssidAP;
extern const char* passwordAP;

void iniciarModoProgramacion();
void configurarEndpoints();
void actualizarPantallaAP();  // Nueva función

#endif