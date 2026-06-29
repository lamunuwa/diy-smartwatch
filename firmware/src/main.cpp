/*
 * SmartWatch
 * Version: v0.4.0
 *
 * Descripción:
 * Se agregaron recursos gráficos (iconos de 8x8) almacenados en PROGMEM.
 * Se implementa el sistema de Bloqueo/Desbloqueo de interfaz mediante la
 * pulsación simultánea de ambos botones físicos, protegiendo los menús.
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

// Iconos
const unsigned char PROGMEM lock_icon[] = {
    0x3c, 0x42, 0x42, 0xff, 0xff, 0xff, 0xff, 0xff
};

const unsigned char PROGMEM unlock_icon[] = {
    0x3c, 0x02, 0x02, 0xff, 0xff, 0xff, 0xff, 0xff
};

const unsigned char PROGMEM heart_icon[] = {
    0x66, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x18, 0x00
};

// Configuración de la pantalla OLED
Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

// RTC Real por Hardware
RTC_DS3231 rtc;
uint8_t ultimoSegundo = 255; 

// Máquina de estados para los Menús
enum EstadosPantalla { MENU_HORA, MENU_ALARMAS, MENU_OXIMETRO, TOTAL_MENUS };
EstadosPantalla menuActual = MENU_HORA;

// Control de Seguridad de Interfaz
bool pantallaBloqueada = true; 

// Control de Tiempos de Botones (Debounce)
unsigned long ultimoTiempoBoton = 0;
const unsigned long tiempoDebounce = 250; 

// Renderizado dinámico de la pantalla OLED
void actualizarPantalla(DateTime ahora)
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Encabezado del sistema
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("INNOVATEK 2026");

    // Dibujar icono de bloqueo según el estado actual en la esquina superior derecha (120, 0)
    if (pantallaBloqueada) {
        display.drawBitmap(120, 0, lock_icon, 8, 8, SSD1306_WHITE); 
    } else {
        display.drawBitmap(120, 0, unlock_icon, 8, 8, SSD1306_WHITE); 
    }

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
            display.print("> 30 Seg"); 
            
            display.setTextSize(1);
            display.setCursor(0, 56);
            if (pantallaBloqueada) {
                display.print("Manten ATRAS para activar");
            } else {
                display.print("Bloquea para configurar");
            }
            break;
        }

        case MENU_OXIMETRO: 
        {
            display.setTextSize(1);             
            display.setCursor(0, 16); 
            display.print("PULSO Y OXIGENO:");
            
            // Renderizar icono de corazón al lado de las lecturas
            display.drawBitmap(10, 34, heart_icon, 8, 8, SSD1306_WHITE);
            
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

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("ERROR: OLED no encontrada");
        while(true)
        {
            digitalWrite(LED_DEBUG, !digitalRead(LED_DEBUG));
            delay(200);
        }
    }

    if (!rtc.begin()) 
    {
        Serial.println("ERROR: No se encuentra módulo RTC");
        while(true);
    }

    if (rtc.lostPower()) 
    {
        rtc.adjust(DateTime(2026, 6, 17, 10, 23, 0)); 
    }

    actualizarPantalla(rtc.now());
    Serial.println("v0.4.0 Recursos Gráficos y Candado OK");
}

// Bucle principal del sistema
void loop()
{
    unsigned long tiempoActualMillis = millis();
    DateTime ahora = rtc.now();

    // Control de botones con debounce y bloqueo de interfaz
    if ((tiempoActualMillis - ultimoTiempoBoton) > tiempoDebounce) 
    {
        bool leerBtnAdelante = (digitalRead(BTN_ADELANTE) == LOW);
        bool leerBtnAtras = (digitalRead(BTN_ATRAS) == LOW);

        // COMBINACIÓN: Si ambos botones se presionan al mismo tiempo
        if (leerBtnAdelante && leerBtnAtras) 
        {
            pantallaBloqueada = !pantallaBloqueada; // Invierte el estado de bloqueo
            ultimoTiempoBoton = tiempoActualMillis;
            actualizarPantalla(ahora);
            Serial.print("[INTERFAZ] Estado de bloqueo cambiado a: "); Serial.println(pantallaBloqueada);
            delay(250); // Delay de estabilidad mecánica provisional
        }
        // Acción de Botón Adelante (Solo si está desbloqueado)
        else if (leerBtnAdelante && !pantallaBloqueada) 
        {
            menuActual = static_cast<EstadosPantalla>((menuActual + 1) % TOTAL_MENUS);
            ultimoTiempoBoton = tiempoActualMillis;
            actualizarPantalla(ahora);
            Serial.print("Menú -> "); Serial.println(menuActual);
        }
        // Acción de Botón Atrás (Solo si está desbloqueado)
        else if (leerBtnAtras && !pantallaBloqueada) 
        {
            menuActual = static_cast<EstadosPantalla>((menuActual - 1 + TOTAL_MENUS) % TOTAL_MENUS);
            ultimoTiempoBoton = tiempoActualMillis;
            actualizarPantalla(ahora);
            Serial.print("Menú <- "); Serial.println(menuActual);
        }
    }

    // Para evitar el parpadeo de la pantalla, solo actualizamos la pantalla si ha cambiado el segundo actual y no estamos en el menú de oxímetro
    if (menuActual != MENU_OXIMETRO) 
    {
        if (ahora.second() != ultimoSegundo) 
        {
            ultimoSegundo = ahora.second();
            actualizarPantalla(ahora);
        }
    }
    
    delay(1);
}