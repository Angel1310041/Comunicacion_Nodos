#include "config.h"
#include "hardware.h"
#include "eeprom.h"

// Arreglo con los números de pin fisicos
const int pinNumbers[6] = {PIN_IO1, PIN_IO2, PIN_IO3, PIN_IO4, PIN_IO5, PIN_IO6};
// Arreglo con los nombres para mensajes
const char* pinNames[6] = {"IO1", "IO2", "IO3", "IO4", "IO5", "IO6"};

void Hardware::inicializar() {
  Hardware::tarjetaNueva();
  pinMode(LED_STATUS, OUTPUT);
  pinMode(TEST_IN, INPUT);
  Hardware::configurarPinesGPIO(pinNumbers);
}

void Hardware::configurarPinesGPIO(const int PinesGPIO[6]) {
  // Suponiendo que tarjeta.PinesGPIO es un arreglo de 6 elementos
  for (int i = 0; i < 6; ++i) {
    if (PinesGPIO[i] == 1) {
      imprimirSerial("Pin " + String(pinNames[i]) + " configurado como entrada", 'b');
      pinMode(pinNumbers[i], INPUT);
    } else if (PinesGPIO[i] == 2) {
      imprimirSerial("Pin " + String(pinNames[i]) + " configurado como salida", 'b');
      pinMode(pinNumbers[i], OUTPUT);
    } else {
      imprimirSerial("Pin " + String(pinNames[i]) + " no especificado", 'y');
    }
  }
}

void Hardware::tarjetaNueva() {
  ManejoEEPROM::leerTarjetaEEPROM();
  if (tarjeta.magic != 0xDEADBEEF) {
    imprimirSerial("Esta tarjeta es nueva, comenzando formateo...", 'c');
    tarjeta.magic = 0xDEADBEEF;
    strcpy(tarjeta.IDLora, "001");
    tarjeta.Canal = 1;
    tarjeta.Pantalla = false;
    tarjeta.UART = true;
    tarjeta.I2C = false;
    tarjeta.PinesGPIO[0] = 0;
    tarjeta.PinesGPIO[1] = 0;
    tarjeta.PinesGPIO[2] = 0;
    tarjeta.PinesGPIO[3] = 0;
    tarjeta.PinesGPIO[4] = 0;
    tarjeta.PinesGPIO[5] = 0;

    ManejoEEPROM::guardarTarjetaConfigEEPROM();

    imprimirSerial("\n\t\t\t<<< Tarjeta formateada correctamente >>>", 'g');
    imprimirSerial("Version de la tarjeta: " + Version);
  } else {
    imprimirSerial("\n\t\t\t<<< Tarjeta formateada lista para utilizarse >>>", 'y');
  }
}

void Hardware::blinkPin(int pin, int times, int delayTime) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    vTaskDelay(delayTime / portTICK_PERIOD_MS);
    digitalWrite(pin, LOW);
    vTaskDelay(delayTime / portTICK_PERIOD_MS);
  }
}

void Hardware::manejoEstrobo(int pin, int freq, int delayTime) {
  int tiempo = delayTime * 1000;
  int ciclos = tiempo / (2 * freq);

  for (int i = 0; i < ciclos; i++) {
    digitalWrite(pin, HIGH);
    vTaskDelay(delayTime / portTICK_PERIOD_MS);
    digitalWrite(pin, LOW);
    vTaskDelay(delayTime / portTICK_PERIOD_MS);
  }
}
