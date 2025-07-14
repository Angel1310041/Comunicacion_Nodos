#include "interfaz.h"
#include "hardware.h"
#include "comunicacion.h"
#include "config.h"
#include "eeprom.h"

void Interfaz::entrarModoProgramacion() {
  modoProgramacion = true;
  esp_task_wdt_reset();
  digitalWrite(LED_STATUS, HIGH);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  xTaskCreatePinnedToCore(
    endpointsMProg,
    "Endpoints",
    8192,
    NULL,
    2,
    NULL,
    0
  );
  imprimirSerial("---# Modo Programacion Activado #---", 'g');
}

void endpointsMProg(void *pvParameters) {
  imprimirSerial("Modo Programacion Falso", 'b');
}
