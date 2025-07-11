#include "interfaz.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "pantalla.h"

// Configuración del servidor
AsyncWebServer server(80);

// WiFi AP config
const char* ssidAP = "DispositivoIoT";
const char* passwordAP = "iotpassword123";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Archivos estáticos comprimidos con gzip
const char* htmlPath = "/interfaz.html.gz";
const char* cssPath = "/estilos.css.gz";
const char* jsPath = "/script.js.gz";

void configurarEndpoints() {
    // Endpoint raíz (HTML comprimido)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists(htmlPath)) {
            request->send(404, "text/plain", "Archivo interfaz no encontrado");
            return;
        }
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, htmlPath, "text/html");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // CSS comprimido
    server.on("/estilos.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists(cssPath)) {
            request->send(404, "text/plain", "CSS no encontrado");
            return;
        }
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, cssPath, "text/css");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // JavaScript comprimido
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists(jsPath)) {
            request->send(404, "text/plain", "JS no encontrado");
            return;
        }
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, jsPath, "application/javascript");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // Endpoint para obtener configuración
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        String jsonResponse = "{\"ssid\":\"" + String(ssidAP) + "\",\"ip\":\"" + WiFi.softAPIP().toString() + "\"}";
        request->send(200, "application/json", jsonResponse);
    });

    // Endpoint para salir del modo programación
    server.on("/exit", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", "Saliendo modo programación");
        //salirModoProgramacion();
    });

    // Manejo de errores 404
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Página no encontrada");
    });
}

void iniciarModoProgramacion() {
    Serial.println("Iniciando modo configuración...");
    
    // Inicializar SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Error al montar SPIFFS");
        pantallaControl("", "Error SPIFFS", "Reiniciar");
        return;
    }

    // Configurar modo AP
    if (!WiFi.mode(WIFI_AP)) {
        Serial.println("Error al configurar modo AP");
        pantallaControl("", "Error WiFi", "Modo AP");
        return;
    }

    // Configurar red AP
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("Error en configuración de red");
        pantallaControl("", "Error Red", "Config IP");
        return;
    }

    // Iniciar punto de acceso
    if (!WiFi.softAP(ssidAP, passwordAP)) {
        Serial.println("Error al iniciar AP");
        pantallaControl("", "Error AP", "Reiniciar");
        return;
    }

    // Mostrar información en pantalla
    String ip = WiFi.softAPIP().toString();
    String mensaje = "SSID: " + String(ssidAP) + "\nPass: " + String(passwordAP) + "\nIP: " + ip;
    pantallaControl("", "MODO CONFIG", mensaje.c_str());

    // Información por serial
    Serial.println("Punto de acceso iniciado:");
    Serial.print("SSID: ");
    Serial.println(ssidAP);
    Serial.print("Contraseña: ");
    Serial.println(passwordAP);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    // Configurar endpoints del servidor web
    configurarEndpoints();
    
    // Iniciar servidor
    server.begin();
    Serial.println("Servidor web iniciado");
}