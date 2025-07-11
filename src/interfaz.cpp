#include "main.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Ticker.h>
#include <EEPROM.h>

// --- Definiciones ---
AsyncWebServer server(80);

// WiFi Access Point
IPAddress local_IP(192, 168, 8, 28);
IPAddress gateway(192, 168, 8, 1);
IPAddress subnet(255, 255, 255, 0);
const char* ssidAP = "Meserito-ABM";
const char* passwordAP = "12345678";

Ticker restartTimer;

// --- Funciones Auxiliares ---
void animacionCarga() {
    const char* estados[] = {"-", "\\", "|", "/"};
    for (int i = 0; i < 10; i++) {
        Serial.print("\rCargando... ");
        Serial.print(estados[i % 4]);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
    Serial.println("\rCargando... ¡Listo!");
}

void programarReinicio() {
    restartTimer.once(1.0, []() {
        ESP.restart();
    });
}





// --- Endpoints del Servidor Web ---
void configurarEndpoints() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists("/interfaz.html.gz")) {
            request->send(404, "text/plain", "Archivo no encontrado");
            return;
        }
        auto* response = request->beginResponse(SPIFFS, "/interfaz.html.gz", "text/html");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server.on("/guardar-parametros", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if (error) {
            request->send(400, "application/json", "{\"error\":\"JSON inválido\"}");
            return;
        }

        int id = doc["order-id"];
        const char* tipoTexto = doc["selected-food-item"];
        const char* estadoTexto = doc["attention-state"];

        if (id < 1 || id > 99 || strlen(tipoTexto) == 0 || strlen(estadoTexto) == 0) {
            request->send(400, "application/json", "{\"error\":\"Parámetros inválidos\"}");
            return;
        }

        guardarOrdenEnEEPROM(id, tipoTexto, estadoTexto);

        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/get-parametros", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["id"] = ordenActual.id;
        doc["tipo"] = ordenActual.tipo;
        doc["estado"] = ordenActual.estado;
        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    server.on("/enviar-lora", HTTP_POST, [](AsyncWebServerRequest *request) {}, nullptr,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t, size_t) {
            JsonDocument doc;
            if (deserializeJson(doc, data, len)) {
                request->send(400, "application/json", "{\"error\":\"JSON inválido\"}");
                return;
            }
            mensajePendiente = doc["mensaje"].as<String>();
            enviarLoraPendiente = true;
            request->send(200, "application/json", "{\"status\":\"Mensaje en cola\"}");
        });

    server.on("/reiniciar", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Reiniciando...");
        programarReinicio();
    });

    server.on("/salir-modo-prog", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Saliendo del modo programación...");
        modoprog = false;
        digitalWrite(LED_PIN, LOW);
        programarReinicio();
    });
}

void iniciarModoProgramacion() {
    Serial.println("Iniciando modo programación...");
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("Error al configurar IP estática");
        return;
    }
    if (!WiFi.softAP(ssidAP, passwordAP)) {
        Serial.println("Error al iniciar el AP");
        return;
    }
    Serial.println("AP iniciado en: " + WiFi.softAPIP().toString());
    configurarEndpoints();
    server.begin();
}

void entrarmodoprog() {
    digitalWrite(LED_PIN, HIGH);
   // digitalWrite(LORA_LED, HIGH);
    
    // Inicializa SPIFFS para los archivos web
    if (!SPIFFS.begin(true)) {
        imprimir("Error montando SPIFFS", "rojo");
        return;
    }

    // Carga datos de EEPROM
    EEPROM.begin(512);
    cargarOrdenDesdeEEPROM(true); // true para imprimir los datos

    // Configuración WiFi AP con IP estática
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        imprimir("Error configurando IP", "rojo");
        return;
    }
    
    if (!WiFi.softAP(ssidAP, passwordAP)) {
        imprimir("Error iniciando AP", "rojo");
        return;
    }

    // Configura endpoints del servidor web
    configurarEndpoints();
    server.begin();

    imprimir("Modo Programación", "amarillo");
    imprimir("Conéctese a:", "amarillo");
    imprimir("Red: Meserito-ABM", "amarillo");
    imprimir("IP: " + WiFi.softAPIP().toString(), "amarillo");
}