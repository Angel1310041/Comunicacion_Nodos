#include "config.h"
#include "hardware.h"
#include "eeprom.h"
#include "comunicacion.h"

#define CANT_CONDICIONALES 12
#define CANT_ACCIONES 12

// Arreglo con los números de pin fisicos
const int pinNumbers[6] = {PIN_IO1, PIN_IO2, PIN_IO3, PIN_IO4, PIN_IO5, PIN_IO6};
// Arreglo con los nombres para mensajes
const char* pinNames[6] = {"IO1", "IO2", "IO3", "IO4", "IO5", "IO6"};

struct ParametroGPIO {
  char nombre;
  int pin;
  char flanco;
};

struct AccionGPIO {
  char nombre;
  int pin;
  char flanco;
};

struct CondicionalGPIO {
  int condicion;
  ParametroGPIO parametro[6];
  AccionGPIO accion;
};

CondicionalGPIO condicionales[CANT_CONDICIONALES];
AccionGPIO acciones[CANT_ACCIONES];

TaskHandle_t tareaEvaluarCondicionales = NULL;

void actualizarEstadoPinesGPIO() {

}

bool verificarFlanco(int pin, char flanco) {

}

AccionGPIO buscarAccion(int condicion) {

}

void ejecutarAccion(AccionGPIO ejecutar) {
  imprimirSerial("Ejecutando accion " + String(ejecutar.nombre), 'g');
  if (ejecutar.flanco == 'A') {
    digitalWrite(ejecutar.pin, HIGH);
  } else if (ejecutar.flanco == 'D') {
    digitalWrite(ejecutar.pin, LOW);
  }
}

void evaluarCondicionales(void *pvParameters) {
  imprimirSerial("Esperando condiciones GPIO...");
  tareaEvaluarCondicionales = xTaskGetCurrentTaskHandle();

  while(true) {
    actualizarEstadoPinesGPIO();
    for (int i = 0; i < CANT_CONDICIONALES; i++) {
      bool cumple = true;
      for (int j = 0; j < 6; j++) {
        int pin = condicionales[i].parametro[j].pin;
        char flanco = condicionales[i].parametro[j].flanco;
        if (!verificarFlanco(pin, flanco)) {
          cumple = false;
          break;
        }
      }
      if (cumple) {
        AccionGPIO accion = buscarAccion(condicionales[i].condicion);
        ejecutarAccion(accion);
      }
    }
  }
  vTaskDelete(NULL);
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

void Hardware::inicializar() {
  ManejoEEPROM::tarjetaNueva();
  pinMode(LED_STATUS, OUTPUT);
  pinMode(TEST_IN, INPUT);
  Hardware::configurarPinesGPIO(tarjeta.PinesGPIO, tarjeta.FlancosGPIO);

  if (!modoProgramacion && tareaEvaluarCondicionales == NULL) {
    imprimirSerial("Iniciando tarea de Evaluacion de Condicionales GPIO...", 'c');
    xTaskCreatePinnedToCore(
      evaluarCondicionales,
      "Condicionales GPIO",
      5120,
      NULL,
      1,
      &tareaEvaluarCondicionales,
      0
    );
    imprimirSerial("Tarea de Evaluacion de Condicionales iniciada", 'c');
  }
}
