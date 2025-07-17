#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "heltec.h"
#include "config.h"
#include "pantalla.h"
#include "comunicacion.h"
#include "eeprom.h"
#include "hardware.h"

SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
String Version = "2.1.1.1";
volatile bool receivedFlag = false;
bool modoProgramacion = false;

TaskHandle_t tareaComandosSerial = NULL;
TaskHandle_t tareaComandosVecinal = NULL;
TaskHandle_t tareaComandosLoRa = NULL;

extern String ultimoComandoRecibido; 
#define MSG_ID_BUFFER_SIZE 16
String msgIdBuffer[MSG_ID_BUFFER_SIZE];
int msgIdBufferIndex = 0;

#define MAX_NODOS_ACTIVOS 32
String nodosActivos[MAX_NODOS_ACTIVOS];
int numNodosActivos = 0;
String mensaje = "";

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

void agregarNodoActivo(const String& id) {
    for (int i = 0; i < numNodosActivos; i++)
        if (nodosActivos[i] == id) return;
    if (numNodosActivos < MAX_NODOS_ACTIVOS)
        nodosActivos[numNodosActivos++] = id;
}

void limpiarNodosActivos() { numNodosActivos = 0; }

void mostrarNodosActivos() {
    Serial.println("Nodos activos detectados:");
    for (int i = 0; i < numNodosActivos; i++)
        Serial.println(" - " + nodosActivos[i]);
}

String generarMsgID() { return String(configLora.IDLora) + "-" + String(millis()); }

bool esMsgDuplicado(const String& msgId) {
    for (int i = 0; i < MSG_ID_BUFFER_SIZE; i++)
        if (msgIdBuffer[i] == msgId) return true;
    return false;
}

void guardarMsgID(const String& msgId) {
    msgIdBuffer[msgIdBufferIndex] = msgId;
    msgIdBufferIndex = (msgIdBufferIndex + 1) % MSG_ID_BUFFER_SIZE;
}

void setFlag() { receivedFlag = true; }

void enviarComandoEstructurado(const String& destino, char red, const String& comando) {
    int numAzar = random(10, 99);
    String msg = destino + "@" + red + "@" + comando + "@" + String(numAzar);

    if (strcmp(destino.c_str(), configLora.IDLora) != 0 && comando.length() > 0) {
        String siguienteHop = destino, msgID = generarMsgID();
        char paquete[256];
        snprintf(paquete, sizeof(paquete), "ORIG:%s|DEST:%s|MSG:%s|HOP:%s|CANAL:%d|ID:%s",
            configLora.IDLora, destino.c_str(), msg.c_str(), siguienteHop.c_str(), configLora.Canal, msgID.c_str());
        imprimirSerial("Enviando comando estructurado a HOP:" + siguienteHop + " por canal " + String(configLora.Canal), 'c');
        mostrarMensaje("Enviando...", "A HOP: " + siguienteHop, 0);
        lora.standby();
        int resultado = lora.transmit(paquete);
        if (resultado == RADIOLIB_ERR_NONE) {
            imprimirSerial("Comando enviado correctamente.", 'g');
            Hardware::manejarComandoPorFuente("lora");
            guardarMsgID(msgID); mostrarMensajeEnviado(destino, msg);
        } else {
            imprimirSerial("Error al enviar: " + String(resultado), 'r');
            mostrarError("Error al enviar: " + String(resultado));
        }
        lora.startReceive();
    } else {
        imprimirSerial("Destino inválido o comando vacío.", 'y');
        mostrarInfo("Destino invalido o comando vacio.");
    }
}

