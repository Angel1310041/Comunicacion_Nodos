#include <Arduino.h>
#include "config.h"

LoRaConfig configLora;
Preferences preferences;

float canales[9] = {
  433.0, 433.2, 433.4, 433.6, 433.8, 434.0, 434.2, 434.4, 434.6
};

void guardarConfig() {
  preferences.begin("lora", false);
  preferences.putInt("id", configLora.IDLora);
  preferences.putInt("canal", configLora.Canal);
  preferences.end();
}

void cargarConfig() {
  preferences.begin("lora", true);
  configLora.IDLora = preferences.getInt("id", -1);
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

void pedirID() {
  configLora.IDLora = -1;
  while (configLora.IDLora < 0) {
    Serial.println("Introduce el ID de este nodo y pulsa ENTER:");
    while (!Serial.available()) delay(100);
    String input = Serial.readStringUntil('\n');
    input.trim();
    int tempID = input.toInt();
    if (tempID > 0) {
      configLora.IDLora = tempID;
      Serial.println("ID configurado: " + String(configLora.IDLora));
    } else {
      Serial.println("ID inválido. Debe ser un número mayor que 0.");
    }
    delay(500);
  }
}

void pedirCanal() {
  configLora.Canal = -1;
  while (configLora.Canal < 0 || configLora.Canal > 8) {
    Serial.println("Selecciona el canal (0-8) y pulsa ENTER:");
    for (int i = 0; i < 9; i++) {
      Serial.print("Canal ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(canales[i], 1);
      Serial.println(" MHz");
    }
    while (!Serial.available()) delay(100);
    String input = Serial.readStringUntil('\n');
    input.trim();
    int tempCanal = input.toInt();
    if (tempCanal >= 0 && tempCanal <= 8) {
      configLora.Canal = tempCanal;
      Serial.println("Canal configurado: " + String(configLora.Canal) + " (" + String(canales[configLora.Canal], 1) + " MHz)");
    } else {
      Serial.println("Canal inválido. Debe ser un número entre 0 y 8.");
    }
    delay(500);
  }
}
