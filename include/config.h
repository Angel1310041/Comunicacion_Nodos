#ifndef CONFIG_H
    #define CONFIG_H
    
    #include <Arduino.h>
    #include <SPI.h>
    #include <RadioLib.h>
    #include <heltec.h>
    #include <Preferences.h>

    // Pines LoRa
    #define LORA_MOSI 10
    #define LORA_MISO 11
    #define LORA_SCK 9
    #define LORA_CS   8
    #define LORA_RST  12
    #define LORA_DIO1 14
    #define LORA_BUSY 13

    #define TEST_IN 1
    #define PIN_IO1 2
    #define PIN_IO2 3
    #define PIN_IO3 4
    #define PIN_IO4 5
    #define PIN_IO5 6
    #define PIN_IO6 7
    #define LED_STATUS 35

    #define UART_RX 46
    #define UART_TX 45

    #define I2C_SDA 19
    #define I2C_SCL 20

    extern const int EEPROM_SIZE;
    extern String Version;
    const int pinNumbers[6];
    const char* pinNames[6];

    void imprimirSerial(String mensaje, char color = 'w');

#endif