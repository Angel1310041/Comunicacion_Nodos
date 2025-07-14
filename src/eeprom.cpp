#include <Arduino.h>
#include "eeprom.h"

void guardarConfig() {
  preferences.begin("lora", false);
  preferences.putString("id", configLora.IDLora); // Guardar como string
  preferences.putInt("canal", configLora.Canal);
  preferences.end();
}

void cargarConfig() {
  preferences.begin("lora", true);
  String id = preferences.getString("id", "");
  strncpy(configLora.IDLora, id.c_str(), sizeof(configLora.IDLora) - 1);
  configLora.IDLora[sizeof(configLora.IDLora) - 1] = '\0'; // Null-terminate
  configLora.Canal = preferences.getInt("canal", -1);
  preferences.end();
}

void borrarConfig() {
  preferences.begin("lora", false);
  preferences.clear();
  preferences.end();
  Serial.println("Configuración borrada. Reinicia el dispositivo o espera...");
  delay(1000);
  ESP.restart();
}