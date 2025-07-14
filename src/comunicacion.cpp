#include "config.h"
#include "comunicacion.h"
#include "eeprom.h"
#include "hardware.h"
#include "interfaz.h"

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
        Hardware::blinkPin(LED_STATUS, 1, 500);
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
  if (tarjeta.UART) {
    Serial2.println(envioVecinal);
    imprimirSerial("Comando enviado a la Alarma Vecinal:\n  -> " + envioVecinal, 'g');
  } else {
    imprimirSerial("Comunicacion UART deshabilitada", 'y');
  }
}

void ManejoComunicacion::initI2C() {
  if (tarjeta.I2C) {
    //I2CGral.begin(I2C_SDA, I2C_SCL);
    Wire.begin(I2C_SDA, I2C_SCL);
    imprimirSerial("I2C inicializado correctamente.", 'g');
  } else {
    imprimirSerial("I2C inhabilitado", 'y');
  }
}

int ManejoComunicacion::scannerI2C() {
  byte error, address;
  int nDevices;
  int cantI2C = 0;
  imprimirSerial("Escaneando puerto I2C...\n", 'y');

  for (address = 1; address< 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      mensaje = "I2C encontrado en la direccion 0x";
      if (address < 16) 
        mensaje += "0";
      mensaje += String(address, HEX);
      mensaje += " !";
      cantI2C += 1;
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
    return cantI2C;
  } else {
    imprimirSerial("\nEscaneo completado!", 'g');
    return cantI2C;
  }
}

