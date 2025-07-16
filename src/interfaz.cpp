#include "interfaz.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include "pantalla.h"

AsyncWebServer server(80);

const char* ssidAP = "DispositivoIoT";
const char* passwordAP = "iotpassword123";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

const char* htmlPath = "/interfaz.html.gz";
const char* cssPath = "/estilos.css.gz";
const char* jsPath = "/script.js.gz";

// --- EEPROM CONFIG ---
#define EEPROM_SIZE 2048
#define MAX_NETWORKS 10
#define SSID_MAXLEN 32
#define PASS_MAXLEN 64

struct SavedNetwork {
    char ssid[SSID_MAXLEN];
    char password[PASS_MAXLEN];
};

struct WifiConfig {
    SavedNetwork networks[MAX_NETWORKS];
    uint8_t count;
    int8_t preferred; // índice de la red preferida, -1 si ninguna
};

WifiConfig wifiConfig;

// --- EEPROM helpers ---
void loadWifiConfig() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0, wifiConfig);
    // Validación básica
    if (wifiConfig.count > MAX_NETWORKS) wifiConfig.count = 0;
    if (wifiConfig.preferred >= MAX_NETWORKS) wifiConfig.preferred = -1;
}

void saveWifiConfig() {
    EEPROM.put(0, wifiConfig);
    EEPROM.commit();
}

void addOrUpdateNetwork(const String& ssid, const String& password) {
    // Si ya existe, actualiza
    for (uint8_t i = 0; i < wifiConfig.count; ++i) {
        if (ssid == wifiConfig.networks[i].ssid) {
            strncpy(wifiConfig.networks[i].password, password.c_str(), PASS_MAXLEN);
            wifiConfig.networks[i].password[PASS_MAXLEN-1] = '\0';
            saveWifiConfig();
            return;
        }
    }
    // Si hay espacio, agrega
    if (wifiConfig.count < MAX_NETWORKS) {
        strncpy(wifiConfig.networks[wifiConfig.count].ssid, ssid.c_str(), SSID_MAXLEN);
        wifiConfig.networks[wifiConfig.count].ssid[SSID_MAXLEN-1] = '\0';
        strncpy(wifiConfig.networks[wifiConfig.count].password, password.c_str(), PASS_MAXLEN);
        wifiConfig.networks[wifiConfig.count].password[PASS_MAXLEN-1] = '\0';
        wifiConfig.count++;
        saveWifiConfig();
    }
}

void deleteNetwork(const String& ssid) {
    for (uint8_t i = 0; i < wifiConfig.count; ++i) {
        if (ssid == wifiConfig.networks[i].ssid) {
            // Mueve las siguientes hacia arriba
            for (uint8_t j = i; j < wifiConfig.count - 1; ++j) {
                wifiConfig.networks[j] = wifiConfig.networks[j + 1];
            }
            wifiConfig.count--;
            if (wifiConfig.preferred == i) wifiConfig.preferred = -1;
            else if (wifiConfig.preferred > i) wifiConfig.preferred--;
            saveWifiConfig();
            return;
        }
    }
}

void clearNetworks() {
    wifiConfig.count = 0;
    wifiConfig.preferred = -1;
    saveWifiConfig();
}

void setPreferredNetwork(const String& ssid) {
    for (uint8_t i = 0; i < wifiConfig.count; ++i) {
        if (ssid == wifiConfig.networks[i].ssid) {
            wifiConfig.preferred = i;
            saveWifiConfig();
            return;
        }
    }
}

// --- Conexión automática ---
bool connectToPreferredNetwork() {
    if (wifiConfig.count == 0) return false;
    int8_t idx = wifiConfig.preferred;
    if (idx < 0 || idx >= wifiConfig.count) idx = 0; // Si no hay preferida, usa la primera
    String ssid = wifiConfig.networks[idx].ssid;
    String password = wifiConfig.networks[idx].password;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long startAttemptTime = millis();
    bool connected = false;
    while (millis() - startAttemptTime < 8000) {
        if (WiFi.status() == WL_CONNECTED) {
            connected = true;
            break;
        }
        delay(200);
    }
    if (connected) {
        Serial.print("Conectado a red preferida: ");
        Serial.println(ssid);
        return true;
    } else {
        Serial.println("No se pudo conectar a la red preferida.");
        WiFi.disconnect(true);
        return false;
    }
}

