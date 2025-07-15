#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "heltec.h"
#include "config.h"
#include "pantalla.h"
#include "comunicacion.h"
#include "eeprom.h"
#include "hardware.h"

// --- Variables globales ---
SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
bool tieneInternet = true; // Pon true en la placa que tiene internet
String Version = "1.3.2.5";
volatile bool receivedFlag = false;
bool modoProgramacion = false;
TaskHandle_t tareaComandosSerial = NULL;
extern String ultimoComandoRecibido; // Definida en comunicacion.cpp

#define MSG_ID_BUFFER_SIZE 16
String msgIdBuffer[MSG_ID_BUFFER_SIZE];
int msgIdBufferIndex = 0;

#define MAX_NODOS_ACTIVOS 32
String nodosActivos[MAX_NODOS_ACTIVOS];
int numNodosActivos = 0;
String mensaje = "";

// --- Utilidades de impresión con color ---
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

// --- Utilidades para nodos activos y mensajes ---
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

// --- Envío de mensajes con estructura nueva ---
void enviarComandoEstructurado(const String& destino, char red, const String& comando) {
    // Generar número aleatorio de 2 dígitos
    int numAzar = random(10, 99);
    String msg = destino + "@" + red + "@" + comando + "@" + String(numAzar);

    // Enviar por LoRa (puedes adaptar para vecinal si es necesario)
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
            digitalWrite(LED_STATUS, HIGH); delay(100); digitalWrite(LED_STATUS, LOW);
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

// --- Tarea para comandos seriales (FreeRTOS) ---
void recibirComandoSerial(void *pvParameters) {
    imprimirSerial("Esperando comandos por Serial...", 'b');
    tareaComandosSerial = xTaskGetCurrentTaskHandle();
    String comandoSerial = "";

    while (true) {
        comandoSerial = ManejoComunicacion::leerSerial();
        comandoSerial.trim();

        if (!comandoSerial.isEmpty()) {
            imprimirSerial("Comando recibido por Serial: " + comandoSerial, 'y');
            // Espera comandos en formato: ID@R@CMD
            int idx1 = comandoSerial.indexOf('@');
            int idx2 = comandoSerial.indexOf('@', idx1 + 1);
            int idx3 = comandoSerial.indexOf('@', idx2 + 1);
            if (idx1 > 0 && idx2 > idx1 && idx3 > idx2) {
                String destino = comandoSerial.substring(0, idx1);
                char red = comandoSerial.charAt(idx1 + 1);
                String comando = comandoSerial.substring(idx2 + 1, idx3);
                // El número aleatorio se ignora al enviar, se genera nuevo
                enviarComandoEstructurado(destino, red, comando);
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

// --- SETUP ---
void setup() {
    configLora.DEBUG = true; // Habilitar debug por defecto
    Serial.begin(9600);
    delay(1000);

    Heltec.begin(false, false, true);
    inicializarPantalla();

    Hardware::inicializar();
    ManejoComunicacion::inicializar();
    cargarConfig();
    configurarDisplay(configLora.displayOn);

    if (strlen(configLora.IDLora) == 0) { pedirID(); guardarConfig(); }
    else { imprimirSerial("ID cargado de memoria: " + String(configLora.IDLora), 'g'); mostrarInfo("ID cargado: " + String(configLora.IDLora)); }

    if (configLora.Canal < 0 || configLora.Canal > 8) { pedirCanal(); guardarConfig(); }
    else { imprimirSerial("Canal cargado de memoria: " + String(configLora.Canal) + " (" + String(canales[configLora.Canal], 1) + " MHz)", 'g'); mostrarInfo("Canal cargado: " + String(configLora.Canal)); }

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

    // Iniciar tarea de comandos seriales si está en modo debug y no en modo programación
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
}

// --- LOOP ---
void loop() {
    static unsigned long ultimoSondeo = 0, tiempoMostrar = 0;
    static bool esperandoMostrar = false, sondeoManual = false;

    // Sondeo automático/manual
    if (tieneInternet) {
        if (millis() - ultimoSondeo > 600000 && !sondeoManual) {
            limpiarNodosActivos(); enviarSondeo();
            ultimoSondeo = millis(); tiempoMostrar = millis(); esperandoMostrar = true;
        }
        if (esperandoMostrar && millis() - tiempoMostrar > 5000) {
            mostrarNodosActivos();
            esperandoMostrar = false; sondeoManual = false;
        }
    }

    // Recepción de mensajes LoRa
    if (receivedFlag) {
        receivedFlag = false;
        String msg;
        int state = lora.readData(msg);
        if (state == RADIOLIB_ERR_NONE) {
            responderSondeo(msg);
            imprimirSerial("Recibido: " + msg, 'b');
            if (tieneInternet && msg.startsWith("RESPUESTA|ID:")) {
                int idxIni = msg.indexOf("RESPUESTA|ID:") + 13, idxFin = msg.indexOf("|CANAL:");
                if (idxIni != -1 && idxFin != -1 && idxFin > idxIni) {
                    String idNodo = msg.substring(idxIni, idxFin);
                    if (idNodo != String(configLora.IDLora)) {
                        agregarNodoActivo(idNodo);
                        imprimirSerial("Nodo activo detectado: " + idNodo, 'g');
                    }
                }
            }
            int canalMsg = -1, idxCanal = msg.indexOf("|CANAL:"), idxID = msg.indexOf("|ID:");
            if (idxCanal != -1)
                canalMsg = msg.substring(idxCanal + 7, idxID == -1 ? msg.length() : idxID).toInt();
            String msgID = (idxID != -1) ? msg.substring(idxID + 4) : "";
            if ((canalMsg != -1 && canalMsg != configLora.Canal) || (msgID.length() > 0 && esMsgDuplicado(msgID))) {
                lora.startReceive(); return;
            }
            if (msgID.length() > 0) guardarMsgID(msgID);

            int idxOrig = msg.indexOf("ORIG:"), idxDest = msg.indexOf("|DEST:"), idxMsg = msg.indexOf("|MSG:"), idxHop = msg.indexOf("|HOP:");
            if (idxOrig != -1 && idxDest != -1 && idxMsg != -1 && idxHop != -1) {
                String orig = msg.substring(idxOrig + 5, idxDest);
                String dest = msg.substring(idxDest + 6, idxMsg);
                String cuerpo = msg.substring(idxMsg + 5, idxHop);
                String hop = msg.substring(idxHop + 5, idxCanal == -1 ? msg.length() : idxCanal);

                // --- Procesa comandos recibidos SOLO si el mensaje es para este nodo ---
                if (strcmp(dest.c_str(), configLora.IDLora) == 0) {
                    digitalWrite(LED_STATUS, HIGH); delay(100); digitalWrite(LED_STATUS, LOW);

                    // Procesar comandos estructurados: ID@R@CMD@##
                    int idxA1 = cuerpo.indexOf('@');
                    int idxA2 = cuerpo.indexOf('@', idxA1 + 1);
                    int idxA3 = cuerpo.indexOf('@', idxA2 + 1);
                    if (idxA1 > 0 && idxA2 > idxA1 && idxA3 > idxA2) {
                        String idCmd = cuerpo.substring(0, idxA1);
                        char redCmd = cuerpo.charAt(idxA1 + 1);
                        String cmd = cuerpo.substring(idxA2 + 1, idxA3);
                        // String numAzar = cuerpo.substring(idxA3 + 1); // Si necesitas el número aleatorio

                        // Ejecutar comando
                        if (cmd.equalsIgnoreCase("SCR>1")) {
                            configurarDisplay(true);
                            imprimirSerial("Comando recibido: DISPLAY ON", 'g');
                        } else if (cmd.equalsIgnoreCase("SCR>0")) {
                            configurarDisplay(false);
                            imprimirSerial("Comando recibido: DISPLAY OFF", 'y');
                        } else if (cmd.equalsIgnoreCase("RESET")) {
                            borrarConfig();
                            imprimirSerial("Comando recibido: RESET. Configuración borrada, reinicia el nodo.", 'r');
                            mostrarInfo("Configuración borrada. Reinicia.");
                        } else if (cmd.equalsIgnoreCase("GETID")) {
                            // Responder con el ID de este nodo al origen
                            String respuesta = String(configLora.IDLora);
                            // Enviar respuesta estructurada de vuelta al origen
                            enviarComandoEstructurado(orig, redCmd, "ID:" + respuesta);
                            imprimirSerial("Comando recibido: GETID. Respondiendo con mi ID: " + respuesta, 'g');
                            mostrarMensajeRecibido(orig, "Mi ID: " + respuesta);
                        } else {
                            mostrarMensajeRecibido(orig, cmd); 
                        }
                    } else {
                        mostrarMensajeRecibido(orig, cuerpo); 
                    }
                }
                // --- Reenvío si soy hop ---
                else if (strcmp(hop.c_str(), configLora.IDLora) == 0) {
                    String siguienteHop = dest;
                    String nuevoMsg = "ORIG:" + orig + "|DEST:" + dest + "|MSG:" + cuerpo + "|HOP:" + siguienteHop + "|CANAL:" + String(configLora.Canal) + "|ID:" + msgID;
                    delay(100); lora.standby(); lora.transmit(nuevoMsg);
                    mostrarMensaje("Reenviando...", "A HOP: " + siguienteHop, 0);
                    lora.startReceive();
                }
            }
        }
        lora.startReceive();
        mostrarEstadoLoRa(String(configLora.IDLora), String(configLora.Canal), Version);
    }
    delay(100); // Pequeño delay para evitar saturar el loop
}