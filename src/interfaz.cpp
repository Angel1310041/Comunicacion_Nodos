  #include "interfaz.h"
  #include "hardware.h"
  #include "comunicacion.h"
  #include "config.h"
  #include "pantalla.h"
  #include "eeprom.h"
  #include <WiFi.h>
  #include <WebServer.h>
  #include <FS.h>
  #include <SPIFFS.h>
  #include <WiFiType.h>

  WebServer server(80);

  // Función para intercambio de datos API
  void apiIntercambiarDatos();

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
  servidorModoProgramacion();

  }

  void modoprogporbotonfisico() {
  static unsigned long tiempoInicio = 0;
  static bool botonAnterior = false;

  bool botonPresionado = digitalRead(BOTON_MODO_PROG) == LOW; // Botón activo en LOW

  if (!modoProgramacion) {
  if (botonPresionado) {
  if (!botonAnterior) {
  // Botón acaba de ser presionado
  tiempoInicio = millis();
  } else {
  // Botón sigue presionado
  if (millis() - tiempoInicio >= 3000) { // 3 segundos
  Interfaz::entrarModoProgramacion();
  }
  }
  } else if (botonAnterior) {
  // Botón fue soltado antes de los 3 segundos
  tiempoInicio = 0;
  }
  }
  botonAnterior = botonPresionado;
  }

 void servidorModoProgramacion() {
  if (!SPIFFS.begin(true)) {
  imprimirSerial("Error al montar SPIFFS", 'r');
  return;
  }

  WiFi.softAP("Interfazlor", "12345678");
  IPAddress myIP = WiFi.softAPIP();
  imprimirSerial("Servidor web iniciado en: http://" + myIP.toString(), 'y');

  // Configurar endpoints
  server.on("/", HTTP_GET, []() {
  if (SPIFFS.exists("/interfaz.html.gz")) {
  File file = SPIFFS.open("/interfaz.html.gz", FILE_READ);
  server.streamFile(file, "text/html");
  file.close();
  } else {
  server.send(404, "text/plain", "Archivo no encontrado");
  }
  });

  // Configuración GET
  server.on("/api/config", HTTP_GET, []() {
  DynamicJsonDocument doc(256);
  doc["id"] = configLora.IDLora;
  doc["channel"] = configLora.Canal;
  doc["displayOn"] = configLora.displayOn;
  doc["UART"] = configLora.UART;
  doc["I2C"] = configLora.I2C;
  doc["WiFi"] = configLora.WiFi;
  doc["DEBUG"] = configLora.DEBUG;
  String respuesta;
  serializeJson(doc, respuesta);
  server.send(200, "application/json", respuesta);
  });

  // Configuración POST
  server.on("/api/config", HTTP_POST, []() {
  if (!server.hasArg("plain")) {
  server.send(400, "text/plain", "Bad Request");
  return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(256);
  deserializeJson(doc, body);

  String newId = doc["id"].as<String>();
  int newChannel = doc["channel"];

  if (newId.length() > 0 && newId.length() <= 3 && newChannel >= 0 && newChannel <= 8) {
  strncpy(configLora.IDLora, newId.c_str(), sizeof(configLora.IDLora));
  configLora.Canal = newChannel;

  if (doc.containsKey("WiFi")) configLora.WiFi = doc["WiFi"];
  if (doc.containsKey("DEBUG")) configLora.DEBUG = doc["DEBUG"];

  ManejoEEPROM::guardarTarjetaConfigEEPROM();
  lora.begin(canales[configLora.Canal]);
  lora.startReceive();

  DynamicJsonDocument responseDoc(128);
  responseDoc["success"] = true;
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);

  mostrarEstadoLoRa(String(configLora.IDLora), String(configLora.Canal), "3.1.1.1");
  } else {
  server.send(400, "text/plain", "Parámetros inválidos");
  }
  });

  // Control WiFi
  server.on("/api/wifi", HTTP_POST, []() {
  if (!server.hasArg("plain")) {
  server.send(400, "text/plain", "Bad Request");
  return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(128);
  deserializeJson(doc, body);

  String state = doc["state"].as<String>();
  configLora.WiFi = (state == "1");

  ManejoEEPROM::guardarTarjetaConfigEEPROM();

  DynamicJsonDocument responseDoc(128);
  responseDoc["success"] = true;
  responseDoc["message"] = "WiFi " + String(configLora.WiFi ? "activado" : "desactivado");
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
  });

  // Escaneo WiFi
  server.on("/api/wifi/scan", HTTP_GET, []() {
  if (!configLora.WiFi) {
  server.send(200, "application/json", "{\"error\":\"WiFi desactivado\"}");
  return;
  }

  DynamicJsonDocument doc(1024);
  JsonArray redes = doc.createNestedArray("redes");

  int n = WiFi.scanNetworks(false, true);
  imprimirSerial("Escaneando redes WiFi... Encontradas: " + String(n), 'y');

  for (int i = 0; i < n; ++i) {
  JsonObject red = redes.createNestedObject();
  red["ssid"] = WiFi.SSID(i);
  red["rssi"] = WiFi.RSSI(i);
  red["channel"] = WiFi.channel(i);
  red["encryption"] = getEncryptionType(WiFi.encryptionType(i));
  red["encryptionType"] = (int)WiFi.encryptionType(i);
  }

  String respuesta;
  serializeJson(doc, respuesta);
  server.send(200, "application/json", respuesta);
  WiFi.scanDelete();
  });

  // Conectar a WiFi
  server.on("/api/wifi/connect", HTTP_POST, []() {
  if (!configLora.WiFi) {
  server.send(200, "application/json", "{\"error\":\"WiFi desactivado\"}");
  return;
  }

  if (!server.hasArg("plain")) {
  server.send(400, "text/plain", "Bad Request");
  return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(256);
  deserializeJson(doc, body);

  String ssid = doc["ssid"].as<String>();
  String password = doc["password"].as<String>();

  if (ssid.length() == 0) {
  server.send(400, "text/plain", "SSID no puede estar vacío");
  return;
  }

  if (ManejoEEPROM::guardarWiFiCredenciales(ssid.c_str(), password.c_str())) {
  DynamicJsonDocument responseDoc(128);
  responseDoc["success"] = true;
  responseDoc["message"] = "Credenciales guardadas. Reinicie para conectar.";
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
  } else {
  server.send(500, "text/plain", "Error al guardar credenciales");
  }
  });

  // Redes guardadas
  server.on("/api/wifi/saved", HTTP_GET, []() {
  DynamicJsonDocument doc(512);
  JsonArray redes = doc.createNestedArray("redes");

  ManejoEEPROM::obtenerRedesGuardadas(redes);

  if (redes.size() == 0) {
  doc["message"] = "No hay redes guardadas";
  }

  String respuesta;
  serializeJson(doc, respuesta);
  server.send(200, "application/json", respuesta);
  });

  // Eliminar red guardada
  server.on("/api/wifi/remove", HTTP_POST, []() {
  if (!server.hasArg("plain")) {
  server.send(400, "text/plain", "Bad Request");
  return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(256);
  deserializeJson(doc, body);

  String ssid = doc["ssid"].as<String>();

  if (ssid.length() == 0) {
  server.send(400, "text/plain", "SSID no puede estar vacío");
  return;
  }

  if (ManejoEEPROM::eliminarRedWiFi(ssid.c_str())) {
  DynamicJsonDocument responseDoc(128);
  responseDoc["success"] = true;
  responseDoc["message"] = "Red eliminada correctamente";
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
  } else {
  server.send(500, "text/plain", "Error al eliminar red");
  }
  });

  // Control de pantalla
  server.on("/api/display", HTTP_POST, []() {
  if (!server.hasArg("plain")) {
  server.send(400, "text/plain", "Bad Request");
  return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(128);
  deserializeJson(doc, body);

  String state = doc["state"].as<String>();
  configurarDisplay(state == "1");

  ManejoEEPROM::guardarTarjetaConfigEEPROM();
  server.send(200, "application/json", "{\"success\":true}");
  });

  // Prueba de pantalla
  server.on("/api/display/test", HTTP_POST, []() {
  String currentId = String(configLora.IDLora);
  mostrarMensaje("Prueba Pantalla", "ID: " + currentId, 3000);
  server.send(200, "application/json", "{\"success\":true}");
  });

  // Control UART
  server.on("/api/uart", HTTP_POST, []() {
  if (!server.hasArg("plain")) {
  server.send(400, "text/plain", "Bad Request");
  return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(128);
  deserializeJson(doc, body);

  String state = doc["state"].as<String>();
  configLora.UART = (state == "1");

  ManejoEEPROM::guardarTarjetaConfigEEPROM();
  ManejoComunicacion::initUART();

  DynamicJsonDocument responseDoc(128);
  responseDoc["success"] = true;
  responseDoc["message"] = "UART " + String(configLora.UART ? "activado" : "desactivado");
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
  });

  // Control I2C
  server.on("/api/i2c", HTTP_POST, []() {
  if (!server.hasArg("plain")) {
  server.send(400, "text/plain", "Bad Request");
  return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(128);
  deserializeJson(doc, body);

  String state = doc["state"].as<String>();
  configLora.I2C = (state == "1");

  ManejoEEPROM::guardarTarjetaConfigEEPROM();
  ManejoComunicacion::initI2C();

  DynamicJsonDocument responseDoc(128);
  responseDoc["success"] = true;
  responseDoc["message"] = "I2C " + String(configLora.I2C ? "activado" : "desactivado");
  String response;
  serializeJson(responseDoc, response);
  server.send(200, "application/json", response);
  });

  // Escaneo I2C
  server.on("/api/i2c/scan", HTTP_GET, []() {
  if (!configLora.I2C) {
  server.send(200, "application/json", "{\"error\":\"I2C desactivado\"}");
  return;
  }

  DynamicJsonDocument doc(512);
  JsonArray dispositivos = doc.createNestedArray("dispositivos");

  byte error, address;
  for (address = 1; address < 127; address++) {
  Wire.beginTransmission(address);
  error = Wire.endTransmission();

  if (error == 0) {
  JsonObject dispositivo = dispositivos.createNestedObject();
  dispositivo["direccion"] = "0x" + String(address, HEX);

  if (address == 0x3C) dispositivo["tipo"] = "Pantalla OLED";
  else if (address == 0x68) dispositivo["tipo"] = "Reloj RTC";
  else if (address == 0x76) dispositivo["tipo"] = "Sensor BME280";
  else dispositivo["tipo"] = "Desconocido";
  }
  delay(1);
  }

  String respuesta;
  serializeJson(doc, respuesta);
  server.send(200, "application/json", respuesta);
  });

  // Reinicio del dispositivo
  server.on("/api/reboot", HTTP_POST, []() {
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Reiniciando dispositivo...\"}");
  delay(500);
  ESP.restart();
  });

  // Manejador para rutas no encontradas
  server.onNotFound([]() {
  server.send(404, "text/plain", "Ruta no encontrada");
  });

  server.begin();

  while (modoProgramacion) {
  server.handleClient();
  delay(10);
  }
  }

String getEncryptionType(wifi_auth_mode_t encryptionType) {
switch (encryptionType) {
case WIFI_AUTH_OPEN: return "Abierta";
case WIFI_AUTH_WEP: return "WEP";
case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-PSK";
case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
default: return "Desconocido";
}
}


