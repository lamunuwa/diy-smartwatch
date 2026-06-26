/*
* ExpWatch 
* Version: v0.1.0
*
* Descripción:
* Se verifica el funcionamiento del ESP32, la comunicación I2C
* y la pantalla OLED SSD1306.
*/

// includes
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Configuración de hardware
#define LED_DEBUG      2
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define I2C_SDA        21
#define I2C_SCL        22
#define BTN_ADELANTE   26
#define BTN_ATRAS      27
#define MOTOR_PIN      14

// Configuración de la pantalla OLED
Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

void mostrarPantallaInicio()
{
    display.clearDisplay();

    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(10,12);
    display.println("SMART");

    display.setCursor(10,34);
    display.println("WATCH");

    display.setTextSize(1);
    display.setCursor(18,56);
    display.println("Hardware OK");

    display.display();
}

// Configuración inicial del sistema
void setup()
{
    pinMode(LED_DEBUG, OUTPUT);
    digitalWrite(LED_DEBUG, HIGH);

    pinMode(BTN_ADELANTE, INPUT_PULLUP);
    pinMode(BTN_ATRAS, INPUT_PULLUP);

    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);

    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println(" SmartWatch OK");

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("ERROR: OLED no encontrada");

        while (true)
        {
            digitalWrite(LED_DEBUG,!digitalRead(LED_DEBUG));
            delay(200);
        }
    }

    Serial.println("OLED inicializada correctamente");

    mostrarPantallaInicio();
}

// Bucle principal del sistema
void loop()
{
    static unsigned long tiempoAnterior = 0;
    static bool estadoLed = true;

    if (millis() - tiempoAnterior >= 500)
    {
        tiempoAnterior = millis();

        estadoLed = !estadoLed;

        digitalWrite(LED_DEBUG, estadoLed);

        Serial.println("Sistema OK");
    }
}