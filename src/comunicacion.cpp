#include <Arduino.h>
#include <RCSwitch.h>
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

volatile bool rfSignalReceived = false;
unsigned long lastRFSignalTime = 0;
static RCSwitch mySwitch = RCSwitch();

void habilitarPantalla() {
    configLora.Pantalla = true;
    configurarDisplay(true);
    imprimirSerial("Pantalla habilitada.", 'g');
    ManejoEEPROM::guardarTarjetaConfigEEPROM();
}

void deshabilitarPantalla() {
    configLora.Pantalla = false;
    configurarDisplay(false); 
    imprimirSerial("Pantalla deshabilitada.", 'y');
    ManejoEEPROM::guardarTarjetaConfigEEPROM();
}

void parpadearLEDStatus(int veces, int periodo_ms) {
    for (int i = 0; i < veces; ++i) {
        digitalWrite(LED_STATUS, HIGH);
        delay(periodo_ms / 2);
        digitalWrite(LED_STATUS, LOW);
        delay(periodo_ms / 2);
    }
}

void manejarCodigoRF(unsigned long value) {
    parpadearLEDStatus(3, 200);

    if (value == 7969128) {
        digitalWrite(PIN_IO1, HIGH);
        digitalWrite(PIN_IO2, LOW);
        imprimirSerial("PIN_IO1 habilitado por RF: 7969128", 'g');
    } else if (value == 7969124) {
        digitalWrite(PIN_IO2, HIGH);
        digitalWrite(PIN_IO1, LOW);
        imprimirSerial("PIN_IO2 habilitado por RF: 7969124", 'g');
    } else if (value == 7969122) {
        digitalWrite(PIN_IO1, HIGH);
        digitalWrite(PIN_IO2, LOW);
        imprimirSerial("PIN_IO1 habilitado por RF: 7969122", 'g');
    } else if (value == 7969121) {
        digitalWrite(PIN_IO1, LOW);
        digitalWrite(PIN_IO2, LOW);
        imprimirSerial("PIN_IO1 y PIN_IO2 deshabilitados por RF: 7969121", 'y');
    }
}

void ManejoComunicacion::initRFReceiver() {
    mySwitch.enableReceive(RECEPTOR_RF); 
    imprimirSerial("Receptor RF RX500 inicializado con rc-switch en pin " + String(RECEPTOR_RF), 'g');
}

void ManejoComunicacion::leerRFReceiver() {
    static unsigned long ultimoValor = 0; 
    static unsigned long ultimoTiempo = 0; 
    const unsigned long intervalo = 200;   

    if (mySwitch.available()) {
        unsigned long value = mySwitch.getReceivedValue();
        unsigned long ahora = millis();

        if (value == 0) {
            Serial.println("Codigo desconocido recibido (demasiado corto o ruido)");
        } else {
            if (value != ultimoValor || (ahora - ultimoTiempo) >= intervalo) {
                Serial.println("Codigo RF recibido: " + String(value));
                manejarCodigoRF(value);
                ultimoValor = value;
                ultimoTiempo = ahora;
            }
        }
        mySwitch.resetAvailable();
    }
}


void ManejoComunicacion::inicializar() {
    Serial.begin(9600);
    delay(1000);
    initI2C();
    initUART();
    initRFReceiver();

    if (lora.begin() != RADIOLIB_ERR_NONE) {
        Serial.println("LoRa init failed!");
        while (true);
    }
}

void ManejoComunicacion::initUART() {
    if (configLora.UART) {
        Serial2.begin(9600, SERIAL_8N1, UART_RX, UART_TX); 
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
        Wire.begin(I2C_SDA, I2C_SCL);
        imprimirSerial("I2C inicializado correctamente.", 'g');
    } else {
        imprimirSerial("I2C inhabilitado", 'y');
    }
}

int ManejoComunicacion::scannerI2C() {
    byte error, address;
    int nDevices = 0;
    imprimirSerial("Escaneando puerto I2C...\n", 'y');

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            String mensaje = "I2C encontrado en la direccion 0x";
            if (address < 16)
                mensaje += "0";
            mensaje += String(address, HEX);
            mensaje += " !";
            imprimirSerial(mensaje, 'c');
            nDevices++;
        } else if (error == 4) {
            String mensaje = "Error desconocido en la direccion 0x";
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
    String IDLoraRecibido = comandoLoRa.substring(0, 3); 
    imprimirSerial("Enviando comando " + comandoLoRa + " a lora con el ID " + IDLoraRecibido);
}

