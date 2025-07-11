#include <Arduino.h>
#include "pantalla.h"
#include "heltec.h"

#define ENTRADA_FIJA 1  

String Version = "1.1.1.1";


bool Pantalla = false;

void Pantallas(bool estado) {
    Pantalla = estado;

    pinMode(ENTRADA_FIJA, OUTPUT);
    digitalWrite(ENTRADA_FIJA, Pantalla ? HIGH : LOW);

    if (!Pantalla) {
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(64, 30, "Pantalla apagada");
    Heltec.display->display();
    Serial.println("Pantalla OLED: APAGADA");
    } else {
    Serial.println("Pantalla OLED: ENCENDIDA");
    }
}
