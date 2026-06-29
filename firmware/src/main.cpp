/*
 * SmartWatch
 * Version: v0.5.0
 *
 * Descripción:
 * Se elimina el delay mecánico y se implementa la ventana de tolerancia (80ms)
 * para detectar la combinación de doble botón de forma asíncrona.
 * Se integra el arreglo de tiempos de alarma y la navegación entre opciones.
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
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RTC Real por Hardware
RTC_DS3231 rtc;
uint8_t ultimoSegundoRegistrado = 99; 

// Máquina de estados para los Menús
enum EstadosPantalla { MENU_HORA, MENU_ALARMAS, MENU_OXIMETRO, TOTAL_MENUS };
EstadosPantalla menuActual = MENU_HORA;

bool pantallaBloqueada = true; 

// Historial Mecánico
unsigned long ultimoTiempoBoton = 0;
const unsigned long tiempoDebounce = 200; 

unsigned long tiempoPrimerBoton = 0;
bool esperandoCombinacion = false;
bool adelantePresionadoPrimero = false;
bool atrasPresionadoPrimero = false;
const unsigned long ventanaTolerancia = 80; 

// Lista de Alarmas
const int TOTAL_OPCIONES = 5;
String listaAlarmas[TOTAL_OPCIONES] = {"30 Seg", "1 Min", "10 Min", "30 Min", "1 Hora"};
int segundosAlarma[TOTAL_OPCIONES] = {30, 60, 600, 1800, 3600};
int alarmaSeleccionada = 0; 

bool alarmaActivaYCorriendo = false;

// Renderizado dinámico de la pantalla OLED
void actualizarPantalla(DateTime tiempoAhoraRTC)
{
    display.clearDisplay();
    display.setTextSize(1);             
    display.setTextColor(SSD1306_WHITE); 
    display.setCursor(0, 0); 
    display.print("INNOVATEK 2026");
    
    if (pantallaBloqueada) {
        display.drawBitmap(120, 0, lock_icon, 8, 8, SSD1306_WHITE); 
    } else {
        display.drawBitmap(120, 0, unlock_icon, 8, 8, SSD1306_WHITE); 
    }

    switch (menuActual) {
        case MENU_HORA: {
            display.setCursor(16, 24); 
            display.setTextSize(2);             
            if(tiempoAhoraRTC.hour() < 10) display.print('0');
            display.print(tiempoAhoraRTC.hour(), DEC);
            display.print(':');
            if(tiempoAhoraRTC.minute() < 10) display.print('0');
            display.print(tiempoAhoraRTC.minute(), DEC);
            display.print(':');
            if(tiempoAhoraRTC.second() < 10) display.print('0');
            display.print(tiempoAhoraRTC.second(), DEC);

            display.setTextSize(1);
            display.setCursor(32, 52);
            if(tiempoAhoraRTC.day() < 10) display.print('0');
            display.print(tiempoAhoraRTC.day(), DEC);
            display.print('/');
            if(tiempoAhoraRTC.month() < 10) display.print('0');
            display.print(tiempoAhoraRTC.month(), DEC);
            display.print('/');
            display.print(tiempoAhoraRTC.year(), DEC);
            break;
        }

        case MENU_ALARMAS: {
            if (alarmaActivaYCorriendo) {
                // Lógica de renderizado de cuenta regresiva
            } 
            else {
                display.setTextSize(1);             
                display.setCursor(0, 20); 
                display.print("SELECCIONAR TIEMPO:");
                
                display.setTextSize(2);
                display.setCursor(10, 34);
                display.print("> " + listaAlarmas[alarmaSeleccionada]);
                
                display.fillRect(0, 56, 128, 8, SSD1306_BLACK); 
                display.setTextSize(1);
                display.setCursor(0, 56);
                if (pantallaBloqueada) {
                    display.print("Manten ATRAS para activar");
                } else {
                    display.print("Bloquea para configurar");
                }
            }
            break;
        }

        case MENU_OXIMETRO: {
            display.setTextSize(1);             
            display.setCursor(0, 16); 
            display.print("PULSO Y OXIGENO:");
            
            display.drawBitmap(10, 34, heart_icon, 8, 8, SSD1306_WHITE);
            display.setTextSize(1);
            display.setCursor(24, 34);
            display.print("Coloque dedo...");
            display.setCursor(24, 48);
            display.print("Leyendo...");
            break;
        }
    }
    display.display(); 
}

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

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000); 

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
        while(true); 
    }
    
    if (!rtc.begin()) {
        while(true);
    }

    if (rtc.lostPower()) {
        rtc.adjust(DateTime(2026, 6, 17, 10, 23, 0)); 
    }

    actualizarPantalla(rtc.now());
}

void loop()
{
    unsigned long tiempoActualMillis = millis();
    DateTime tiempoAhoraRTC = rtc.now();

    bool leerBtnAdelante = (digitalRead(BTN_ADELANTE) == LOW);
    bool leerBtnAtras = (digitalRead(BTN_ATRAS) == LOW);
    
    // Control de botones con debounce y bloqueo de interfaz
    if ((tiempoActualMillis - ultimoTiempoBoton) > tiempoDebounce) {
        
        // Detección inmediata si ambos bajan en el mismo ciclo exacto
        if (leerBtnAdelante && leerBtnAtras) {
            pantallaBloqueada = !pantallaBloqueada;
            ultimoTiempoBoton = tiempoActualMillis;
            esperandoCombinacion = false;
            adelantePresionadoPrimero = false;
            atrasPresionadoPrimero = false;
            actualizarPantalla(tiempoAhoraRTC); 
            delay(250); 
        }
        // Llegó Adelante primero, abrimos ventana de espera sutil
        else if (leerBtnAdelante && !esperandoCombinacion) {
            tiempoPrimerBoton = tiempoActualMillis;
            esperandoCombinacion = true;
            adelantePresionadoPrimero = true;
            atrasPresionadoPrimero = false;
        }
        // Llegó Atrás primero, abrimos ventana de espera sutil
        else if (leerBtnAtras && !esperandoCombinacion) {
            tiempoPrimerBoton = tiempoActualMillis;
            esperandoCombinacion = true;
            atrasPresionadoPrimero = true;
            adelantePresionadoPrimero = false;
        }

        // Procesamiento de la ventana de tolerancia (80 ms)
        if (esperandoCombinacion) {
            if (adelantePresionadoPrimero && leerBtnAtras) {
                pantallaBloqueada = !pantallaBloqueada;
                ultimoTiempoBoton = tiempoActualMillis;
                esperandoCombinacion = false;
                adelantePresionadoPrimero = false;
                actualizarPantalla(tiempoAhoraRTC);
                delay(250);
            }
            else if (atrasPresionadoPrimero && leerBtnAdelante) {
                pantallaBloqueada = !pantallaBloqueada;
                ultimoTiempoBoton = tiempoActualMillis;
                esperandoCombinacion = false;
                atrasPresionadoPrimero = false;
                actualizarPantalla(tiempoAhoraRTC);
                delay(250);
            }
            // Si expiró el tiempo de espera, se procesa como una pulsación de botón individual
            else if (tiempoActualMillis - tiempoPrimerBoton >= ventanaTolerancia) {
                esperandoCombinacion = false;

                if (adelantePresionadoPrimero) {
                    adelantePresionadoPrimero = false;
                    ultimoTiempoBoton = tiempoActualMillis;
                    
                    if (!pantallaBloqueada) {
                        // Cambiar de menú hacia adelante si la pantalla está libre
                        menuActual = static_cast<EstadosPantalla>((menuActual + 1) % TOTAL_MENUS); 
                        actualizarPantalla(tiempoAhoraRTC);
                    } 
                    else if (pantallaBloqueada && menuActual == MENU_ALARMAS && !alarmaActivaYCorriendo) {
                        // Modificar alarma elegida en modo bloqueado
                        alarmaSeleccionada = (alarmaSeleccionada + 1) % TOTAL_OPCIONES;
                        actualizarPantalla(tiempoAhoraRTC);
                    }
                }
                else if (atrasPresionadoPrimero) {
                    atrasPresionadoPrimero = false;
                    ultimoTiempoBoton = tiempoActualMillis;

                    if (!pantallaBloqueada) {
                        // Cambiar de menú hacia atrás si la pantalla está libre
                        menuActual = static_cast<EstadosPantalla>((menuActual - 1 + TOTAL_MENUS) % TOTAL_MENUS); 
                        actualizarPantalla(tiempoAhoraRTC);
                    }
                }
            }
        }
    }

    // --- REDIBUJAR PANTALLA INTELIGENTE POR SEGUNDO ---
    if (menuActual != MENU_OXIMETRO) {
        if (tiempoAhoraRTC.second() != ultimoSegundoRegistrado) {
            ultimoSegundoRegistrado = tiempoAhoraRTC.second();
            actualizarPantalla(tiempoAhoraRTC);
        }
    }
    
    delay(1); 
}