// --- Endpoints ---
void configurarEndpoints() {
    // Página principal
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists(htmlPath)) {
            request->send(404, "text/plain", "Archivo interfaz no encontrado");
            return;
        }
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, htmlPath, "text/html");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // CSS
    server.on("/estilos.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists(cssPath)) {
            request->send(404, "text/plain", "CSS no encontrado");
            return;
        }
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, cssPath, "text/css");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // JS
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists(jsPath)) {
            request->send(404, "text/plain", "JS no encontrado");
            return;
        }
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, jsPath, "application/javascript");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // Config info
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        String jsonResponse = "{\"ssid\":\"" + String(ssidAP) + "\",\"ip\":\"" + WiFi.softAPIP().toString() + "\"}";
        request->send(200, "application/json", jsonResponse);
    });

    // Escanear redes WiFi disponibles
    server.on("/redesDisponibles", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(2048);
        JsonArray arr = doc.createNestedArray("networks");
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; ++i) {
            JsonObject net = arr.createNestedObject();
            net["ssid"] = WiFi.SSID(i);
            net["security"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "Abierta" : "WPA/WPA2";
            int rssi = WiFi.RSSI(i);
            int intensity = constrain(map(rssi, -90, -30, 1, 4), 1, 4);
            net["intensity"] = intensity;
        }
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // Listar redes guardadas
    server.on("/redesGuardadas", HTTP_GET, [](AsyncWebServerRequest *request) {
        DynamicJsonDocument doc(2048);
        JsonArray arr = doc.createNestedArray("networks");
        for (uint8_t i = 0; i < wifiConfig.count; ++i) {
            JsonObject obj = arr.createNestedObject();
            obj["ssid"] = wifiConfig.networks[i].ssid;
            obj["password"] = wifiConfig.networks[i].password;
            obj["preferred"] = (wifiConfig.preferred == i);
        }
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // Guardar una red (POST)
    server.on("/guardarRed", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
            request->send(400, "application/json", "{\"error\":\"Faltan parámetros\"}");
            return;
        }
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        addOrUpdateNetwork(ssid, password);
        request->send(200, "application/json", "{\"mensaje\": \"Red guardada\"}");
    });

    // Conectar a una red (POST) - Modificado para no validar contraseña
    server.on("/conectarRed", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true)) {
            request->send(400, "application/json", "{\"error\":\"Falta el SSID\"}");
            return;
        }
        
        String ssid = request->getParam("ssid", true)->value();
        String password = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";
        
        // Simplemente devolvemos éxito sin validar
        request->send(200, "application/json", "{\"connected\": true}");
    });

    // Actualizar red guardada (POST)
    server.on("/actualizarRed", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
            request->send(400, "application/json", "{\"error\":\"Faltan parámetros\"}");
            return;
        }
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        addOrUpdateNetwork(ssid, password);
        request->send(200, "application/json", "{\"mensaje\": \"Red actualizada\"}");
    });

    // Borrar una red guardada (POST)
    server.on("/borrarRed", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true)) {
            request->send(400, "application/json", "{\"error\":\"Falta el SSID\"}");
            return;
        }
        String ssid = request->getParam("ssid", true)->value();
        deleteNetwork(ssid);
        request->send(200, "application/json", "{\"mensaje\": \"Red guardada eliminada\"}");
    });

    // Borrar todas las redes guardadas
    server.on("/borrarRedes", HTTP_GET, [](AsyncWebServerRequest *request) {
        clearNetworks();
        request->send(200, "application/json", "{\"mensaje\": \"Redes guardadas eliminadas\"}");
    });

    // Marcar red preferida (POST)
    server.on("/preferirRed", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true)) {
            request->send(400, "application/json", "{\"error\":\"Falta el SSID\"}");
            return;
        }
        String ssid = request->getParam("ssid", true)->value();
        setPreferredNetwork(ssid);
        request->send(200, "application/json", "{\"mensaje\": \"Red preferida actualizada\"}");
    });

    // Control de pantalla
    server.on("/pantalla/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        pantallaControl("on");
        request->send(200, "application/json", "{\"pantalla\": \"on\"}");
    });
    server.on("/pantalla/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        pantallaControl("off");
        request->send(200, "application/json", "{\"pantalla\": \"off\"}");
    });

    // Reinicio de la placa
    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Reiniciando...");
        delay(500);
        ESP.restart();
    });

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Página no encontrada");
    });
}

void iniciarModoProgramacion() {
    Serial.println("Iniciando modo configuración...");

    if (!SPIFFS.begin(true)) {
        Serial.println("Error al montar SPIFFS");
        pantallaControl("", "Error SPIFFS", "Reiniciar");
        return;
    }

    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("Error al iniciar EEPROM");
        pantallaControl("", "Error EEPROM", "Reiniciar");
        return;
    }

    loadWifiConfig();

    // Intentar conectar a la red preferida
    bool conectado = connectToPreferredNetwork();

    // Siempre inicia el AP para configuración
    if (!WiFi.mode(WIFI_AP)) {
        Serial.println("Error al configurar modo AP");
        pantallaControl("", "Error WiFi", "Modo AP");
        return;
    }
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("Error en configuración de red");
        pantallaControl("", "Error Red", "Config IP");
        return;
    }
    if (!WiFi.softAP(ssidAP, passwordAP)) {
        Serial.println("Error al iniciar AP");
        pantallaControl("", "Error AP", "Reiniciar");
        return;
    }

    String ip = WiFi.softAPIP().toString();
    String mensaje = "SSID: " + String(ssidAP) + "\nPass: " + String(passwordAP) + "\nIP: " + ip;
    pantallaControl("", "MODO CONFIG", mensaje.c_str());

    Serial.println("Punto de acceso iniciado:");
    Serial.print("SSID: ");
    Serial.println(ssidAP);
    Serial.print("Contraseña: ");
    Serial.println(passwordAP);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    configurarEndpoints();
    server.begin();
    Serial.println("Servidor web iniciado");
}