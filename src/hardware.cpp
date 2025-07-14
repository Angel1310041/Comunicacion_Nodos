#include "config.h"
#include "hardware.h"
#include "eeprom.h"

// Arreglo con los números de pin fisicos
const int pinNumbers[6] = {PIN_IO1, PIN_IO2, PIN_IO3, PIN_IO4, PIN_IO5, PIN_IO6};
// Arreglo con los nombres para mensajes
const char* pinNames[6] = {"IO1", "IO2", "IO3", "IO4", "IO5", "IO6"};

void Hardware::inicializar() {
  ManejoEEPROM::tarjetaNueva();
  pinMode(LED_STATUS, OUTPUT);
  pinMode(TEST_IN, INPUT);
  Hardware::configurarPinesGPIO(configLora.PinesGPIO, configLora.FlancosGPIO);
}

void Hardware::configurarPinesGPIO(char PinesGPIO[6], char Flancos[6]) {
  String mensajePines = ""; // Variable para armar mensaje personalizado
  for (int i = 0; i < 6; ++i) { // Bucle for para configurar los pines GPIO
    if (PinesGPIO[i] == 'E') { // Condicional para entradas
      mensajePines = "Pin " + String(pinNames[i]) + " configurado como entrada con flanco ";
      if (Flancos[i] == 'A') {
        mensajePines += "ascendente"; // Activacion con pulsos altos
      } else if (Flancos[i] == 'D') {
        mensajePines += "descendente"; // Activacion con pulsos bajos
      } else {
        mensajePines += "no especificado"; // Sin especificar
      }
      imprimirSerial(mensajePines, 'b');
      pinMode(pinNumbers[i], INPUT);

    } else if (PinesGPIO[i] == 'S') { // Condicional para salidas
      mensajePines = "Pin " + String(pinNames[i]) + " configurado como entrada con flanco ";
      if (Flancos[i] == 'A') {
        mensajePines += "ascendente"; // Salida activa con pulso alto
      } else if (Flancos[i] == 'D') {
        mensajePines += "descendente"; // Salida activa con pulso bajo
      } else {
        mensajePines += "no especificado"; // Salida con pulso indefinido
      }
      imprimirSerial(mensajePines, 'b');
      pinMode(pinNumbers[i], OUTPUT);

    } else { // Condicional para pines no especificados
      imprimirSerial("Pin " + String(pinNames[i]) + " no especificado", 'y');
    }
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
