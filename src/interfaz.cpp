#include "interfaz.h"
#include "hardware.h"
#include "comunicacion.h"
#include "config.h"
#include "eeprom.h"
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <WiFi.h>

// Esto se Agrego entrar a la red 
const char* ssidAP = "InterfazIoT";
const char* passwordAP = "12345678";
IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer server(80);

const char* htmlPath = "/interfaz.html.gz";

// --- Declaraciones para gestión de redes WiFi ---
#define MAX_NETWORKS 3 // GUarda la cantidad de redes 
#define SSID_MAXLEN 32 // Longitud maxima paranombre de las redes 
#define PASS_MAXLEN 64 //Logitud maxima para la   contraseña de las redes

struct SavedNetwork {
char ssid[SSID_MAXLEN];
char password[PASS_MAXLEN];
};

struct WifiConfig {
SavedNetwork networks[MAX_NETWORKS];
uint8_t count;
int8_t preferred;
};

WifiConfig wifiConfig;


// --- Función para registrar los endpoints de la API de intercambio de información y WiFi ---
void IntercambioInfoAPI(const String& ip) {
// Endpoint para mostrar info de red
server.on("/info", HTTP_GET, [ip](AsyncWebServerRequest *request) {
String jsonResponse = "{\"ssid\":\"" + String(ssidAP) + "\",\"ip\":\"" + ip + "\",\"password\":\"" + String(passwordAP) + "\"}";
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

// Conectar a una red (POST)
server.on("/conectarRed", HTTP_POST, [](AsyncWebServerRequest *request) {
if (!request->hasParam("ssid", true)) {
  request->send(400, "application/json", "{\"error\":\"Falta el SSID\"}");
  return;
}
String ssid = request->getParam("ssid", true)->value();
String password = request->hasParam("password", true) ? request->getParam("password", true)->value() : "";
// Aquí podrías intentar conectar realmente, pero solo respondemos éxito
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
}


// Función para cargar la interfaz  en modo programación
void CargarInterfaz() {
if (!SPIFFS.begin(true)) {
Serial.println("Error al montar SPIFFS");
return;
}

WiFi.mode(WIFI_AP);
WiFi.softAPConfig(local_IP, gateway, subnet);
WiFi.softAP(ssidAP, passwordAP);

String ip = WiFi.softAPIP().toString();

Serial.println("Punto de acceso iniciado:");
Serial.print("SSID: ");
Serial.println(ssidAP);
Serial.print("Contraseña: ");
Serial.println(passwordAP);
Serial.print("IP: ");
Serial.println(ip);

// Endpoint para servir el HTML comprimido
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
if (!SPIFFS.exists(htmlPath)) {
request->send(404, "text/plain", "Archivo interfaz no encontrado");
return;
}
AsyncWebServerResponse *response = request->beginResponse(SPIFFS, htmlPath, "text/html");
response->addHeader("Content-Encoding", "gzip");
request->send(response);
});

// Endpoint para mostrar info de red
server.on("/info", HTTP_GET, [ip](AsyncWebServerRequest *request) {
String jsonResponse = "{\"ssid\":\"" + String(ssidAP) + "\",\"ip\":\"" + ip + "\",\"password\":\"" + String(passwordAP) + "\"}";
request->send(200, "application/json", jsonResponse);
});

server.onNotFound([](AsyncWebServerRequest *request) {
request->send(404, "text/plain", "Página no encontrada");
});

server.begin();
Serial.println("Servidor web iniciado en modo programación");
}



void tareaDetectarModoProgramacion(void *pvParameters) {
    pinMode(PROG, INPUT_PULLUP);
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, LOW);

    while (true) {
        // Espera a que el botón sea presionado (LOW)
        if (digitalRead(PROG) == LOW) {
            unsigned long tiempoInicio = millis();
            // Espera mientras el botón siga presionado
            while (digitalRead(PROG) == LOW) {
                // Si pasan 3 segundos (3000 ms)
                if (millis() - tiempoInicio >= 3000) {
                    // Entra en modo programación solo si no está ya activo
                    if (!modoProgramacion) {
                        modoProgramacion = true;
                        digitalWrite(LED_STATUS, HIGH);
                        Serial.println("Entrando en modo programación por botón físico...");
                        CargarInterfaz();
                        Serial.print("SSID: ");
                        Serial.println("DispositivoIoT");
                        Serial.print("Contraseña: ");
                        Serial.println("iotpassword123");
                        Serial.print("IP: ");
                        Serial.println(WiFi.softAPIP());
                        // El LED queda encendido mientras esté en modo programación
                    }
                    // Una vez en modo programación, queda en bucle infinito hasta reinicio
                    while (true) {
                        digitalWrite(LED_STATUS, HIGH); // Mantiene el LED encendido
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                    }
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}



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
