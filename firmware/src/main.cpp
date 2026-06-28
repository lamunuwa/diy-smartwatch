/*
 * SmartWatch
 * Version: v0.3.0
 *
 * Descripción:
 * Se migra el reloj simulado a un RTC por hardware real (DS3231) montado en el PCB.
 * Se implementa la máquina de estados para la navegación cíclica de menús:
 * Hora, Alarmas y Oxímetro usando los botones físicos.
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
#define SCREEN_HEIGHT  64
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

// RTC Real por Hardware (DS3231 del PCB)
RTC_DS3231 rtc;
uint8_t ultimoSegundo = 255; // Evita redibujar cada ciclo si no cambia el tiempo

// Máquina de estados para los Menús
enum EstadosPantalla { MENU_HORA, MENU_ALARMAS, MENU_OXIMETRO, TOTAL_MENUS };
EstadosPantalla menuActual = MENU_HORA;

// Variables de control de botones (Debounce básico)
unsigned long ultimoTiempoBoton = 0;
const unsigned long tiempoDebounce = 250; 

// Renderizado OLED según el menú actual
void actualizarPantalla(DateTime ahora)
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Encabezado del sistema
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("INNOVATEK 2026");

    // Lógica de renderizado por menú
    switch (menuActual) 
    {
        case MENU_HORA: 
        {
            display.setTextSize(2);
            display.setCursor(16, 24);

            if(ahora.hour() < 10) display.print('0');
            display.print(ahora.hour());
            display.print(':');

            if(ahora.minute() < 10) display.print('0');
            display.print(ahora.minute());
            display.print(':');

            if(ahora.second() < 10) display.print('0');
            display.print(ahora.second());

            display.setTextSize(1);
            display.setCursor(32, 52);

            if(ahora.day() < 10) display.print('0');
            display.print(ahora.day());
            display.print('/');

            if(ahora.month() < 10) display.print('0');
            display.print(ahora.month());
            display.print('/');
            display.print(ahora.year());
            break;
        }

        case MENU_ALARMAS: 
        {
            display.setTextSize(1);             
            display.setCursor(0, 20); 
            display.print("SELECCIONAR TIEMPO:");
            
            display.setTextSize(2);
            display.setCursor(10, 34);
            display.print("> 30 Seg"); // Placeholder inicial de alarma
            
            display.setTextSize(1);
            display.setCursor(0, 56);
            display.print("Bloquea para configurar");
            break;
        }

        case MENU_OXIMETRO: 
        {
            display.setTextSize(1);             
            display.setCursor(0, 16); 
            display.print("PULSO Y OXIGENO:");
            
            display.setCursor(24, 34);
            display.print("Coloque dedo...");
            
            display.setCursor(24, 48);
            display.print("Leyendo...");
            break;
        }
    }

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

    // Inicialización de la pantalla OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("ERROR: OLED no encontrada");
        while(true)
        {
            digitalWrite(LED_DEBUG, !digitalRead(LED_DEBUG));
            delay(200);
        }
    }

    // Inicialización del RTC físico
    if (!rtc.begin()) 
    {
        Serial.println("ERROR: No se encuentra módulo RTC");
        while(true);
    }

    // Si el RTC perdió la energía, se fuerza la hora de compilación original
    if (rtc.lostPower()) 
    {
        Serial.println("RTC perdió energía, configurando hora...");
        rtc.adjust(DateTime(2026, 6, 17, 10, 23, 0)); 
    }

    actualizarPantalla(rtc.now());
    Serial.println("RTC de Hardware Inicializado");
}

// Bucle principal del sistema
void loop()
{
    unsigned long tiempoActualMillis = millis();
    DateTime ahora = rtc.now();

    // --- NAVEGACIÓN DE MENÚS (Lectura de Botones Físicos con Debounce) ---
    if ((tiempoActualMillis - ultimoTiempoBoton) > tiempoDebounce) 
    {
        // Botón Adelante avanza en los menús de manera cíclica
        if (digitalRead(BTN_ADELANTE) == LOW) 
        {
            menuActual = static_cast<EstadosPantalla>((menuActual + 1) % TOTAL_MENUS);
            ultimoTiempoBoton = tiempoActualMillis;
            actualizarPantalla(ahora);
            Serial.print("Menú Cambiado a: "); Serial.println(menuActual);
        }
        // Botón Atrás retrocede en los menús de manera cíclica
        else if (digitalRead(BTN_ATRAS) == LOW) 
        {
            menuActual = static_cast<EstadosPantalla>((menuActual - 1 + TOTAL_MENUS) % TOTAL_MENUS);
            ultimoTiempoBoton = tiempoActualMillis;
            actualizarPantalla(ahora);
            Serial.print("Menú Cambiado a: "); Serial.println(menuActual);
        }
    }

    // --- ACTUALIZACIÓN POR SEGUNDO (Menú Hora) ---
    if (menuActual == MENU_HORA) 
    {
        if (ahora.second() != ultimoSegundo) 
        {
            ultimoSegundo = ahora.second();
            actualizarPantalla(ahora);

            Serial.print("Hora RTC: ");
            if(ahora.hour() < 10) Serial.print('0');
            Serial.print(ahora.hour());
            Serial.print(':');
            if(ahora.minute() < 10) Serial.print('0');
            Serial.print(ahora.minute());
            Serial.print(':');
            if(ahora.second() < 10) Serial.print('0');
            Serial.println(ahora.second());
        }
    }
    
    delay(10);
}