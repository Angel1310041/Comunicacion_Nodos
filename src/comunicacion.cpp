#include <Arduino.h>
#include "config.h"
#include "pantalla.h"
#include "heltec.h"
#include "RadioLib.h"
#include "comunicacion.h"
#include "eeprom.h"
#include "interfaz.h"

extern SX1262 lora;
extern bool tieneInternet;
LoRaConfig configLora;
String ultimoComandoRecibido = "";
TwoWire I2CGral = TwoWire(1);

// --- FUNCIONES PARA CONTROL DE PANTALLA ---
void habilitarPantalla() {
    configLora.Pantalla = true;
    configurarDisplay(true); // Enciende la pantalla físicamente
    imprimirSerial("Pantalla habilitada.", 'g');
    ManejoEEPROM::guardarTarjetaConfigEEPROM();
}

void deshabilitarPantalla() {
    configLora.Pantalla = false;
    configurarDisplay(false); // Apaga la pantalla físicamente
    imprimirSerial("Pantalla deshabilitada.", 'y');
    ManejoEEPROM::guardarTarjetaConfigEEPROM();
}

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

void ManejoComunicacion::inicializar() {
    Serial.begin(9600);
    delay(1000);
    initI2C();
    initUART();

    if (lora.begin() != RADIOLIB_ERR_NONE) {
        Serial.println("LoRa init failed!");
        while (true);
    }
}

void ManejoComunicacion::initUART() {
    if (configLora.UART) {
        Serial2.begin(9600, SERIAL_8N1, UART_RX, UART_TX); // RX=46, TX=45
        imprimirSerial("UART inicializado correctamente.", 'g');
    } else {
        imprimirSerial("UART inhabilitado", 'y');
    }
}

String ManejoComunicacion::leerVecinal() {
    if (configLora.UART) {
        String comandoVecinal = "";
        if (Serial2.available()) {
            comandoVecinal = Serial2.readStringUntil('\n');
            if (comandoVecinal != "") {
                imprimirSerial("Comando recibido en Serial2: " + comandoVecinal, 'y');
                return comandoVecinal;
            } else {
                //imprimirSerial("Comando vacio");
                return "";
            }
        }
        return "";
    } else {
        imprimirSerial("Comunicacion UART deshabilitada", 'y');
        return "";
    }
}

void ManejoComunicacion::escribirVecinal(String envioVecinal) {
    if (configLora.UART) {
        Serial2.println(envioVecinal);
        imprimirSerial("Comando enviado a la Alarma Vecinal:\n  -> " + envioVecinal, 'g');
    }
}

void ManejoComunicacion::initI2C() {
    if (configLora.I2C) {
        //I2CGral.begin(I2C_SDA, I2C_SCL);
        Wire.begin(I2C_SDA, I2C_SCL);
        imprimirSerial("I2C inicializado correctamente.", 'g');
    } else {
        imprimirSerial("I2C inhabilitado", 'y');
    }
}

// Cambiado de void a int
int ManejoComunicacion::scannerI2C() {
    byte error, address;
    int nDevices = 0;
    imprimirSerial("Escaneando puerto I2C...\n", 'y');

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            mensaje = "I2C encontrado en la direccion 0x";
            if (address < 16)
                mensaje += "0";
            mensaje += String(address, HEX);
            mensaje += " !";
            imprimirSerial(mensaje, 'c');
            nDevices++;
        } else if (error == 4) {
            mensaje = "Error desconocido en la direccion 0x";
            if (address < 16)
                mensaje += "0";
            mensaje += String(address, HEX);
            imprimirSerial(mensaje, 'r');
        }
        delay(5);
    }

    if (nDevices == 0) {
        imprimirSerial("Ningun dispositivo I2C encontrado", 'r');
    } else {
        imprimirSerial("\nEscaneo completado!", 'g');
    }
    return nDevices;
}

String ManejoComunicacion::leerSerial() {
    String comandoVecinal = "";
    if (Serial.available()) {
        comandoVecinal = Serial.readStringUntil('\n');
        if (comandoVecinal != "") {
            imprimirSerial("Comando recibido en Serial: " + comandoVecinal, 'y');
            return comandoVecinal;
        }
    }
    return "";
}

