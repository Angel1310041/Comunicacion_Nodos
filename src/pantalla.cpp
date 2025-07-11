#include <Arduino.h>
#include "pantalla.h"

#define ENTRADA_FIJA 1  

String Version = "1.1.3.2";
bool Pantalla = false;

void Pantallas(bool estado) {
Pantalla = estado;

pinMode(ENTRADA_FIJA, OUTPUT);
digitalWrite(ENTRADA_FIJA, Pantalla ? HIGH : LOW);

if (!Pantalla) {
Serial.println("Pantalla OLED: APAGADA");
} else {
Serial.println("Pantalla OLED: ENCENDIDA");
}
}