void recibirComandoSerial(void *pvParameters) {
    imprimirSerial("Esperando comandos por Serial...", 'b');
    tareaComandosSerial = xTaskGetCurrentTaskHandle();
    String comandoSerial = "";
    String respuesta;
    while (true) {
        comandoSerial = ManejoComunicacion::leerSerial();
        comandoSerial.trim();

        if (!comandoSerial.isEmpty()) {
            imprimirSerial("Comando recibido por Serial: " + comandoSerial, 'y');
            Hardware::manejarComandoPorFuente("serial");
            int idx1 = comandoSerial.indexOf('@');
            int idx2 = comandoSerial.indexOf('@', idx1 + 1);
            int idx3 = comandoSerial.indexOf('@', idx2 + 1);
            if (idx1 > 0 && idx2 > idx1 && idx3 > idx2) {
                String destino = comandoSerial.substring(0, idx1);
                char red = comandoSerial.charAt(idx1 + 1);
                String comando = comandoSerial.substring(idx2 + 1, idx3);
                enviarComandoEstructurado(destino, red, comando);
                ManejoComunicacion::procesarComando(comandoSerial, respuesta);
                if (!respuesta.isEmpty()) {
                    imprimirSerial("Respuesta: " + respuesta, 'g');
                }
            } else {
                imprimirSerial("Formato inválido. Usa: ID@R@CMD@##", 'r');
                mostrarInfo("Formato invalido. Usa: ID@R@CMD@##");
            }
            ultimoComandoRecibido = comandoSerial;
        }
        esp_task_wdt_reset();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void recibirComandosLoRa(void *pvParameters) {
    imprimirSerial("Esperando comandos por LoRa...", 'b');
    tareaComandosLoRa = xTaskGetCurrentTaskHandle();
    String msg;

    while (true) {
        if (receivedFlag) {
            receivedFlag = false;
            int state = lora.readData(msg);
            Hardware::manejarComandoPorFuente("lora");
            if (state == RADIOLIB_ERR_NONE) {
                imprimirSerial("Comando recibido por LoRa: " + msg, 'c');
                int idxMsg = msg.indexOf("MSG:");
                int idxPipe = msg.indexOf("|", idxMsg);
                if (idxMsg != -1) {
                    String comandoRecibido;
                    String respuesta;
                    if (idxPipe != -1)
                        comandoRecibido = msg.substring(idxMsg + 4, idxPipe);
                    else
                        comandoRecibido = msg.substring(idxMsg + 4);
                    comandoRecibido.trim();
                    int idx1 = comandoRecibido.indexOf('@');
                    int idx2 = comandoRecibido.indexOf('@', idx1 + 1);
                    int idx3 = comandoRecibido.indexOf('@', idx2 + 1);
                    if (idx1 > 0 && idx2 > idx1 && idx3 > idx2) {
                        ManejoComunicacion::procesarComando(comandoRecibido, respuesta);
                        ultimoComandoRecibido = comandoRecibido;
                    } else {
                        imprimirSerial("Mensaje LoRa recibido no es un comando válido: " + comandoRecibido, 'y');
                    }
                } else {
                    imprimirSerial("Paquete LoRa recibido sin campo MSG: " + msg, 'y');
                }
            }
            lora.startReceive();
            mostrarEstadoLoRa(String(configLora.IDLora), String(configLora.Canal), Version);
        }
        esp_task_wdt_reset();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void recibirComandosVecinal(void *pvParameters) {
  imprimirSerial("Esperando comandos por UART (Vecinal)...", 'b');
  tareaComandosVecinal = xTaskGetCurrentTaskHandle();
  String comandoVecinal = "";
  String respuesta;
  while (true) {
    comandoVecinal = ManejoComunicacion::leerVecinal();
    if (!comandoVecinal.isEmpty()) {
      ManejoComunicacion::procesarComando(comandoVecinal, respuesta);
      ultimoComandoRecibido = comandoVecinal;
      if (!respuesta.isEmpty()) {
        imprimirSerial("Respuesta: " + respuesta, 'g');
      }
    } else if (comandoVecinal == ultimoComandoRecibido) {
      comandoVecinal = "";
    }
    esp_task_wdt_reset();
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void setup() {
    configLora.DEBUG = true; 
    Serial.begin(9600);
    delay(1000);

    Heltec.begin(false, false, true);
    inicializarPantalla();

    Hardware::inicializar();
    ManejoComunicacion::inicializar();
    ManejoEEPROM::leerTarjetaEEPROM(); 
    configurarDisplay(configLora.displayOn);

    if (strlen(configLora.IDLora) == 0) {
        imprimirSerial("ID no encontrado en memoria, usando valor por defecto.", 'r');
        strcpy(configLora.IDLora, "001");
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
    } else {
        imprimirSerial("ID cargado de memoria: " + String(configLora.IDLora), 'g');
        mostrarInfo("ID cargado: " + String(configLora.IDLora));
    }

    if (configLora.Canal < 0 || configLora.Canal > 8) {
        imprimirSerial("Canal no válido en memoria, usando valor por defecto.", 'r');
        configLora.Canal = 0;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
    } else {
        imprimirSerial("Canal cargado de memoria: " + String(configLora.Canal) + " (" + String(canales[configLora.Canal], 1) + " MHz)", 'g');
        mostrarInfo("Canal cargado: " + String(configLora.Canal));
    }

    if (lora.begin(canales[configLora.Canal]) != RADIOLIB_ERR_NONE) {
        imprimirSerial("LoRa init failed!", 'r'); mostrarError("LoRa init failed!"); while (true);
    }

    lora.setDio1Action(setFlag);
    lora.startReceive();
    pinMode(LED_STATUS, OUTPUT);

    imprimirSerial("LoRa ready.", 'g');
    imprimirSerial("ID de este nodo: " + String(configLora.IDLora), 'c');
    imprimirSerial("Canal: " + String(configLora.Canal) + " (" + String(canales[configLora.Canal], 1) + " MHz)", 'c');
    imprimirSerial("Escribe en el formato: ID@R@CMD@##", 'y');
    imprimirSerial("Ejemplo: A01@L@GETID@42", 'y');
    imprimirSerial("Version:" + Version, 'm');

    mostrarEstadoLoRa(String(configLora.IDLora), String(configLora.Canal), Version);

    if (!modoProgramacion && tareaComandosSerial == NULL && configLora.DEBUG) {
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

    if (!modoProgramacion && tareaComandosLoRa == NULL) {
      imprimirSerial("Iniciando tarea de recepcion de comandos LoRa...", 'c');
      xTaskCreatePinnedToCore(
        recibirComandosLoRa,
        "Comandos LoRa",
        5120,
        NULL,
        1,
        &tareaComandosLoRa,
        0
      );
      imprimirSerial("Tarea de recepcion de comandos LoRa iniciada", 'c');
    }
}

void loop() {
  if (!modoProgramacion && tareaComandosSerial == NULL && configLora.DEBUG) {
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

  if (!modoProgramacion && tareaComandosVecinal == NULL && configLora.UART) {
    imprimirSerial("Iniciando tarea de recepcion de comandos Vecinal...", 'c');
    xTaskCreatePinnedToCore(
      recibirComandosVecinal,
      "Comandos Vecinales",
      5120,
      NULL,
      1,
      &tareaComandosVecinal,
      0
    );
    imprimirSerial("Tarea de recepcion de comandos Vecinal iniciada", 'c');
  }

  if (!modoProgramacion && tareaComandosLoRa == NULL) {
    imprimirSerial("Iniciando tarea de recepcion de comandos LoRa...", 'c');
    xTaskCreatePinnedToCore(
      recibirComandosLoRa,
      "Comandos LoRa",
      5120,
      NULL,
      1,
      &tareaComandosLoRa,
      0
    );
    imprimirSerial("Tarea de recepcion de comandos LoRa iniciada", 'c');
  }

  delay(100);
}