#include <Arduino.h>
#include "config.h"
#include "pantalla.h"
#include "heltec.h"
#include "RadioLib.h"

extern SX1262 lora;
extern bool tieneInternet;

void enviarSondeo() {
    String paquete = "SONDEO|CANAL:" + String(configLora.Canal) + "|ID:" + String(configLora.IDLora);
    lora.standby();
    lora.transmit(paquete);
    lora.startReceive();
    Serial.println("Sondeo enviado por canal " + String(configLora.Canal));
}

void responderSondeo(const String& msg) {
    if (msg.startsWith("SONDEO|")) {
        String respuesta = "RESPUESTA|ID:" + String(configLora.IDLora) + "|CANAL:" + String(configLora.Canal);
        lora.standby();
        lora.transmit(respuesta);
        lora.startReceive();
        Serial.println("Respuesta enviada a sondeo.");
    }
}