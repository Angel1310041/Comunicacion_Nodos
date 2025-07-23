#include <Arduino.h>
#include "eeprom.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#define WIFI_NAMESPACE "WIFI_NETS"
#define WIFI_MAX 3


extern LoRaConfig configLora;
extern Network redes[3];

Preferences eeprom;
const int EEPROM_SIZE = 512;
const int DIRECCION_INICIO_CONFIG = 0;


void ManejoEEPROM::leerTarjetaEEPROM() {
  eeprom.begin("EEPROM_PPAL", false);
  configLora.magic = eeprom.getInt("magic");
  String idLeido = eeprom.getString("IDLora");
  strcpy(configLora.IDLora, idLeido.c_str());
  configLora.Canal = eeprom.getInt("Canal");
  configLora.Pantalla = eeprom.getBool("Pantalla");
  configLora.UART = eeprom.getBool("UART");
  configLora.I2C = eeprom.getBool("I2C");
  configLora.WiFi = eeprom.getBool("WiFi");
  configLora.DEBUG = eeprom.getBool("DEBUG");
  String GPIOS = eeprom.getString("PinesGPIO");
  strcpy(configLora.PinesGPIO, GPIOS.c_str());
  String FLANCOS = eeprom.getString("FlancosGPIO");
  strcpy(configLora.FlancosGPIO, FLANCOS.c_str());
  eeprom.end();
}

void ManejoEEPROM::guardarTarjetaConfigEEPROM() {
  eeprom.begin("EEPROM_PPAL", false);
  eeprom.putInt("magic", configLora.magic);
  eeprom.putString("IDLora", configLora.IDLora);
  eeprom.putInt("Canal", configLora.Canal);
  eeprom.putBool("Pantalla", configLora.Pantalla);
  eeprom.putBool("UART", configLora.UART);
  eeprom.putBool("I2C", configLora.I2C);
  eeprom.putBool("WiFi", configLora.WiFi);
  eeprom.putBool("DEBUG", configLora.DEBUG);
  eeprom.putString("PinesGPIO", configLora.PinesGPIO);
  eeprom.putString("FlancosGPIO", configLora.FlancosGPIO);
  eeprom.end();

  imprimirSerial("Configuracion guardada: ");
  imprimirSerial("ID: " + String(configLora.IDLora));
  imprimirSerial("Canal: " + String(configLora.Canal));
  imprimirSerial("Pantalla: " + String(configLora.Pantalla));
  imprimirSerial("UART: " + String(configLora.UART));
  imprimirSerial("DEBUG: " + String(configLora.DEBUG));
  imprimirSerial("I2C: " + String(configLora.I2C));

  for (int i = 0; i < 6; ++i) {
    if (configLora.PinesGPIO[i] == 1) {
      imprimirSerial("o- Pin " + String(pinNames[i]) + " configurado como entrada", 'c');
    } else if (configLora.PinesGPIO[i] == 2) {
      imprimirSerial("o- Pin " + String(pinNames[i]) + " configurado como salida", 'c');
    } else {
      imprimirSerial("o- Pin " + String(pinNames[i]) + " no especificado", 'y');
    }
  }
}

void ManejoEEPROM::borrarTarjetaConfigEEPROM() {
    eeprom.begin("EEPROM_PPAL", false);
    eeprom.clear();
    eeprom.end();
    Serial.println("Configuración borrada. Reinicia el dispositivo o espera...");
    delay(1000);
    ESP.restart();
}

void ManejoEEPROM::tarjetaNueva() {
  ManejoEEPROM::leerTarjetaEEPROM();
  if (configLora.magic != 0xDEADBEEF) {
    imprimirSerial("Esta tarjeta es nueva, comenzando formateo...", 'c');
    configLora.magic = 0xDEADBEEF;
    strcpy(configLora.IDLora, "001");
    configLora.Canal = 0;
    configLora.Pantalla = false;
    configLora.UART = true;
    configLora.I2C = false;
    configLora.DEBUG = true;
    //strcpy(configLora.PinesGPIO, "IIIIII");
    //strcpy(configLora.FlancosGPIO, "NNNNNN");

    ManejoEEPROM::guardarTarjetaConfigEEPROM();

    imprimirSerial("\n\t\t\t<<< Tarjeta formateada correctamente >>>", 'g');
    imprimirSerial("Version de la tarjeta: " + Version);
  } else {
    imprimirSerial("\n\t\t\t<<< Tarjeta lista para utilizarse >>>", 'y');
  }
}

bool ManejoEEPROM::guardarWiFiCredenciales(const char* ssid, const char* password) {
    Preferences prefs;
    prefs.begin(WIFI_NAMESPACE, false);

    // Buscar si ya existe la red o el primer slot vacío
    int idx = -1;
    for (int i = 0; i < WIFI_MAX; ++i) {
        String key = "ssid_" + String(i);
        String storedSsid = prefs.getString(key.c_str(), "");
        if (storedSsid == ssid) {
            idx = i;
            break;
        }
        if (idx == -1 && storedSsid.length() == 0) {
            idx = i; // Primer slot vacío
        }
    }

    if (idx == -1) {
        prefs.end();
        return false; // No hay espacio
    }

    String ssidKey = "ssid_" + String(idx);
    String passKey = "pass_" + String(idx);
    prefs.putString(ssidKey.c_str(), ssid);
    prefs.putString(passKey.c_str(), password);

    prefs.end();
    return true;
}

void ManejoEEPROM::obtenerRedesGuardadas(JsonArray& redesJson) {
    Preferences prefs;
    prefs.begin(WIFI_NAMESPACE, true);

    for (int i = 0; i < WIFI_MAX; ++i) {
        String ssidKey = "ssid_" + String(i);
        String ssid = prefs.getString(ssidKey.c_str(), "");
        if (ssid.length() > 0) {
            JsonObject red = redesJson.createNestedObject();
            red["ssid"] = ssid;
            // Por seguridad, no se retorna el password
        }
    }

    prefs.end();
}

bool ManejoEEPROM::eliminarRedWiFi(const char* ssid) {
    Preferences prefs;
    prefs.begin(WIFI_NAMESPACE, false);

    bool found = false;
    for (int i = 0; i < WIFI_MAX; ++i) {
        String ssidKey = "ssid_" + String(i);
        String passKey = "pass_" + String(i);
        String storedSsid = prefs.getString(ssidKey.c_str(), "");
        if (storedSsid == ssid) {
            prefs.remove(ssidKey.c_str());
            prefs.remove(passKey.c_str());
            found = true;
            break;
        }
    }

    prefs.end();
    return found;
}
