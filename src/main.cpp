#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "heltec.h"
#include "config.h"

SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);

String Version = "1.1.2.4";
volatile bool receivedFlag = false;
#define MSG_ID_BUFFER_SIZE 16
String msgIdBuffer[MSG_ID_BUFFER_SIZE];
int msgIdBufferIndex = 0;

String generarMsgID() {
  return String(configLora.IDLora) + "-" + String(millis());
}

bool esMsgDuplicado(const String& msgId) {
  for (int i = 0; i < MSG_ID_BUFFER_SIZE; i++) {
    if (msgIdBuffer[i] == msgId) return true;
  }
  return false;
}

void guardarMsgID(const String& msgId) {
  msgIdBuffer[msgIdBufferIndex] = msgId;
  msgIdBufferIndex = (msgIdBufferIndex + 1) % MSG_ID_BUFFER_SIZE;
}

void setFlag() {
  receivedFlag = true;
}

void setFlag(void) {
  receivedFlag = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  cargarConfig();

  if (configLora.IDLora < 0) {
    pedirID();
    guardarConfig();
  } else {
    Serial.println("ID cargado de memoria: " + String(configLora.IDLora));
  }

  if (configLora.Canal < 0 || configLora.Canal > 8) {
    pedirCanal();
    guardarConfig();
  } else {
    Serial.println("Canal cargado de memoria: " + String(configLora.Canal) + " (" + String(canales[configLora.Canal], 1) + " MHz)");
  }

  if (lora.begin(canales[configLora.Canal]) != RADIOLIB_ERR_NONE) {
    Serial.println("LoRa init failed!");
    while (true);
  }

  lora.setDio1Action(setFlag);

  lora.startReceive();

  pinMode(LED_PIN, OUTPUT);

  Serial.println("LoRa ready.");
  Serial.println("ID de este nodo: " + String(configLora.IDLora));
  Serial.println("Canal: " + String(configLora.Canal) + " (" + String(canales[configLora.Canal], 1) + " MHz)");
  Serial.println("Escribe en el formato: ID_DESTINO|mensaje");
  Serial.println("Para borrar la configuración y reconfigurar, escribe: RESET");
  Serial.println("Version:" + Version);
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("RESET")) {
      borrarConfig();
      return;
    }

    int sep = input.indexOf('|');
    if (sep > 0) {
      int destino = input.substring(0, sep).toInt();
      String mensaje = input.substring(sep + 1);

      if (destino != configLora.IDLora && mensaje.length() > 0) {
        int siguienteHop = (destino > configLora.IDLora) ? configLora.IDLora + 1 : configLora.IDLora - 1;

        String msgID = generarMsgID();

        String paquete = "ORIG:" + String(configLora.IDLora) +
                         "|DEST:" + String(destino) +
                         "|MSG:" + mensaje +
                         "|HOP:" + String(siguienteHop) +
                         "|CANAL:" + String(configLora.Canal) +
                         "|ID:" + msgID;

        Serial.println("Enviando a HOP:" + String(siguienteHop) + " por canal " + String(configLora.Canal));

        lora.standby();
        int resultado = lora.transmit(paquete);

        if (resultado == RADIOLIB_ERR_NONE) {
          Serial.println("Mensaje enviado correctamente.");
          digitalWrite(LED_PIN, HIGH);
          delay(100);
          digitalWrite(LED_PIN, LOW);
          guardarMsgID(msgID);
        } else {
          Serial.println("Error al enviar: " + String(resultado));
        }

        lora.startReceive();
      } else {
        Serial.println("Destino inválido o mensaje vacío.");
      }
    } else {
      Serial.println("Formato inválido. Usa: 3|Hola");
    }
  }

  if (receivedFlag) {
    receivedFlag = false;
    String msg;
    int state = lora.readData(msg);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("Recibido: " + msg);

      int canalMsg = -1;
      int idxCanal = msg.indexOf("|CANAL:");
      int idxID = msg.indexOf("|ID:");
      if (idxCanal != -1) {
        canalMsg = msg.substring(idxCanal + 7, idxID == -1 ? msg.length() : idxID).toInt();
      }

      String msgID = "";
      if (idxID != -1) {
        msgID = msg.substring(idxID + 4);
      }

      if (canalMsg != -1 && canalMsg != configLora.Canal) {
        Serial.println("Mensaje recibido de otro canal (" + String(canalMsg) + "), ignorado.");
        lora.startReceive();
        return;
      }

      if (msgID.length() > 0 && esMsgDuplicado(msgID)) {
        Serial.println("Mensaje duplicado, ignorado.");
        lora.startReceive();
        return;
      }
      if (msgID.length() > 0) {
        guardarMsgID(msgID);
      }

      int orig = msg.substring(msg.indexOf("ORIG:") + 5, msg.indexOf("|DEST")).toInt();
      int dest = msg.substring(msg.indexOf("DEST:") + 5, msg.indexOf("|MSG")).toInt();
      String cuerpo = msg.substring(msg.indexOf("MSG:") + 4, msg.indexOf("|HOP"));
      int hop = msg.substring(msg.indexOf("HOP:") + 4, idxCanal == -1 ? msg.length() : idxCanal).toInt();

      if (configLora.IDLora == dest) {
        Serial.println("Mensaje para mí: " + cuerpo);
        digitalWrite(LED_PIN, HIGH); 
        delay(100);
        digitalWrite(LED_PIN, LOW);
      } else if (configLora.IDLora == hop) {
        int siguienteHop = (dest > configLora.IDLora) ? configLora.IDLora + 1 : configLora.IDLora - 1;

        String nuevoMsg = "ORIG:" + String(orig) +
                          "|DEST:" + String(dest) +
                          "|MSG:" + cuerpo +
                          "|HOP:" + String(siguienteHop) +
                          "|CANAL:" + String(configLora.Canal) +
                          "|ID:" + msgID;

        delay(100); 
        lora.standby();
        lora.transmit(nuevoMsg);
        Serial.println("Reenviado a HOP:" + String(siguienteHop) + " por canal " + String(configLora.Canal));
        lora.startReceive();
      } else {
        Serial.println("No es para mí ni me toca reenviar.");
      }
    } else {
      Serial.println("Error al recibir: " + String(state));
    }
    lora.startReceive();
  }
}