#include "config.h"
#include "hardware.h"
#include "eeprom.h"
#include "comunicacion.h"
#include "interfaz.h"

String Version = "1.4.1.5";

SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);

String mensaje = "";
bool modoProgramacion = false;

TaskHandle_t tareaComandosSerial;

void imprimirSerial(String mensaje, char color) {
  String colorCode;
  switch (color) {
    case 'r': colorCode = "\033[31m"; break; // Rojo
    case 'g': colorCode = "\033[32m"; break; // Verde
    case 'b': colorCode = "\033[34m"; break; // Azul
    case 'y': colorCode = "\033[33m"; break; // Amarillo
    case 'c': colorCode = "\033[36m"; break; // Cian
    case 'm': colorCode = "\033[35m"; break; // Magenta
    case 'w': colorCode = "\033[37m"; break; // Blanco
    default: colorCode = "\033[0m"; // Sin color
  }
  Serial.print(colorCode);
  Serial.println(mensaje);
  Serial.print("\033[0m"); // Resetear color
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  if (lora.begin() != RADIOLIB_ERR_NONE) {
    imprimirSerial("Inicializacion de LoRa fallida!", 'r');
    while (true);
  }

  Hardware::inicializar();
  ManejoComunicacion::inicializar();

  TaskHandle_t tareaComandosSerial = NULL;

  imprimirSerial("Lora iniciada correctamente\n", 'g');
  imprimirSerial("ID de este nodo: " + String(tarjeta.IDLora));
  imprimirSerial("Version -> " + Version);
}

void recibirComandoSerial(void *pvParameters) {
  imprimirSerial("Esperando comandos por Serial...");
  tareaComandosSerial = xTaskGetCurrentTaskHandle();
  String comandoSerial = "";

  while (true) {
    comandoSerial = ManejoComunicacion::leerSerial();
    if (!comandoSerial.isEmpty()) {
      ManejoComunicacion::procesarComando(comandoSerial);
      ultimoComandoRecibido = comandoSerial;
    } else if (comandoSerial == ultimoComandoRecibido) {
      comandoSerial = "";
    }
    esp_task_wdt_reset();
    vTaskDelay(5000);
  }
  vTaskDelete(NULL);
}

void loop() {
  if (!modoProgramacion && tareaComandosSerial == NULL && tarjeta.DEBUG) {
    imprimirSerial("Iniciando tarea de recepcion de comandos Seriales...", 'c');
    xTaskCreatePinnedToCore(
      recibirComandoSerial,
      "Comandos Seriales",
      5120,
      NULL,
      1,
      &tareaComandosSerial,
      0
    );
    imprimirSerial("Tarea de recepcion de comandos Seriales iniciada", 'c');
  }

  /*
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
  */

  delay(100);
}
