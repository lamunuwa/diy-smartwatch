/*
* SmartWatch
* Version: v0.2.0
*
* Descripción:
* Se incorpora un reloj por software utilizando RTC_Millis.
* La pantalla OLED ahora muestra fecha y hora en tiempo real.
*/

// includes
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

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

// RTC por software
RTC_Millis rtc;
uint8_t ultimoSegundo = 255; // Evita redibujar cada ciclo

// Imagen de la pantalla OLED
void actualizarPantalla(DateTime ahora)
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    //---------------------------------

    display.setTextSize(1);
    display.setCursor(18,0);
    display.println("SMARTWATCH");

    //---------------------------------

    display.setTextSize(2);
    display.setCursor(12,22);

    if(ahora.hour()<10) display.print('0');
    display.print(ahora.hour());
    display.print(':');

    if(ahora.minute()<10) display.print('0');
    display.print(ahora.minute());
    display.print(':');

    if(ahora.second()<10) display.print('0');
    display.print(ahora.second());

    //---------------------------------

    display.setTextSize(1);
    display.setCursor(22,52);

    if(ahora.day()<10) display.print('0');
    display.print(ahora.day());
    display.print('/');

    if(ahora.month()<10) display.print('0');
    display.print(ahora.month());
    display.print('/');
    display.print(ahora.year());

    //---------------------------------

    display.display();
}

// Configuración inicial del sistema
void setup()
{
    pinMode(LED_DEBUG,OUTPUT);
    digitalWrite(LED_DEBUG,HIGH);

    pinMode(BTN_ADELANTE,INPUT_PULLUP);
    pinMode(BTN_ATRAS,INPUT_PULLUP);

    pinMode(MOTOR_PIN,OUTPUT);
    digitalWrite(MOTOR_PIN,LOW);

    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println(" SmartWatch OK");

    Wire.begin(I2C_SDA,I2C_SCL);
    Wire.setClock(100000);

    if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C))
    {
        Serial.println("ERROR: OLED no encontrada");

        while(true)
        {
            digitalWrite(LED_DEBUG,!digitalRead(LED_DEBUG));
            delay(200);
        }
    }

    rtc.begin(DateTime(2025,1,1,12,0,0));

    actualizarPantalla(rtc.now());

    Serial.println("RTC iniciado");
}

// Bucle principal del sistema
void loop()
{
    DateTime ahora = rtc.now();
    if(ahora.second()!=ultimoSegundo) // Actualiza únicamente cuando cambia el segundo
    {
        ultimoSegundo=ahora.second();
        actualizarPantalla(ahora);

        Serial.print("Hora: ");

        if(ahora.hour()<10) Serial.print('0');
        Serial.print(ahora.hour());

        Serial.print(':');

        if(ahora.minute()<10) Serial.print('0');
        Serial.print(ahora.minute());

        Serial.print(':');

        if(ahora.second()<10) Serial.print('0');
        Serial.println(ahora.second());
    }

    delay(10);
}