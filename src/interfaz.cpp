#include "interfaz.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
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

struct SavedNetwork {
    String ssid;
    String password;
};
std::vector<SavedNetwork> savedNetworks;

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

    // Salir modo programación (solo respuesta, el reinicio es en /restart)
    server.on("/exit", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Saliendo modo programación");
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
        DynamicJsonDocument doc(1024);
        JsonArray arr = doc.createNestedArray("networks");
        for (const auto& net : savedNetworks) {
            JsonObject obj = arr.createNestedObject();
            obj["ssid"] = net.ssid;
            obj["password"] = net.password;
        }
        String json;
        serializeJson(doc, json);
        request->send(200, "application/json", json);
    });

    // Guardar una red (POST) - SOLO GUARDA, NO CONECTA
    server.on("/guardarRed", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
            request->send(400, "application/json", "{\"error\":\"Faltan parámetros\"}");
            return;
        }
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        auto it = std::find_if(savedNetworks.begin(), savedNetworks.end(), [&](const SavedNetwork& n){ return n.ssid == ssid; });
        if (it == savedNetworks.end()) {
            savedNetworks.push_back({ssid, password});
        } else {
            it->password = password;
        }
        request->send(200, "application/json", "{\"mensaje\": \"Red guardada\"}");
    });

    // Conectar a una red guardada (POST)
    server.on("/conectarRed", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true)) {
            request->send(400, "application/json", "{\"error\":\"Falta el SSID\"}");
            return;
        }
        String ssid = request->getParam("ssid", true)->value();
        String password = "";
        auto it = std::find_if(savedNetworks.begin(), savedNetworks.end(), [&](const SavedNetwork& n){ return n.ssid == ssid; });
        if (it != savedNetworks.end()) {
            password = it->password;
        } else {
            request->send(404, "application/json", "{\"error\": \"Red no encontrada\"}");
            return;
        }
        WiFi.mode(WIFI_STA);
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
            request->send(200, "application/json", "{\"mensaje\": \"Conectado a la red\", \"connected\": true}");
        } else {
            WiFi.disconnect();
            request->send(200, "application/json", "{\"mensaje\": \"No se pudo conectar\", \"connected\": false}");
        }
    });

    // Actualizar red guardada (POST)
    server.on("/actualizarRed", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
            request->send(400, "application/json", "{\"error\":\"Faltan parámetros\"}");
            return;
        }
        String ssid = request->getParam("ssid", true)->value();
        String password = request->getParam("password", true)->value();
        auto it = std::find_if(savedNetworks.begin(), savedNetworks.end(), [&](const SavedNetwork& n){ return n.ssid == ssid; });
        if (it != savedNetworks.end()) {
            it->password = password;
            request->send(200, "application/json", "{\"mensaje\": \"Red actualizada\"}");
        } else {
            request->send(404, "application/json", "{\"error\": \"Red no encontrada\"}");
        }
    });

    // Borrar una red guardada (POST)
    server.on("/borrarRed", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid", true)) {
            request->send(400, "application/json", "{\"error\":\"Falta el SSID\"}");
            return;
        }
        String ssid = request->getParam("ssid", true)->value();
        savedNetworks.erase(std::remove_if(savedNetworks.begin(), savedNetworks.end(),
            [&](const SavedNetwork& n){ return n.ssid == ssid; }), savedNetworks.end());
        request->send(200, "application/json", "{\"mensaje\": \"Red guardada eliminada\"}");
    });

    // Borrar todas las redes guardadas
    server.on("/borrarRedes", HTTP_GET, [](AsyncWebServerRequest *request) {
        savedNetworks.clear();
        request->send(200, "application/json", "{\"mensaje\": \"Redes guardadas eliminadas\"}");
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