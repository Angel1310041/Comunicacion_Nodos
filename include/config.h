#ifndef CONFIG_H
#define CONFIG_H

#include <Preferences.h>

#define LORA_MOSI 10
#define LORA_MISO 11
#define LORA_SCK 9
#define LORA_CS   8
#define LORA_RST  12
#define LORA_DIO1 14
#define LORA_BUSY 13

#define PIN_IO0 1
#define PIN_IO1 2
#define PIN_IO2 3
#define PIN_IO3 4
#define PIN_IO4 5
#define PIN_IO5 6
#define PIN_IO6 7
#define LED_PIN 35

#define UART_RX 46
#define UART_TX 45

#define I2C_SDA 19
#define I2C_SCL 20

#define MSG_ID_BUFFER_SIZE 16

struct LoRaConfig {
  char IDLora[4];
  int Canal;
  bool Pantalla;
  bool UART;
  bool I2C;
  char Pines[6];
};

extern LoRaConfig configLora;
extern Preferences preferences;
extern float canales[9];

void guardarConfig();
void cargarConfig();
void borrarConfig();
void pedirID();
void pedirCanal();

#endif
