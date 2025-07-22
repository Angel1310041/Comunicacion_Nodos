#include "pantalla.h"
#include "OLEDDisplayFonts.h"
#include "config.h"
#include "eeprom.h"



bool displayActivo = true; // Initialize display as active by default

void inicializarPantalla() {
  Heltec.display->init();
  Heltec.display->setFont(ArialMT_Plain_10); 
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->clear();
  Heltec.display->drawString(0, 0, "Iniciando LoRa...");
  Heltec.display->display();
}

void limpiarPantalla() {
  if (displayActivo) { // Only clear if active
    Heltec.display->clear();
    Heltec.display->display();
  }
}

void mostrarMensaje(const String& titulo, const String& mensaje, int delayMs) {
  if (displayActivo) { // Only update if active
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, titulo);
    Heltec.display->drawString(0, 20, mensaje);
    Heltec.display->display();
    if (delayMs > 0) {
      delay(delayMs);
    }
  }
}

void mostrarEstadoLoRa(const String& idNodo, const String& canal, const String& version) {
  if (displayActivo) { // Only update if active
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "ID: " + idNodo);
    Heltec.display->drawString(0, 15, "Canal: " + canal);
    Heltec.display->drawString(0, 30, "Ver: " + version);
    Heltec.display->drawString(0, 45, "Esperando mensajes...");
    Heltec.display->display();
  }
}

void mostrarMensajeRecibido(const String& origen, const String& mensaje) {
  if (displayActivo) { // Only update if active
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "MSG Recibido!");
    Heltec.display->drawString(0, 15, "De: " + origen);
    String msgDisplay = mensaje;
    if (msgDisplay.length() > 30) {
      msgDisplay = msgDisplay.substring(0, 27) + "...";
    }
    Heltec.display->drawString(0, 30, "Msg: " + msgDisplay);
    Heltec.display->display();
    // También imprimir por Serial
    Serial.println("[DISPLAY] MSG Recibido!");
    Serial.println("[DISPLAY] De: " + origen);
    Serial.println("[DISPLAY] Msg: " + mensaje);
    delay(3000);
  }
}

void mostrarMensajeEnviado(const String& destino, const String& mensaje) {
  if (displayActivo) { // Only update if active
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "MSG Enviado!");
    Heltec.display->drawString(0, 15, "A: " + destino);
    String msgDisplay = mensaje;
    if (msgDisplay.length() > 30) {
      msgDisplay = msgDisplay.substring(0, 27) + "...";
    }
    Heltec.display->drawString(0, 30, "Msg: " + msgDisplay);
    Heltec.display->display();
    // También imprimir por Serial
    Serial.println("[DISPLAY] MSG Enviado!");
    Serial.println("[DISPLAY] A: " + destino);
    Serial.println("[DISPLAY] Msg: " + mensaje);
    delay(2000);
  }
}

void mostrarError(const String& mensajeError) {
  if (displayActivo) { // Only update if active
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "ERROR!");
    Heltec.display->drawString(0, 20, mensajeError);
    Heltec.display->display();
    delay(3000);
  }
}

void mostrarInfo(const String& mensajeInfo) {
  if (displayActivo) { // Only update if active
    Heltec.display->clear();
    Heltec.display->drawString(0, 0, "INFO:");
    Heltec.display->drawString(0, 20, mensajeInfo);
    Heltec.display->display();
    delay(2000);
  }
}

void configurarDisplay(bool habilitar) {
  displayActivo = habilitar;
  configLora.displayOn = habilitar; // Update the config
  if (habilitar) {
    Heltec.display->displayOn(); // Turn on the display
    mostrarInfo("Display HABILITADO.");
  } else {
    Heltec.display->displayOff(); // Turn off the display
    Serial.println("Display DESHABILITADO.");
  }
}