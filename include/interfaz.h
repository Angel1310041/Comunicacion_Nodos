/*
    Método Definitivo para cargar interfaz:
    1. Minimizar el HTML y CSS con: https://codebeautify.org/minify-html
    2. Minimizar JS con: https://obfuscator.io/#code
    3. Codificar JS con: https://minify-js.com/
    4. Juntar todo en un solo archivo .html
    5. Comprimir el archivo .html con "comprimirGZ.py" de la carpeta "tools"
    6. Subir el archivo .gz a la carpeta "data" del proyecto con el nombre "interfaz.html.gz"

    Comprimir imagenes en formato 64:
    1. Tener la imagen en el formato deseado (png, jpg, etc.)
    2. Redimensionar la imagen en el tamaño deseado (ejemplo: 200x200 px): https://www.iloveimg.com/es/redimensionar-imagen#resize-options,pixels
    3. Convertir la imagen a formato base64: https://www.base64-image.de/
    4. Copiar el resultado en la variable "imagenes" de la interfaz

    Para usar la esptool se tiene que colocar así:
    python -m esptool --chip esp32 --port COM3 "accion"
    
    Acciones posibles: 
    erase_flash - Borra la memoria flash del ESP32
    write_flash - Graba el firmware en la memoria flash del ESP32
    read_flash - Lee el firmware de la memoria flash del ESP32
    --baud 115200 - Velocidad de transferencia de datos (115200 es la velocidad por defecto)
    --flash_size=detect - Detecta el tamaño de la memoria flash del ESP32
    --before default_reset - Resetea el ESP32 antes de grabar el firmware
    --after hard_reset - Resetea el ESP32 después de grabar el firmware
    --compress - Comprime el firmware para que ocupe menos espacio en la memoria flash del ESP32
    --verify - Verifica que el firmware se haya grabado correctamente
    --flash_mode dio - Modo de grabación del firmware (dio es el modo por defecto)
    --flash_freq 40m - Frecuencia de grabación del firmware (40m es la frecuencia por defecto)
    --flash_size 4MB - Tamaño de la memoria flash del ESP32 (4MB es el tamaño por defecto)
    --flash_offset 0x1000 - Offset de la memoria flash del ESP32 (0x1000 es el offset por defecto)

    Si se carga una programación en OTA la partición que utiliza es app0 y la partición app0 es para cargar a través de USB.
    pio run --target uploadfs    comando que debe ser abierto en una terminal de pio para poder cargar la app
    */

#ifndef INTERFAZ_H
    #define INTERFAZ_H
    #include "main.h"

    extern AsyncWebServer server;
    void entrarmodoprog();
    void endpointsMProg(void *pvParameters);
    void enviarPorLora(String mensaje);
    void procesarEnvioLora();
    void mostrarPantallaPorNumero(int numero);
    void cargarOrdenDesdeEEPROM(bool imprimirDatos = false);
    void iniciarModoProgramacion();
#endif 
