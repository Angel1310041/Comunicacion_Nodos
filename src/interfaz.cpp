#include "main.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// --- Configuración del servidor ---
AsyncWebServer server(80);

// WiFi Access Point (configuración modificada)
IPAddress local_IP(192, 168, 1, 100);       // Nueva IP
IPAddress gateway(192, 168, 1, 1);          // Nueva puerta de enlace
IPAddress subnet(255, 255, 255, 0);         // Misma máscara
const char* ssidAP = "DispositivoIoT";      // Nuevo nombre de red
const char* passwordAP = "iotpassword123";  // Nueva contraseña

// --- Función para configurar los endpoints ---
void configurarEndpoints() {
    // Endpoint principal que sirve el archivo comprimido
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists("/interfaz.html.gz")) {
            request->send(404, "text/plain", "Error: interfaz.html.gz no encontrado");
            return;
        }
        
        // Configurar la respuesta con compresión gzip
        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/interfaz.html.gz", "text/html");
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    // Puedes añadir más endpoints aquí si es necesario
}

// --- Función para iniciar el modo programación ---
void iniciarModoProgramacion() {
    Serial.println("Iniciando modo configuración...");
    
    // Inicializar SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Error al montar SPIFFS");
        return;
    }

    // Configurar WiFi como AP
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("Error en configuración de red");
        return;
    }
    
    if (!WiFi.softAP(ssidAP, passwordAP)) {
        Serial.println("Error al iniciar el punto de acceso");
        return;
    }

    Serial.print("Punto de acceso iniciado. SSID: ");
    Serial.println(ssidAP);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    // Configurar los endpoints del servidor
    configurarEndpoints();
    server.begin();
    Serial.println("Servidor web iniciado");
}