void ManejoComunicacion::envioMsjLoRa(String comandoLoRa) {
    String IDLoraRecibido = comandoLoRa.substring(0, 3); // ID del nodo LoRa
    imprimirSerial("Enviando comando " + comandoLoRa + " a lora con el ID " + IDLoraRecibido);
}

void ManejoComunicacion::procesarComando(const String &comandoRecibido, String &respuesta) {
    esp_task_wdt_reset();
    int primerSep = comandoRecibido.indexOf('@');
    int segundoSep = comandoRecibido.indexOf('@', primerSep + 1);
    int tercerSep = comandoRecibido.indexOf('@', segundoSep + 1);
    String IDLora = comandoRecibido.substring(0, primerSep);
    String destino = comandoRecibido.substring(primerSep + 1, segundoSep);
    String comandoProcesar = comandoRecibido.substring(segundoSep + 1, tercerSep);
    String prefix1 = "";
    String prefix2 = "";
    String accion = "";

    respuesta = ""; // Por defecto, vacío

    if (comandoRecibido == "") {
        respuesta = "Comando vacío";
        return;
    } else if (comandoRecibido == ultimoComandoRecibido) {
        imprimirSerial("Comando repetido, no se ejecutara una nueva accion\n", 'c');
        respuesta = "Comando repetido";
        return;
    }
    ultimoComandoRecibido = comandoRecibido; // Actualizar el ultimo comando
    imprimirSerial("\nComando recibido en procesado de comando: " + comandoRecibido);
    imprimirSerial("\nComando a Procesar -> " + comandoProcesar);

    if (IDLora == String(configLora.IDLora) && destino == "L") {
        if (comandoProcesar.startsWith("ID")) {
            accion = comandoProcesar.substring(2, 3);
            if (accion == "L") {
                respuesta = "ID del nodo: " + String(configLora.IDLora);
                imprimirSerial("El ID del nodo es -> " + String(configLora.IDLora), 'y');
            } else if (accion == "C") {
    prefix1 = comandoProcesar.substring(4, 7);
    imprimirSerial("Cambiando el ID del nodo a -> " + prefix1, 'c');
    strncpy(configLora.IDLora, prefix1.c_str(), sizeof(configLora.IDLora) - 1);
    configLora.IDLora[sizeof(configLora.IDLora) - 1] = '\0'; // Asegura el terminador nulo
    ManejoEEPROM::guardarTarjetaConfigEEPROM();
    respuesta = "ID cambiado a: " + prefix1;
}

        } else if (comandoProcesar.startsWith("CH")) {
    accion = comandoProcesar.substring(2, 3); // <-- CORREGIDO
    if (accion == "L") {
        respuesta = "Canal actual: " + String(configLora.Canal);
        imprimirSerial("El canal del nodo es -> " + String(configLora.Canal), 'y');
    } else if (accion == "C") {
        int idxMayor = comandoProcesar.indexOf('>');
        if (idxMayor != -1 && idxMayor + 1 < comandoProcesar.length()) {
            prefix1 = comandoProcesar.substring(idxMayor + 1); // Toma todo lo que sigue al '>'
            imprimirSerial("Cambiando el canal del nodo a -> " + prefix1, 'c');
            configLora.Canal = prefix1.toInt();
            ManejoEEPROM::guardarTarjetaConfigEEPROM();
            lora.begin(canales[configLora.Canal]);
            respuesta = "Canal cambiado a: " + prefix1;
            esp_restart();
        } else {
            imprimirSerial("Formato de comando de canal inválido", 'r');
            respuesta = "Formato de comando de canal inválido";
        }
    }
}   else if (comandoProcesar.startsWith("SCR")) {
            accion = comandoProcesar.substring(4, 5);
            if (accion == "L") {
                respuesta = "Pantalla " + String(configLora.Pantalla ? "activada" : "desactivada");
                mensaje = "La pantalla de la LoRa se encuentra ";
                mensaje += configLora.Pantalla ? "Activada" : "Desactivada";
                imprimirSerial(mensaje);
            } else if (accion == "0") {
                imprimirSerial("Desactivando la pantalla");
                deshabilitarPantalla();
                respuesta = "Pantalla deshabilitada";
            } else if (accion == "1") {
                imprimirSerial("Activando la pantalla");
                habilitarPantalla();
                respuesta = "Pantalla habilitada";
            }

        } else if (comandoProcesar.startsWith("I2C")) {
            accion = comandoProcesar.substring(4, 5);
            if (accion == "L") {
                respuesta = "I2C " + String(configLora.I2C ? "activada" : "desactivada");
                mensaje = "La comunicacion I2C se encuentra ";
                mensaje += configLora.I2C ? "Activada" : "Desactivada";
                imprimirSerial(mensaje);
            } else if (accion == "0") {
                imprimirSerial("Desactivando la comunicacion I2C", 'y');
                configLora.I2C = false;
                ManejoEEPROM::guardarTarjetaConfigEEPROM();
                respuesta = "I2C desactivada";
            } else if (accion == "1") {
                imprimirSerial("Activando la comunicacion I2C");
                configLora.I2C = true;
                ManejoEEPROM::guardarTarjetaConfigEEPROM();
                ManejoComunicacion::initI2C();
                respuesta = "I2C activada";
            }

        } else if (comandoProcesar.startsWith("SCANI2C")) {
            if (configLora.I2C) {
                imprimirSerial("Escaneando el puerto I2C en busca de dispositivos...", 'y');
                int encontrados = ManejoComunicacion::scannerI2C();
                imprimirSerial("Se encontraron " + String(encontrados) + " direcciones I2C");
                respuesta = "Se encontraron " + String(encontrados) + " direcciones I2C";
            } else {
                imprimirSerial("La comunicacion I2C esta desactivada, no se puede escanear", 'r');
                respuesta = "I2C desactivada, no se puede escanear";
            }

        } else if (comandoProcesar.startsWith("UART")) {
            accion = comandoProcesar.substring(5, 6);
            if (accion == "L") {
                respuesta = "UART " + String(configLora.UART ? "activada" : "desactivada");
                mensaje = "La comunicacion UART se encuentra ";
                mensaje += configLora.UART ? "Activada" : "Desactivada";
                imprimirSerial(mensaje);
            } else if (accion == "0") {
                imprimirSerial("Desactivando la comunicacion UART");
                configLora.UART = false;
                ManejoEEPROM::guardarTarjetaConfigEEPROM();
                respuesta = "UART desactivada";
            } else if (accion == "1") {
                imprimirSerial("Activando la comunicacion UART");
                configLora.UART = true;
                ManejoEEPROM::guardarTarjetaConfigEEPROM();
                ManejoComunicacion::initUART();
                respuesta = "UART activada";
            }

        } else if (comandoProcesar.startsWith("WIFI")) {
            accion = comandoProcesar.substring(5, 6);
            if (accion == "L") {
                respuesta = "WiFi " + String(configLora.WiFi ? "activada" : "desactivada");
                mensaje = "La conexion WiFi se encuentra ";
                mensaje += configLora.WiFi ? "Activada" : "Desactivada";
                imprimirSerial(mensaje);
            } else if (accion == "0") {
                imprimirSerial("Desactivando la comunicacion WiFi");
                configLora.WiFi = false;
                ManejoEEPROM::guardarTarjetaConfigEEPROM();
                respuesta = "WiFi desactivada";
            } else if (accion == "1") {
                imprimirSerial("Activando la comunicacion WiFi");
                configLora.WiFi = true;
                ManejoEEPROM::guardarTarjetaConfigEEPROM();
                // Colocar la funcion de conexion a redes existentes
                respuesta = "WiFi activada";
            }

        } else if (comandoProcesar.startsWith("RESET")) {
            imprimirSerial("Reiniciando la tarjeta LoRa...");
            respuesta = "Reiniciando la tarjeta LoRa...";
            delay(1000);
            ManejoEEPROM::borrarTarjetaConfigEEPROM();

        } else if (comandoProcesar.startsWith("MPROG")) {
            imprimirSerial("Entrando a modo programacion a traves de comando...");
            if (!modoProgramacion) {
                Interfaz::entrarModoProgramacion();
            }
            respuesta = "Entrando a modo programación";
        }

    } else if (IDLora == String(configLora.IDLora) && destino == "V") {
        ManejoComunicacion::escribirVecinal(comandoRecibido);
        respuesta = "Comando enviado a vecinal";
    } else {
        imprimirSerial("Reenviando comando al nodo con el ID " + IDLora);
        ManejoComunicacion::envioMsjLoRa(IDLora);
        respuesta = "Reenviando comando al nodo con el ID " + IDLora;
    }
}