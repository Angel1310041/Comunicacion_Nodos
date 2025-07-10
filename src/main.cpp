#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "heltec.h"
#include "config.h"

SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);

String Version = "1.1.2.5";
volatile bool receivedFlag = false;
#define MSG_ID_BUFFER_SIZE 16
String msgIdBuffer[MSG_ID_BUFFER_SIZE];
int msgIdBufferIndex = 0;

// Genera un ID de mensaje único usando el IDLora (string) y millis
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

void setup() {
  Serial.begin(115200);
  delay(1000);

  cargarConfig();

  if (strlen(configLora.IDLora) == 0) {
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
      String destino = input.substring(0, sep);
      String mensaje = input.substring(sep + 1);

      // Compara destino (string) con configLora.IDLora (char[])
      if (strcmp(destino.c_str(), configLora.IDLora) != 0 && mensaje.length() > 0) {
        // Para IDs alfanuméricos, siguienteHop será igual al destino (o puedes implementar lógica de routing avanzada)
        String siguienteHop = destino;

        String msgID = generarMsgID();

        String paquete = "ORIG:" + String(configLora.IDLora) +
                         "|DEST:" + destino +
                         "|MSG:" + mensaje +
                         "|HOP:" + siguienteHop +
                         "|CANAL:" + String(configLora.Canal) +
                         "|ID:" + msgID;

        Serial.println("Enviando a HOP:" + siguienteHop + " por canal " + String(configLora.Canal));

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

      // Extraer ORIG, DEST, HOP como string
      int idxOrig = msg.indexOf("ORIG:");
      int idxDest = msg.indexOf("|DEST:");
      int idxMsg = msg.indexOf("|MSG:");
      int idxHop = msg.indexOf("|HOP:");

      String orig = msg.substring(idxOrig + 5, idxDest);
      String dest = msg.substring(idxDest + 6, idxMsg);
      String cuerpo = msg.substring(idxMsg + 5, idxHop);
      String hop = msg.substring(idxHop + 5, idxCanal == -1 ? msg.length() : idxCanal);

      // Compara si el mensaje es para mí
      if (strcmp(dest.c_str(), configLora.IDLora) == 0) {
        Serial.println("Mensaje para mí: " + cuerpo);
        digitalWrite(LED_PIN, HIGH); 
        delay(100);
        digitalWrite(LED_PIN, LOW);
      } else if (strcmp(hop.c_str(), configLora.IDLora) == 0) {
        // Para IDs alfanuméricos, siguienteHop será igual al destino (o puedes implementar lógica de routing avanzada)
        String siguienteHop = dest;

        String nuevoMsg = "ORIG:" + orig +
                          "|DEST:" + dest +
                          "|MSG:" + cuerpo +
                          "|HOP:" + siguienteHop +
                          "|CANAL:" + String(configLora.Canal) +
                          "|ID:" + msgID;

        delay(100); 
        lora.standby();
        lora.transmit(nuevoMsg);
        Serial.println("Reenviado a HOP:" + siguienteHop + " por canal " + String(configLora.Canal));
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