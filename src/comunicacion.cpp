#include <Arduino.h>
#include "config.h"
#include "pantalla.h"
#include "heltec.h"
#include "RadioLib.h"
#include "comunicacion.h"
#include "eeprom.h"


extern SX1262 lora;
extern bool tieneInternet;
LoRaConfig configLora;
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

TwoWire I2CGral = TwoWire(1);
String ultimoComandoRecibido = "";

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

void ManejoComunicacion::scannerI2C() {
  byte error, address;
  int nDevices;
  imprimirSerial("Escaneando puerto I2C...", 'y');

  for (address = 1; address< 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      mensaje = "\nI2C encontrado en la direccion 0x";
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

  if (nDevices == 0)
    imprimirSerial("Ningun dispositivo I2C encontrado", 'r');
  else
    imprimirSerial("\nEscaneo completado", 'g');
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

void ManejoComunicacion::procesarComando(String comandoRecibido) {
  esp_task_wdt_reset();
  String IDLora = comandoRecibido.substring(0, 3);
  int primerSep = comandoRecibido.indexOf('@');
  int segundoSep = comandoRecibido.indexOf('@', primerSep + 1);
  int tercerSep = comandoRecibido.indexOf('@', segundoSep + 1);
  String comandoProcesar = comandoRecibido.substring(segundoSep + 1, tercerSep);
  String prefix1 = "";
  String prefix2 = "";
  String accion = "";

  if (comandoRecibido == "") {
    //imprimirSerial("Comando vacio");
    return;
  } else if (comandoRecibido == ultimoComandoRecibido) {
    imprimirSerial("Comando repetido, no se ejecutara una nueva accion\n", 'c');
    return;
  }
  ultimoComandoRecibido = comandoRecibido; // Actualizar el ultimo comando
  imprimirSerial("\nComando recibido en procesado de comando: " + comandoRecibido);
  imprimirSerial("\nComando a Procesar -> " + comandoProcesar);

  if (IDLora == String(configLora.IDLora) && comandoRecibido.substring(4, 5) == "L") {
    if (comandoProcesar.startsWith("ID")) {
      // Ejemplo: IDC>AD9 / IDL
      accion = comandoProcesar.substring(2, 3);
      if (accion == "L") {
        imprimirSerial("El ID del nodo es -> " + String(configLora.IDLora), 'y');
      } else if (accion == "C") {
        prefix1 = comandoProcesar.substring(4, 7);
        imprimirSerial("Cambiando el ID del nodo a -> " + prefix1, 'c');
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
        strcpy(configLora.IDLora, prefix1.c_str());
      }

    } else if (comandoProcesar.startsWith("I2C")) {
      // Ejemplo: I2C>L / I2C>1/0
      accion = comandoProcesar.substring(4, 5);
      if (accion == "L") {
        mensaje = "La comunicacion I2C se encuentra ";
        mensaje += configLora.I2C ? "Activada" : "Desactivada";
        imprimirSerial(mensaje);
      } else if (accion == "0") {
        imprimirSerial("Desactivando la comunicacion I2C", 'y');
        configLora.I2C = false;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
      } else if (accion == "1") {
        imprimirSerial("Activando la comunicacion I2C");
        configLora.I2C = true;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
        ManejoComunicacion::initI2C();
      }
    
    } else if (comandoProcesar.startsWith("SCANI2C")) {
      if (configLora.I2C) { 
        imprimirSerial("Escaneando el puerto I2C en busca de dispositivos...", 'y');
        ManejoComunicacion::scannerI2C();
      } else {
        imprimirSerial("La comunicacion I2C esta desactivada, no se puede escanear", 'r');
      }
    }
  } else if (IDLora == String(configLora.IDLora) && comandoRecibido.substring(4, 5) == "V") {
    ManejoComunicacion::escribirVecinal(comandoRecibido);
  } else {
    imprimirSerial("Reenviando comando al nodo con el ID " + IDLora);
    ManejoComunicacion::envioMsjLoRa(IDLora);
  }
}
