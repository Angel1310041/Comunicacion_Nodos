#include "config.h"
#include "comunicacion.h"
#include "eeprom.h"
#include "hardware.h"
#include "interfaz.h"

SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
TwoWire I2CGral = TwoWire(1);

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
  if (tarjeta.UART) {
    Serial2.begin(9600, SERIAL_8N1, UART_RX, UART_TX); // RX=46, TX=45
    imprimirSerial("UART inicializado correctamente.", 'g');
  } else {
    imprimirSerial("UART inhabilitado", 'y');
  }
}

String ManejoComunicacion::leerVecinal() {
  if (tarjeta.UART) {
    String comandoVecinal = "";
    if (Serial2.available()) {
      comandoVecinal = Serial2.readStringUntil('\n');
      if (comandoVecinal != "") {
        imprimirSerial("Comando recibido en Serial2: " + comandoVecinal, 'y');
        return comandoVecinal;
      } else {
        imprimirSerial("Comando vacio");
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
  if (tarjeta.UART) {
    Serial2.println(envioVecinal);
    imprimirSerial("Comando enviado a la Alarma Vecinal:\n  -> " + envioVecinal, 'g');
  }
}

void ManejoComunicacion::initI2C() {
  if (tarjeta.I2C) {
    I2CGral.begin(I2C_SDA, I2C_SCL);
    imprimirSerial("I2C inicializado correctamente.", 'g');
  } else {
    imprimirSerial("I2C inhabilitado", 'y');
  }
}

void ManejoComunicacion::scannerI2C() {
  byte error, address;
  int nDevices;
  imprimirSerial("Escaneando puerto I2C...", 'y');

  String msjScan;
  nDevices = 0;
  for (address = 1; address< 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      msjScan = "I2C encontrado en la direccion 0x";
      if (address < 16) 
        msjScan = "0";
      msjScan += address, HEX;
      msjScan += "  !";
      imprimirSerial(msjScan);
      nDevices++;
    } else if (error == 4) {
      msjScan = "Error desconocido en la direccion 0x";
      if (address < 16)
        msjScan += "0";
      msjScan += address, HEX;
      imprimirSerial(msjScan);
    }
  }
  if (nDevices == 0)
    imprimirSerial("Ningun dispositivo I2C encontrado", 'y');
  else
    imprimirSerial("Escaneo completado.", 'g');
}

String ManejoComunicacion::leerSerial() {
  String comandoVecinal = "";
  if (Serial.available()) {
    comandoVecinal = Serial.readStringUntil('\n');
    if (comandoVecinal != "") {
      imprimirSerial("Comando recibido en Serial: " + comandoVecinal, 'y');
      return comandoVecinal;
    } else {
      imprimirSerial("Comando vacio");
      return "";
    }
  }
  return "";
}
