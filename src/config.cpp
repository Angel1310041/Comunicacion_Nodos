#include <Arduino.h>
#include "config.h"
#include "eeprom.h"

extern LoRaConfig configLora;
Preferences preferences;

float canales[9] = {
  433.0, 433.2, 433.4, 433.6, 433.8, 434.0, 434.2, 434.4, 434.6
};
