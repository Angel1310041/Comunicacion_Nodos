#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "heltec.h"


// Pines LoRa
#define LORA_MOSI 10
#define LORA_MISO 11
#define LORA_SCK 9
#define LORA_CS   8
#define LORA_RST  12
#define LORA_DIO1 14
#define LORA_BUSY 13

#define PIN_IO1 2
#define PIN_IO2 3
#define PIN_IO3 4
#define PIN_IO4 5
#define PIN_IO5 6
#define PIN_IO6 7

#define UART_RX 46
#define UART_TX 45

#define I2C_SDA 19
#define I2C_SCL 20

String Version = "1.1.2.2";

SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);

const int nodeID = 3;  // ⚠️ CAMBIA ESTE VALOR EN CADA PLACA

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (lora.begin() != RADIOLIB_ERR_NONE) {
    Serial.println("LoRa init failed!");
    while (true);
  }

  Serial.println("LoRa ready.");
  Serial.println("ID de este nodo: " + String(nodeID));
  Serial.println("Escribe en el formato: ID_DESTINO|mensaje");
  Serial.println("Version:" + Version);
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    int sep = input.indexOf('|');
    if (sep > 0) {
      int destino = input.substring(0, sep).toInt();
      String mensaje = input.substring(sep + 1);

      if (destino != nodeID && mensaje.length() > 0) {
        int siguienteHop = (destino > nodeID) ? nodeID + 1 : nodeID - 1;

        String paquete = "ORIG:" + String(nodeID) +
                         "|DEST:" + String(destino) +
                         "|MSG:" + mensaje +
                         "|HOP:" + String(siguienteHop);

        Serial.println("Enviando a HOP:" + String(siguienteHop));
        int resultado = lora.transmit(paquete);
        if (resultado == RADIOLIB_ERR_NONE) {
          Serial.println("Mensaje enviado correctamente.");
        } else {
          Serial.println("Error al enviar: " + String(resultado));
        }
      } else {
        Serial.println("Destino inválido o mensaje vacío.");
      }
    } else {
      Serial.println("Formato inválido. Usa: 3|Hola");
    }
  }

  String msg;
  int state = lora.receive(msg);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Recibido: " + msg);

    int orig = msg.substring(msg.indexOf("ORIG:") + 5, msg.indexOf("|DEST")).toInt();
    int dest = msg.substring(msg.indexOf("DEST:") + 5, msg.indexOf("|MSG")).toInt();
    String cuerpo = msg.substring(msg.indexOf("MSG:") + 4, msg.indexOf("|HOP"));
    int hop = msg.substring(msg.indexOf("HOP:") + 4).toInt();

    if (nodeID == dest) {
      Serial.println("Mensaje para mí: " + cuerpo);
    } else if (nodeID == hop) {
      int siguienteHop = (dest > nodeID) ? nodeID + 1 : nodeID - 1;

      String nuevoMsg = "ORIG:" + String(orig) + 
                        "|DEST:" + String(dest) + 
                        "|MSG:" + cuerpo + 
                        "|HOP:" + String(siguienteHop);
      delay(1000);
      lora.transmit(nuevoMsg);
      Serial.println("Reenviado a HOP:" + String(siguienteHop));
    } else {
      Serial.println("No es para mí ni me toca reenviar.");
    }
  }

  delay(100);
}
