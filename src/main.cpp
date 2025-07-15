    #include <Arduino.h>
    #include <SPI.h>
    #include <RadioLib.h>
    #include "heltec.h"
    #include "pantalla.h"
    #include "interfaz.h"

    // Pines LoRa
    #define LORA_MOSI 10
    #define LORA_MISO 11
    #define LORA_SCK 9
    #define LORA_CS   8
    #define LORA_RST  12
    #define LORA_DIO1 14
    #define LORA_BUSY 13

    #define PIN_IO1 2
    #define PIN_IO2 3
    #define PIN_IO3 4
    #define PIN_IO4 5
    #define PIN_IO5 6
    #define PIN_IO6 7
    #define LED_STATUS 35
    #define ENTRADA_FIJA  1
    #define BOTON_MODO_PROG 0  // Pin del botón para modo programación

    #define UART_RX 46
    #define UART_TX 45

    #define I2C_SDA 19
    #define I2C_SCL 20

    SX1262 lora = new Module(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY);
    extern const int nodeID; // Definido en pantalla.cpp

    bool modoProgramacion = false;
    unsigned long tiempoPresionado = 0;
    bool botonPresionado = false;
    unsigned long lastUpdate = 0;
    unsigned long lastActivity = 0;

    void setup() {
    Heltec.begin(true /*DisplayEnable*/, false /*LoRaEnable*/, true /*SerialEnable*/);
    Serial.begin(115200);
    delay(1000);

    pinMode(BOTON_MODO_PROG, INPUT_PULLUP);
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, LOW);

    pantallaControl("on", "Iniciando...", "", true);

    if (lora.begin() != RADIOLIB_ERR_NONE) {
    Serial.println("LoRa: FALLÓ inicialización");
    pantallaControl("", "Error LoRa", "Reiniciar");
    while (true);
    }

    Serial.println("\nSistema listo");
    Serial.println("ID Nodo: " + String(nodeID));
    Serial.println("Comandos disponibles:");
    Serial.println("  pantalla [on|off|toggle|show msg]");
    Serial.println("  lora [on|off]");
    Serial.println("  Enviar mensaje: ID_DESTINO|mensaje");
    Serial.println("  exitprog - Salir modo programación");

    pantallaControl("", "Nodo " + String(nodeID), "Esperando datos...");
    }

    void entrarModoProgramacion() {
    modoProgramacion = true;
    digitalWrite(LED_STATUS, HIGH);
    lastActivity = millis();
    Serial.println("Modo programación activado");
    iniciarModoProgramacion();
    }

    void salirModoProgramacion() {
    modoProgramacion = false;
    digitalWrite(LED_STATUS, LOW);
    WiFi.softAPdisconnect(true);
    pantallaControl("", "Nodo " + String(nodeID), "Operación normal");
    Serial.println("Modo programación desactivado");
    }

    void controlLoRa(String accion) {
    if(accion == "on") {
    if(lora.begin() == RADIOLIB_ERR_NONE) {
    Serial.println("LoRa: Activado");
    pantallaControl("", "LoRa Activado", "", true);
    } else {
    Serial.println("LoRa: Error al activar");
    pantallaControl("", "Error LoRa", "Reintentar");
    }
    } 
    else if(accion == "off") {
    lora.standby();
    Serial.println("LoRa: Desactivado");
    pantallaControl("", "LoRa Apagado", "", true);
    }
    }

    void loop() {
    // Control del botón para modo programación
    if (digitalRead(BOTON_MODO_PROG) == LOW) {
    if (!botonPresionado) {
    botonPresionado = true;
    tiempoPresionado = millis();
    } else if (millis() - tiempoPresionado >= 3000 && !modoProgramacion) {
    entrarModoProgramacion();
    }
    } else {
    botonPresionado = false;
    }

    // Comandos por serial
    if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    lastActivity = millis();
    Serial.println("Comando: " + input);

    if (input == "exitprog" && modoProgramacion) {
    salirModoProgramacion();
    return;
    }

    if (input.startsWith("pantalla ")) {
    pantallaControl(input.substring(9));
    return;
    }
    else if (input.startsWith("lora ")) {
    controlLoRa(input.substring(5));
    return;
    }

    int sep = input.indexOf('|');
    if (sep > 0) {
    int destino = input.substring(0, sep).toInt();
    String mensaje = input.substring(sep + 1);

    if (destino != nodeID && mensaje.length() > 0) {
    int siguienteHop = (destino > nodeID) ? nodeID + 1 : nodeID - 1;

    String paquete = "ORIG:" + String(nodeID) +
    "|DEST:" + String(destino) +
    "|MSG:" + mensaje +
    "|HOP:" + String(siguienteHop);

    pantallaControl("", "Enviando a", "HOP: " + String(siguienteHop));

    int resultado = lora.transmit(paquete);
    if (resultado == RADIOLIB_ERR_NONE) {
    pantallaControl("", "Mensaje enviado", "OK");
    } else {
    pantallaControl("", "Error envio", "Cod: " + String(resultado));
    }
    }
    }
    lastUpdate = millis();
    }

    if (!modoProgramacion) {
    // Operación normal
    if (millis() - lastUpdate > 5000) {  
    pantallaControl("", "Nodo " + String(nodeID), "Esperando...");
    lastUpdate = millis();
    }

    // Recepción LoRa
    String msg;
    int state = lora.receive(msg);
    if (state == RADIOLIB_ERR_NONE) {
    int orig = msg.substring(msg.indexOf("ORIG:") + 5, msg.indexOf("|DEST")).toInt();
    int dest = msg.substring(msg.indexOf("DEST:") + 5, msg.indexOf("|MSG")).toInt();
    String cuerpo = msg.substring(msg.indexOf("MSG:") + 4, msg.indexOf("|HOP"));

    if (nodeID == dest) {
    pantallaControl("", "Mensaje recibido", cuerpo);
    } 
    else if (nodeID == msg.substring(msg.indexOf("HOP:") + 4).toInt()) {
    int siguienteHop = (dest > nodeID) ? nodeID + 1 : nodeID - 1;
    String nuevoMsg = msg.substring(0, msg.lastIndexOf("|HOP:")) + String(siguienteHop);
    lora.transmit(nuevoMsg);
    pantallaControl("", "Reenviando a", "HOP: " + String(siguienteHop));
    }
    }
    } else {
    // Modo programación
    if (millis() - lastUpdate > 5000) {
    String ip = WiFi.softAPIP().toString();
    String mensaje = "SSID: " + String(ssidAP) + "\nPass: " + String(passwordAP) + "\nIP: " + ip;
    pantallaControl("", "MODO CONFIG", mensaje.c_str());
    lastUpdate = millis();
    }

    // Salir después de 10 minutos de inactividad (opcional)
    if (millis() - lastActivity > 600000) {
    salirModoProgramacion();
    }
    }

    delay(100);
    }