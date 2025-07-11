#ifndef EEPROM_H
    #define EEPROM_H
    #include "config.h"

    struct LoRaMem {
        uint32_t magic;
        char IDLora[3]; // 0 - 9 + A - Z
        int Canal; // 9
        bool Pantalla;
        bool UART;
        bool I2C;
        char PinesGPIO[6];
    };

    extern LoRaMem tarjeta;

    /*
    tarjeta.PinesGPIO -> 0 = Inhabilitado, 1 = Entrada, 2 = Salida
    */

    class ManejoEEPROM {
        public:
            // Lectura de la EEPROM
            static void leerTarjetaEEPROM();

            // Escritura de la EEPROM
            static void guardarTarjetaConfigEEPROM();
    };

#endif