String ManejoComunicacion::leerSerial() {
  String comandoVecinal = "";
  if (Serial.available()) {
    comandoVecinal = Serial.readStringUntil('\n');
    if (comandoVecinal != "") {
      imprimirSerial("Comando recibido en Serial: " + comandoVecinal, 'y');
      Hardware::blinkPin(LED_STATUS, 1, 500);
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
  //String IDLora = comandoRecibido.substring(0, 3);
  int primerSep = comandoRecibido.indexOf('@');
  int segundoSep = comandoRecibido.indexOf('@', primerSep + 1);
  int tercerSep = comandoRecibido.indexOf('@', segundoSep + 1);
  String IDLora = comandoRecibido.substring(0, primerSep);
  String destino = comandoRecibido.substring(primerSep + 1, segundoSep);
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

  if (IDLora == String(tarjeta.IDLora) && /*comandoRecibido.substring(4, 5)*/ destino == "L") {
    if (comandoProcesar.startsWith("ID")) {
      // Ejemplo: IDC>AD9 / IDL
      accion = comandoProcesar.substring(2, 3);
      if (accion == "L") {
        imprimirSerial("El ID del nodo es -> " + String(tarjeta.IDLora), 'y');
      } else if (accion == "C") {
        prefix1 = comandoProcesar.substring(4, 7);
        imprimirSerial("Cambiando el ID del nodo a -> " + prefix1, 'c');
        strcpy(tarjeta.IDLora, prefix1.c_str());
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
      }

    } else if (comandoProcesar.startsWith("CH")) {
      // Ejemplo: CHL>1 / CHC>2
      accion = comandoProcesar.substring(3, 4);
      if (accion == "L") {
        imprimirSerial("El canal del nodo es -> " + String(tarjeta.Canal), 'y');
      } else if (accion == "C") {
        prefix1 = comandoProcesar.substring(4, 5);
        imprimirSerial("Cambiando el canal del nodo a -> " + prefix1, 'c');
        tarjeta.Canal = prefix1.toInt();
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
      }

    } else if (comandoProcesar.startsWith("SCR")) {
      // Ejemplo: SCR>L / SCR>1/0
      accion = comandoProcesar.substring(4, 5);
      if (accion == "L") {
        mensaje = "La pantalla de la LoRa se encuentra ";
        mensaje += tarjeta.Pantalla ? "Activada" : "Desactivada";
        imprimirSerial(mensaje);
      } else if (accion == "0") {
        imprimirSerial("Desactivando la pantalla");
        tarjeta.Pantalla = false;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
      } else if (accion == "1") {
        imprimirSerial("Activando la pantalla");
        tarjeta.Pantalla = true;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
      }

    } else if (comandoProcesar.startsWith("I2C")) {
      // Ejemplo: I2C>L / I2C>1/0
      accion = comandoProcesar.substring(4, 5);
      if (accion == "L") {
        mensaje = "La comunicacion I2C se encuentra ";
        mensaje += tarjeta.I2C ? "Activada" : "Desactivada";
        imprimirSerial(mensaje);
      } else if (accion == "0") {
        imprimirSerial("Desactivando la comunicacion I2C", 'y');
        tarjeta.I2C = false;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
      } else if (accion == "1") {
        imprimirSerial("Activando la comunicacion I2C");
        tarjeta.I2C = true;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
        ManejoComunicacion::initI2C();
      }
    
    } else if (comandoProcesar.startsWith("SCANI2C")) {
      if (tarjeta.I2C) { 
        imprimirSerial("Escaneando el puerto I2C en busca de dispositivos...", 'y');
        imprimirSerial("Se encontraron " + String(ManejoComunicacion::scannerI2C()) + " direcciones I2C");
      } else {
        imprimirSerial("La comunicacion I2C esta desactivada, no se puede escanear", 'r');
      }
    
    } else if (comandoProcesar.startsWith("UART")) {
      // Ejemplo: UART>L / UART>1/0
      accion = comandoProcesar.substring(5, 6);
      if (accion == "L") {
        mensaje = "La comunicacion UART se encuentra ";
        mensaje += tarjeta.UART ? "Activada" : "Desactivada";
        imprimirSerial(mensaje);
      } else if (accion == "0") {
        imprimirSerial("Desactivando la comunicacion UART");
        tarjeta.UART = false;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
      } else if (accion == "1") {
        imprimirSerial("Activando la comunicacion UART");
        tarjeta.UART = true;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
        ManejoComunicacion::initUART();
      }

    } else if (comandoProcesar.startsWith("WIFI")) {
      // Ejemplo: WIFI>L WIFI>1/0
      accion = comandoProcesar.substring(5, 6);
      if (accion == "L") {
        mensaje = "La conexion WiFi se encuentra ";
        mensaje += tarjeta.WiFi ? "Activada" : "Desactivada";
        imprimirSerial(mensaje);
      } else if (accion == "0") {
        imprimirSerial("Desactivando la comunicacion WiFi");
        tarjeta.WiFi = false;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
      } else if (accion == "1") {
        imprimirSerial("Activando la comunicacion WiFi");
        tarjeta.WiFi = true;
        ManejoEEPROM::guardarTarjetaConfigEEPROM();
        // Colocar la funcion de conexion a redes existentes
      }

    } else if (comandoProcesar.startsWith("RESET")) {
      imprimirSerial("Reiniciando la tarjeta LoRa...");
      delay(1000);
      ESP.restart();

    } else if (comandoProcesar.startsWith("MPROG")) {
      imprimirSerial("Entrando a modo programacion a traves de comando...");
      if (!modoProgramacion) {
        Interfaz::entrarModoProgramacion();
      }
    }

  } else if (IDLora == String(tarjeta.IDLora) && /*comandoRecibido.substring(4, 5)*/destino == "V") {
    ManejoComunicacion::escribirVecinal(comandoRecibido);
  } else {
    imprimirSerial("Reenviando comando al nodo con el ID " + IDLora);
    ManejoComunicacion::envioMsjLoRa(IDLora);
  }
}

/*
  //* <<< Comandos LoRa >>>

  //* ID@&@CMD@##
       ^^^   ^  ^
    ID Dirigido | Separador | Vecinal/Lora | Comando | Numeros al azar

  - IDC> / IDL> = Cambiar o leer el ID del Nodo LoRa
    |-> OK-ID>###
  - CHC> / CHL> = Cambiar o leer el canal LoRa
    |-> OK-CH>#
  - SCR>L/1/0 = Habilitar, deshabilitar o leer el estado de la pantalla de LoRa
    |-> OK-SCR>1/0
  - UART>L/1/0 = Habilitar, deshabilitar o leer el estado de la comunicación UART hacia la Alarma Vecinal
    |-> OK-UART>1/0
  - I2C>L/1/0 = Habilitar, deshabilitar o leer el estado del I2C
    |-> OK-I2C>1/0
  - SCANI2C = Escaneo de dispositivos I2C conectados
    |-> OK-SCANI2C># Cantidad de dispositivos encontrados
  - WIFI>L/1/0 = Habilitar, deshabilitar o leer el estado de la conexión WiFi
    |-> OK-WIFI>1/0
  - RESET = Reiniciar la tarjeta LoRa
    |-> OK-RESET
  - MPROG = Entra a modo programacion
    |-> OK-MPROG
  - GPIO>1EA1A = Configurar un pin GPIO como entrada con flanco ascendente o descendente, número de condicional y parámetro de condición
         ^^^^^^
        # PIn | Entrada | Flanco | Condicion | Parametro
    |-> OK-GPIO>1EA1A
  - GPIO>3SDU = Configurar un pin GPIO como salida con flanco asendente o descendente con resultado de salida
         ^^^^
        # Pin | Salida | Flanco | Resultado
    |-> OK-GPIO>3SDU
  - COND>1AyB / COND>1L = Configurar condicional con parametros o leer la formula de un condicional
    |-> OK-COND>1AyB

                  Parametros condicion
                        v
                    A   B   C   D   E   F 
                  1                       U
                  2                       V
  Condiciones ->  3                       W  <- Resultados
                  4                       X
                  5                       Y
                  6                       Z
*/
