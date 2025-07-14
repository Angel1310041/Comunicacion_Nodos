#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>
#include "heltec.h"
#include "config.h"
#include "pantalla.h"
#include "comunicacion.h"
#include "eeprom.h"

SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
bool tieneInternet = true; // Pon true en la placa que tiene internet
String Version = "1.2.2.2";
volatile bool receivedFlag = false;

#define MSG_ID_BUFFER_SIZE 16
String msgIdBuffer[MSG_ID_BUFFER_SIZE];
int msgIdBufferIndex = 0;

#define MAX_NODOS_ACTIVOS 32
String nodosActivos[MAX_NODOS_ACTIVOS];
int numNodosActivos = 0;

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

// --- Envío de mensajes ---
void enviarMensaje(const String& destino, const char* mensaje) {
    if (strcmp(destino.c_str(), configLora.IDLora) != 0 && strlen(mensaje) > 0) {
        String siguienteHop = destino, msgID = generarMsgID();
        char paquete[256];
        snprintf(paquete, sizeof(paquete), "ORIG:%s|DEST:%s|MSG:%s|HOP:%s|CANAL:%d|ID:%s",
            configLora.IDLora, destino.c_str(), mensaje, siguienteHop.c_str(), configLora.Canal, msgID.c_str());
        Serial.println("Enviando a HOP:" + siguienteHop + " por canal " + String(configLora.Canal));
        mostrarMensaje("Enviando...", "A HOP: " + siguienteHop, 0);
        lora.standby();
        int resultado = lora.transmit(paquete);
        if (resultado == RADIOLIB_ERR_NONE) {
            Serial.println("Mensaje enviado correctamente.");
            digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
            guardarMsgID(msgID); mostrarMensajeEnviado(destino, mensaje);
        } else {
            Serial.println("Error al enviar: " + String(resultado));
            mostrarError("Error al enviar: " + String(resultado));
        }
        lora.startReceive();
    } else {
        Serial.println("Destino inválido o mensaje vacío.");
        mostrarInfo("Destino invalido o mensaje vacio.");
    }
}

// --- SETUP ---
void setup() {
    Serial.begin(9600);
    Heltec.begin(false, false, true);
    inicializarPantalla();
    delay(1000);

    cargarConfig();
    configurarDisplay(configLora.displayOn);

    if (strlen(configLora.IDLora) == 0) { pedirID(); guardarConfig(); }
    else { Serial.println("ID cargado de memoria: " + String(configLora.IDLora)); mostrarInfo("ID cargado: " + String(configLora.IDLora)); }

    if (configLora.Canal < 0 || configLora.Canal > 8) { pedirCanal(); guardarConfig(); }
    else { Serial.println("Canal cargado de memoria: " + String(configLora.Canal) + " (" + String(canales[configLora.Canal], 1) + " MHz)"); mostrarInfo("Canal cargado: " + String(configLora.Canal)); }

    if (lora.begin(canales[configLora.Canal]) != RADIOLIB_ERR_NONE) {
        Serial.println("LoRa init failed!"); mostrarError("LoRa init failed!"); while (true);
    }

    lora.setDio1Action(setFlag);
    lora.startReceive();
    pinMode(LED_PIN, OUTPUT);

    Serial.println("LoRa ready.");
    Serial.println("ID de este nodo: " + String(configLora.IDLora));
    Serial.println("Canal: " + String(configLora.Canal) + " (" + String(canales[configLora.Canal], 1) + " MHz)");
    Serial.println("Escribe en el formato: ID_DESTINO|mensaje");
    Serial.println("Para borrar la configuración y reconfigurar, escribe: RESET");
    Serial.println("Para habilitar el display: DISPLAY ON");
    Serial.println("Para deshabilitar el display: DISPLAY OFF");
    Serial.println("Version:" + Version);

    mostrarEstadoLoRa(String(configLora.IDLora), String(configLora.Canal), Version);
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
            Serial.println("IDs de nodos activos detectados:");
            for (int i = 0; i < numNodosActivos; i++) Serial.println(nodosActivos[i]);
            esperandoMostrar = false; sondeoManual = false;
        }
    }

    // Comandos por Serial
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n'); input.trim();
        if (input.equalsIgnoreCase("RESET")) { borrarConfig(); mostrarInfo("Configuracion borrada. Reinicia."); return; }
        if (input.equalsIgnoreCase("DISPLAY ON")) { configurarDisplay(true); Serial.println("Comando: DISPLAY ON - Display habilitado."); return; }
        if (input.equalsIgnoreCase("DISPLAY OFF")) { configurarDisplay(false); Serial.println("Comando: DISPLAY OFF - Display deshabilitado."); return; }
        if (input.equalsIgnoreCase("SONDEO")) {
            if (tieneInternet) {
                limpiarNodosActivos(); enviarSondeo();
                tiempoMostrar = millis(); esperandoMostrar = true; sondeoManual = true;
                Serial.println("Sondeo manual iniciado...");
            } else Serial.println("Esta placa no tiene internet, no puede hacer sondeo.");
            return;
        }

        // Envío de mensajes
        int sep = input.indexOf('|');
        if (sep > 0) {
            String destino = input.substring(0, sep);
            char mensaje[201];
            input.substring(sep + 1).toCharArray(mensaje, sizeof(mensaje));
            enviarMensaje(destino, mensaje);
        } else {
            Serial.println("Formato inválido. Usa: ID_DESTINO|mensaje");
            mostrarInfo("Formato invalido. Usa: ID|mensaje");
        }
    }

    // Recepción de mensajes
    if (receivedFlag) {
        receivedFlag = false;
        String msg;
        int state = lora.readData(msg);
        if (state == RADIOLIB_ERR_NONE) {
            responderSondeo(msg);
            Serial.println("Recibido: " + msg);
            if (tieneInternet && msg.startsWith("RESPUESTA|ID:")) {
                int idxIni = msg.indexOf("RESPUESTA|ID:") + 13, idxFin = msg.indexOf("|CANAL:");
                if (idxIni != -1 && idxFin != -1 && idxFin > idxIni) {
                    String idNodo = msg.substring(idxIni, idxFin);
                    if (idNodo != String(configLora.IDLora)) {
                        agregarNodoActivo(idNodo);
                        Serial.println("Nodo activo detectado: " + idNodo);
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
                    digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);

                    if (cuerpo.equalsIgnoreCase("DISPLAY ON")) {
                        configurarDisplay(true);
                        Serial.println("Comando recibido: DISPLAY ON");
                    } else if (cuerpo.equalsIgnoreCase("DISPLAY OFF")) {
                        configurarDisplay(false);
                        Serial.println("Comando recibido: DISPLAY OFF");
                    } else if (cuerpo.equalsIgnoreCase("RESET")) {
                        borrarConfig();
                        Serial.println("Comando recibido: RESET. Configuración borrada, reinicia el nodo.");
                        mostrarInfo("Configuración borrada. Reinicia.");
                        // ESP.restart(); // Descomenta si quieres reinicio automático
                    } else {
                        mostrarMensajeRecibido(orig, cuerpo); // Mensaje normal
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
}