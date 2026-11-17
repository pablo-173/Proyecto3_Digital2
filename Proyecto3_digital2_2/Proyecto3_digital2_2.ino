
#define CUSTOM_SETTINGS
#define INCLUDE_TERMINAL_MODULE

#include <Arduino.h>
#include <DabbleESP32.h>
#include "esp_system.h"

// UART1 para comunicación con Nucleo 1
#define RXD1 16
#define TXD1 17

// Procesar comandos recibidos por Bluetooth o USB
void enviarComando(char dato)
{
    if (dato >= '1' && dato <= '4')
    {
        // Enviar el número a Nucleo 1
        Serial1.write(dato);

        Serial.print("Comando enviado: ");
        Serial.println(dato);

        switch (dato)
        {
            case '1':
                Serial.println("Solicitando LEER");
                Terminal.println("Comando LEER enviado");
                break;

            case '2':
                Serial.println("Solicitando GUARDAR");
                Terminal.println("Comando GUARDAR enviado");
                break;

            case '3':
                Serial.println("Solicitando VER");
                Terminal.println("Comando VER enviado");
                break;

            case '4':
                Serial.println("Solicitando ESTADO");
                Terminal.println("Comando ESTADO enviado");
                break;
        }
    }
}

void setup()
{
    Serial.begin(115200);

    Serial.println("ESP32 INICIANDO...");
    Serial.print("Motivo de reinicio: ");
    Serial.println(esp_reset_reason());

    // Bluetooth Dabble
    Dabble.begin("perroThdpt");

    // UART hacia Nucleo 1
    Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);

    Serial.println("Bluetooth inicializado");
    Serial.println("ESP32 lista");

    Serial.println("1 = LEER");
    Serial.println("2 = GUARDAR");
    Serial.println("3 = VER");
    Serial.println("4 = ESTADO");
}

void loop()
{
    Dabble.processInput();

    // Comandos desde el monitor serial de Arduino IDE
    while (Serial.available() > 0)
    {
        char dato = Serial.read();

        // Mostrar también en terminal Bluetooth
        Terminal.print(dato);

        enviarComando(dato);
    }

    // Comandos desde Dabble Bluetooth
    while (Terminal.available() > 0)
    {
        char dato = Terminal.read();

        Serial.write(dato);

        enviarComando(dato);
    }

    // Recibir respuestas desde Nucleo 1
    while (Serial1.available() > 0)
    {
        char respuesta = Serial1.read();

        // Mostrar respuesta en Arduino IDE
        Serial.write(respuesta);

        // Mostrar respuesta en Dabble
        Terminal.print(respuesta);
    }

    // Confirmar que ESP32 sigue funcionando
    static unsigned long tiempo = 0;

    if (millis() - tiempo >= 2000)
    {
        tiempo = millis();
        Serial.println("ESP32 funcionando...");
    }
}
