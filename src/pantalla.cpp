#include <Arduino.h>
#include "pantalla.h"
#include "heltec.h"

#define ENTRADA_FIJA 1  

String Version = "1.2.4.2";
bool Pantalla = false;
const int nodeID = 3; // O el valor que corresponda a tu placa

void pantallaControl(String comando, String linea1, String linea2, bool mostrarInfo) {
    static bool Pantalla = false;
    static String Version = "1.1.3.2";
    static const int nodeID = 3;
    #define ENTRADA_FIJA 1

    comando.trim();
    comando.toLowerCase();

    if (comando == "on") {
        Pantalla = true;
        pinMode(ENTRADA_FIJA, OUTPUT);
        digitalWrite(ENTRADA_FIJA, HIGH);
        Heltec.display->displayOn();
        Serial.println("Pantalla OLED: ENCENDIDA");
        // Mostrar mensaje de activación
        Heltec.display->clear();
        Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
        Heltec.display->setFont(ArialMT_Plain_10);
        Heltec.display->drawString(64, 16, "Pantalla activa");
        Heltec.display->display();
        return;
    } else if (comando == "off") {
        Pantalla = false;
        pinMode(ENTRADA_FIJA, OUTPUT);
        digitalWrite(ENTRADA_FIJA, LOW);
        Heltec.display->displayOff();
        Serial.println("Pantalla OLED: APAGADA");
        return;
    } else if (comando == "toggle") {
        pantallaControl(Pantalla ? "off" : "on");
        return;
    } else if (comando.startsWith("show ")) {
        if (Pantalla) {
            String mensaje = comando.substring(5);
            Heltec.display->clear();
            Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
            Heltec.display->setFont(ArialMT_Plain_10);
            Heltec.display->drawString(64, 16, "Comando:");
            Heltec.display->drawString(64, 30, mensaje);
            Heltec.display->display();
            Serial.println("Mostrando: " + mensaje);
        } else {
            Serial.println("Error: Pantalla apagada");
        }
        return;
    } else if (comando != "") {
        Serial.println("Comandos pantalla:");
        Serial.println("on, off, toggle, show <msg>");
        return;
    }

    // Si no hay comando, solo mostrar texto si la pantalla está encendida
    if (Pantalla && (linea1 != "" || linea2 != "")) {
        Heltec.display->clear();
        Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
        Heltec.display->setFont(ArialMT_Plain_10);
        if (mostrarInfo) {
            Heltec.display->drawString(64, 0, "ID: " + String(nodeID) + " | v" + Version);
            Heltec.display->drawHorizontalLine(0, 12, 128);
        }
        Heltec.display->drawString(64, 16, linea1);
        if (linea2 != "") {
            Heltec.display->drawString(64, 30, linea2);
        }
        Heltec.display->display();
    }
}