void ManejoComunicacion::procesarComando(const String &comandoRecibido, String &respuesta) {
    esp_task_wdt_reset();

    if (comandoRecibido.isEmpty()) { 
        respuesta = "Comando vacio";
        imprimirSerial(respuesta, 'r');
        return;
    } else if (comandoRecibido == ultimoComandoRecibido) {
        imprimirSerial("Comando repetido, no se ejecutara una nueva accion\n", 'c');
        respuesta = "Comando repetido";
        return;
    }
    ultimoComandoRecibido = comandoRecibido;
    
    int primerSep = comandoRecibido.indexOf('@');
    int segundoSep = comandoRecibido.indexOf('@', primerSep + 1);
    int tercerSep = comandoRecibido.indexOf('@', segundoSep + 1);

    if (primerSep == -1 || segundoSep == -1 || tercerSep == -1 ||
        primerSep == 0 || segundoSep == primerSep + 1 || tercerSep == segundoSep + 1) {
        respuesta = "ERR: Formato de comando invalido. Usa: ID@R@CMD@##";
        imprimirSerial(respuesta, 'r');
        return;
    }

    String comandoDestinoID = comandoRecibido.substring(0, primerSep); 
    char red = comandoRecibido.charAt(primerSep + 1);
    String comandoProcesar = comandoRecibido.substring(segundoSep + 1, tercerSep); 
    String valorAleatorio = comandoRecibido.substring(tercerSep + 1); 

    String prefix1 = "";
    String accion = "";

    imprimirSerial("\nComando recibido en procesado de comando: " + comandoRecibido);
    imprimirSerial("ID de Destino del Comando -> " + comandoDestinoID);
    imprimirSerial("Red -> " + String(red));
    imprimirSerial("Comando a Procesar -> " + comandoProcesar);
    imprimirSerial("Valor Aleatorio -> " + valorAleatorio);

    if (comandoProcesar.startsWith("SETPIN:") || comandoProcesar.startsWith("CLRPIN:") || comandoProcesar.startsWith("GETPIN:")) {
        String pinName = comandoProcesar.substring(comandoProcesar.indexOf(':') + 1);
        int pin = -1;
        if (pinName == "IO1") pin = PIN_IO1;
        else if (pinName == "IO2") pin = PIN_IO2;
        else if (pinName == "IO3") pin = PIN_IO3;
        else if (pinName == "IO4") pin = PIN_IO4;
        else if (pinName == "IO5") pin = PIN_IO5;
        else if (pinName == "IO6") pin = PIN_IO6;

        if (pin == -1) {
            respuesta = "Pin no reconocido: " + pinName;
            imprimirSerial(respuesta, 'r');
            return;
        }

        if (comandoProcesar.startsWith("SETPIN:")) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, HIGH);
            respuesta = "Pin " + pinName + " habilitado (HIGH)";
            imprimirSerial(respuesta, 'g');
            return;
        } else if (comandoProcesar.startsWith("CLRPIN:")) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, LOW);
            respuesta = "Pin " + pinName + " deshabilitado (LOW)";
            imprimirSerial(respuesta, 'y');
            return;
        } else if (comandoProcesar.startsWith("GETPIN:")) {
            pinMode(pin, INPUT); 
            int estado = digitalRead(pin);
            respuesta = "Pin " + pinName + " estado: " + String(estado ? "HABILITADO (HIGH)" : "DESHABILITADO (LOW)");
            imprimirSerial(respuesta, 'c');
            return;
        }
    }

    if (strcmp(comandoDestinoID.c_str(), configLora.IDLora) == 0 || strcmp(comandoDestinoID.c_str(), "000") == 0) {
        imprimirSerial("Comando dirigido a este nodo (" + String(configLora.IDLora) + ") o es broadcast (000).", 'g');

        if (red == 'L') { 
            if (comandoProcesar.startsWith("ID")) {
                accion = comandoProcesar.substring(2, 3);
                if (accion == "L") {
                    respuesta = "ID del nodo: " + String(configLora.IDLora);
                    imprimirSerial("El ID del nodo es -> " + String(configLora.IDLora), 'y');
                } else if (accion == "C") {
                    if (comandoProcesar.length() >= 7) { 
                        prefix1 = comandoProcesar.substring(4, 7); 
                        imprimirSerial("Cambiando el ID del nodo a -> " + prefix1, 'c');
                        strncpy(configLora.IDLora, prefix1.c_str(), sizeof(configLora.IDLora) - 1);
                        configLora.IDLora[sizeof(configLora.IDLora) - 1] = '\0';
                        ManejoEEPROM::guardarTarjetaConfigEEPROM();
                        respuesta = "ID cambiado a: " + prefix1;
                    } else {
                        imprimirSerial("Formato de comando ID inválido para cambio. Esperado: IDC:XXX", 'r');
                        respuesta = "Formato ID invalido";
                    }
                }
            } else if (comandoProcesar.startsWith("CH")) {
                accion = comandoProcesar.substring(2, 3);
                if (accion == "L") {
                    respuesta = "Canal actual: " + String(configLora.Canal);
                    imprimirSerial("El canal del nodo es -> " + String(configLora.Canal), 'y');
                } else if (accion == "C") {
                    int idxMayor = comandoProcesar.indexOf('>');
                    if (idxMayor != -1 && idxMayor + 1 < comandoProcesar.length()) {
                        prefix1 = comandoProcesar.substring(idxMayor + 1);
                        imprimirSerial("Cambiando el canal del nodo a -> " + prefix1, 'c');
                        configLora.Canal = prefix1.toInt();
                        if (configLora.Canal >= 0 && configLora.Canal < (sizeof(canales) / sizeof(canales[0]))) { 
                            ManejoEEPROM::guardarTarjetaConfigEEPROM();
                            lora.begin(canales[configLora.Canal]); 
                            respuesta = "Canal cambiado a: " + prefix1;
                            esp_restart(); 
                        } else {
                            respuesta = "Canal fuera de rango";
                            imprimirSerial("Canal especificado fuera de rango: " + prefix1, 'r');
                        }
                    } else {
                        imprimirSerial("Formato de comando de canal inválido", 'r');
                        respuesta = "Formato de comando de canal inválido";
                    }
                }
            } else if (comandoProcesar.startsWith("SCR")) {
                accion = comandoProcesar.substring(4, 5);
                if (accion == "L") {
                    respuesta = "Pantalla " + String(configLora.displayOn ? "activada" : "desactivada");
                    String mensaje = "La pantalla de la LoRa se encuentra ";
                    mensaje += configLora.displayOn ? "Activada" : "Desactivada";
                    imprimirSerial(mensaje);
                } else if (accion == "0") {
                    imprimirSerial("Desactivando la pantalla");
                    configurarDisplay(false); 
                    respuesta = "Pantalla deshabilitada";
                } else if (accion == "1") {
                    imprimirSerial("Activando la pantalla");
                    configurarDisplay(true);
                    respuesta = "Pantalla habilitada";
                }
            } else if (comandoProcesar.startsWith("I2C")) {
                accion = comandoProcesar.substring(4, 5);
                if (accion == "L") {
                    respuesta = "I2C " + String(configLora.I2C ? "activada" : "desactivada");
                    String mensaje = "La comunicacion I2C se encuentra ";
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
                    String mensaje = "La comunicacion UART se encuentra ";
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
                    String mensaje = "La conexion WiFi se encuentra ";
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
                    respuesta = "WiFi activada";
                }
            } else if (comandoProcesar.startsWith("RESET")) {
                imprimirSerial("Reiniciando la tarjeta LoRa...");
                respuesta = "Reiniciando la tarjeta LoRa...";
                delay(1000);
                ManejoEEPROM::borrarTarjetaConfigEEPROM(); 
                esp_restart();
            } else if (comandoProcesar.startsWith("MPROG")) {
                imprimirSerial("Entrando a modo programacion a traves de comando...");
                if (!modoProgramacion) { 
                    Interfaz::entrarModoProgramacion(); 
                }
                respuesta = "Entrando a modo programación";
            } else {
                respuesta = "ERR: Comando LoRa desconocido";
                imprimirSerial("Comando LoRa desconocido: " + comandoProcesar, 'r');
            }
        } else if (red == 'V') {
            imprimirSerial("Comando para red Vecinal (UART).", 'g');
            ManejoComunicacion::escribirVecinal(comandoRecibido); 
            respuesta = "Comando enviado a vecinal";
        } else {
            respuesta = "ERR: Red desconocida";
            imprimirSerial("Red desconocida: " + String(red), 'r');
        }
    } else {
        imprimirSerial("Comando ignorado, no es para este ID (" + String(configLora.IDLora) + ") ni para 000. Destino recibido: " + comandoDestinoID, 'y');
        respuesta = "IGNORADO: " + comandoDestinoID;
    }
}