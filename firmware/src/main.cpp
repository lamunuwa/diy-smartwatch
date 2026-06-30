/*
 * SmartWatch
 * Version: v1.0.0
 * Description: Final firmware for the smartwatch.
 * - DS3231 RTC synchronization via I2C.
 * - Cyclic state machine with 3 menus.
 * - Asynchronous tolerance window for two-button combinations.
 * - Long press to activate/cancel countdown alarms.
 * - Triggering and noise immunization of the vibration motor.
 * - Asynchronous data acquisition and power management for the MAX30100 oximeter.
 * - Automatic low-power management via Light Sleep mode during inactivity.
 */

// includes
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include "MAX30100_PulseOximeter.h" 
#include "esp_sleep.h" 

#define LED_DEBUG 2

// OLED Display Settings
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define I2C_SDA 21
#define I2C_SCL 22

// Watch Hardware Pins
#define BTN_ADELANTE 26
#define BTN_ATRAS 27
#define RTC_INT_PIN 13   
#define MOTOR_PIN 14     

// Icons
const unsigned char PROGMEM lock_icon[] = {
  0x3c, 0x42, 0x42, 0xff, 0xff, 0xff, 0xff, 0xff
};

const unsigned char PROGMEM unlock_icon[] = {
  0x3c, 0x02, 0x02, 0xff, 0xff, 0xff, 0xff, 0xff
};

const unsigned char PROGMEM heart_icon[] = {
  0x66, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x18, 0x00
};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
PulseOximeter pox; 

RTC_DS3231 rtc; 

// State Machine for Menu Navigation
enum EstadosPantalla { MENU_HORA, MENU_ALARMAS, MENU_OXIMETRO, TOTAL_MENUS };
EstadosPantalla menuActual = MENU_HORA; 

bool pantallaBloqueada = true; 

// Time management for button debouncing and long press detection
unsigned long ultimoTiempoBoton = 0;
const unsigned long tiempoDebounce = 200; 
unsigned long tiempoPresionadoAtras = 0;
bool botonAtrasPresionado = false;
bool ejecutoPulsacionLarga = false;

// Tolerance window for two-button combinations
unsigned long tiempoPrimerBoton = 0;
bool esperandoCombinacion = false;
bool adelantePresionadoPrimero = false;
bool atrasPresionadoPrimero = false;
const unsigned long ventanaTolerancia = 80; 

// Sleep mode
bool pantallEncendida = true;
unsigned long tiempoUltimaAccion = 0;
const unsigned long tiempoInactividad = 15000; 
bool recienDespertado = false; 

// Alarm management
const int TOTAL_OPCIONES = 5;
String listaAlarmas[TOTAL_OPCIONES] = {"30 Seg", "1 Min", "10 Min", "30 Min", "1 Hora"};
int segundosAlarma[TOTAL_OPCIONES] = {30, 60, 600, 1800, 3600};
int alarmaSeleccionada = 0; 

// Control of the countdown alarm
bool alarmaActivaYCorriendo = false;
DateTime tiempoObjetivoAlarma; 

// Motor control
volatile bool alarmaDisparada = false;
unsigned long tiempoEncendidoMotor = 0;
bool motorActivo = false;

// Oximeter data
unsigned long ultimoMuestreoPox = 0;
float bpmActual = 0;
uint8_t spo2Actual = 0;
uint8_t ultimoSegundoRegistrado = 99; 
bool oximetroInicializado = false; 

void actualizarPantalla(DateTime tiempoAhoraRTC);

void onBeatDetected() {
  if (menuActual == MENU_OXIMETRO) {
    Serial.println("[POX] ¡Pulso real detectado!");
  }
}

void arrancarOximetro() {
  Serial.println("[POX] Reiniciando sensor de forma simple...");
  bpmActual = 0;
  spo2Actual = 0;

  if (pox.begin()) {
    pox.setOnBeatDetectedCallback(onBeatDetected);
    pox.setIRLedCurrent(MAX30100_LED_CURR_24MA); 
    delay(100); 
    Wire.setClock(100000); 
    oximetroInicializado = true;
    Serial.println("[POX] Sensor listo y estabilizado.");
  } else {
    oximetroInicializado = false;
    Serial.println("[POX] Error al iniciar sensor.");
  }
}

void apagarOximetroHardware() {
  if (oximetroInicializado) {
    pox.shutdown(); 
    oximetroInicializado = false;
    Serial.println("[POX] Hardware suspendido. LED apagado.");
  }
}

void setup() {
  pinMode(MOTOR_PIN, INPUT_PULLDOWN);
  delay(10); 
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW); 

  pinMode(LED_DEBUG, OUTPUT);
  digitalWrite(LED_DEBUG, HIGH); 
  
  pinMode(BTN_ADELANTE, INPUT_PULLUP);
  pinMode(BTN_ATRAS, INPUT_PULLUP);

  Serial.begin(115200);
  delay(1000); 

  Serial.println("[SISTEMA] Inicializando Smartwatch v1.0.0 Real Hardware...");

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    while(true); 
  }
  
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); 
  delay(200);

  // Initialize the RTC
  if (!rtc.begin()) {
    Serial.println("[RTC] ERROR CRÍTICO: No se detecta el chip DS3231 físico.");
    while(true);
  }

  // Check for power loss and set the calibration date if necessary
  if (rtc.lostPower()) {
    Serial.println("[RTC] Pérdida de energía detectada. Estableciendo fecha de calibración de laboratorio...");
    // Adjust the RTC to the date
    //rtc.adjust(DateTime(2026, 6, 17, 10, 23, 0)); 
  }

  display.clearDisplay();
  display.display();
  
  oximetroInicializado = false;
  tiempoUltimaAccion = millis();
}

void loop() {
  unsigned long tiempoActualMillis = millis();
  
  // I2C bus recovery in case of lockup
  DateTime tiempoAhoraRTC = rtc.now(); 

  if (pantallEncendida && menuActual == MENU_OXIMETRO && oximetroInicializado) {
    pox.update();
  }

  // Notification of wake-up from sleep mode
  if (recienDespertado) {
    digitalWrite(MOTOR_PIN, LOW); 
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);

    display.ssd1306_command(SSD1306_DISPLAYON);
    pantallEncendida = true;
    tiempoUltimaAccion = millis();
    ultimoTiempoBoton = millis() + 300; 
    esperandoCombinacion = false;
    recienDespertado = false;
    
    if(menuActual == MENU_OXIMETRO) {
      arrancarOximetro();
    }

    tiempoAhoraRTC = rtc.now();
    actualizarPantalla(tiempoAhoraRTC);
    return; 
  }

  // Alarm trigger check
  if (alarmaActivaYCorriendo && tiempoAhoraRTC.unixtime() >= tiempoObjetivoAlarma.unixtime()) {
    alarmaDisparada = true;
  }

  // Activity detection
  bool leerBtnAdelante = (digitalRead(BTN_ADELANTE) == LOW);
  bool leerBtnAtras = (digitalRead(BTN_ATRAS) == LOW);

  if (leerBtnAdelante || leerBtnAtras || alarmaDisparada) {
      tiempoUltimaAccion = tiempoActualMillis; 
  }

  if (menuActual == MENU_OXIMETRO && bpmActual > 0) {
      tiempoUltimaAccion = tiempoActualMillis;
  }

  // Large press detection for the back button to activate/cancel
  if (leerBtnAtras && !botonAtrasPresionado && !esperandoCombinacion && pantallaBloqueada && menuActual == MENU_ALARMAS) {
    botonAtrasPresionado = true;
    tiempoPresionadoAtras = tiempoActualMillis;
    ejecutoPulsacionLarga = false;
  } 
  else if (botonAtrasPresionado) {
    if (leerBtnAtras && (tiempoActualMillis - tiempoPresionadoAtras >= 1000) && !ejecutoPulsacionLarga) {
      if (!alarmaActivaYCorriendo) {
        tiempoObjetivoAlarma = tiempoAhoraRTC + TimeSpan(segundosAlarma[alarmaSeleccionada]);
        alarmaActivaYCorriendo = true;
      }
      else {
        alarmaActivaYCorriendo = false;
      }
      ejecutoPulsacionLarga = true; 
    }
    else if (!leerBtnAtras) {
      botonAtrasPresionado = false;
    }
  }

  // Machine state for menu navigation and button combination detection
  if ((tiempoActualMillis - ultimoTiempoBoton) > tiempoDebounce && !ejecutoPulsacionLarga) {
    
    if (leerBtnAdelante && leerBtnAtras) {
      pantallaBloqueada = !pantallaBloqueada;
      ultimoTiempoBoton = tiempoActualMillis;
      esperandoCombinacion = false;
      adelantePresionadoPrimero = false;
      atrasPresionadoPrimero = false;
      actualizarPantalla(tiempoAhoraRTC); 
      delay(250); 
    }
    else if (leerBtnAdelante && !esperandoCombinacion) {
      tiempoPrimerBoton = tiempoActualMillis;
      esperandoCombinacion = true;
      adelantePresionadoPrimero = true;
      atrasPresionadoPrimero = false;
    }
    else if (leerBtnAtras && !esperandoCombinacion && !botonAtrasPresionado) {
      tiempoPrimerBoton = tiempoActualMillis;
      esperandoCombinacion = true;
      atrasPresionadoPrimero = true;
      adelantePresionadoPrimero = false;
    }

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
      else if (tiempoActualMillis - tiempoPrimerBoton >= ventanaTolerancia) {
        esperandoCombinacion = false;

        if (adelantePresionadoPrimero) {
          adelantePresionadoPrimero = false;
          ultimoTiempoBoton = tiempoActualMillis;
          
          if (!pantallaBloqueada) {
            apagarOximetroHardware(); 
            menuActual = static_cast<EstadosPantalla>((menuActual + 1) % TOTAL_MENUS); 
            if (menuActual == MENU_OXIMETRO) {
              arrancarOximetro(); 
              bpmActual = 0; spo2Actual = 0;
            }
            actualizarPantalla(tiempoAhoraRTC);
          } 
          else if (pantallaBloqueada && menuActual == MENU_ALARMAS && !alarmaActivaYCorriendo) {
            alarmaSeleccionada = (alarmaSeleccionada + 1) % TOTAL_OPCIONES;
            actualizarPantalla(tiempoAhoraRTC);
          }
        }
        else if (atrasPresionadoPrimero) {
          atrasPresionadoPrimero = false;
          ultimoTiempoBoton = tiempoActualMillis;

          if (!pantallaBloqueada) {
            apagarOximetroHardware(); 
            menuActual = static_cast<EstadosPantalla>((menuActual - 1 + TOTAL_MENUS) % TOTAL_MENUS); 
            if (menuActual == MENU_OXIMETRO) {
              arrancarOximetro(); 
              bpmActual = 0; spo2Actual = 0;
            }
            actualizarPantalla(tiempoAhoraRTC);
          }
        }
      }
    }
  }

  // Motor control
  if (alarmaDisparada) {
    alarmaDisparada = false;
    alarmaActivaYCorriendo = false; 
    digitalWrite(MOTOR_PIN, HIGH);
    tiempoEncendidoMotor = tiempoActualMillis;
    motorActivo = true;
  }

  if (motorActivo && (tiempoActualMillis - tiempoEncendidoMotor >= 1000)) {
    digitalWrite(MOTOR_PIN, LOW);
    motorActivo = false;
  }

  // Oximeter data acquisition and display update
  if (menuActual == MENU_OXIMETRO && oximetroInicializado && (tiempoActualMillis - ultimoMuestreoPox > 500)) {
    bpmActual = pox.getHeartRate();
    spo2Actual = pox.getSpO2();
    ultimoMuestreoPox = tiempoActualMillis;
    actualizarPantalla(tiempoAhoraRTC); 
  }

  // Rewrite the screen every second
  if (menuActual != MENU_OXIMETRO) {
    if (tiempoAhoraRTC.second() != ultimoSegundoRegistrado) {
      ultimoSegundoRegistrado = tiempoAhoraRTC.second();
      actualizarPantalla(tiempoAhoraRTC);
    }
  }
  
  // Entry into sleep mode
  if (pantallEncendida && (tiempoActualMillis - tiempoUltimaAccion > tiempoInactividad) && !motorActivo) {
    Serial.println("[SLEEP] Suspendiendo...");
    
    apagarOximetroHardware(); 
    pantallaBloqueada = true; 
    esperandoCombinacion = false;

    display.ssd1306_command(SSD1306_DISPLAYOFF); 
    digitalWrite(LED_DEBUG, LOW);
    
    digitalWrite(MOTOR_PIN, LOW);
    pinMode(MOTOR_PIN, INPUT_PULLDOWN); 
    pantallEncendida = false;

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, 0); 
    esp_sleep_enable_ext1_wakeup((1ULL << BTN_ATRAS), ESP_EXT1_WAKEUP_ALL_LOW); 

    delay(200);
    esp_light_sleep_start(); 

    recienDespertado = true; 
  }

  delay(1); 
}

void actualizarPantalla(DateTime tiempoAhoraRTC) {
  display.clearDisplay();
  display.setTextSize(1);            
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0); 
  display.print("Your title here");
  
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
        long segundosRestantes = tiempoObjetivoAlarma.unixtime() - tiempoAhoraRTC.unixtime();
        if (segundosRestantes < 0) segundosRestantes = 0;

        display.setTextSize(1);            
        display.setCursor(0, 20); 
        display.print("TIEMPO RESTANTE:");

        display.setTextSize(2);
        display.setCursor(30, 36);
        display.print(segundosRestantes);
        display.setTextSize(1);
        display.print(" s");

        display.fillRect(0, 56, 128, 8, SSD1306_BLACK); 
        display.setCursor(0, 56);
        display.print("ATRAS = cancelar");
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
      
      if (oximetroInicializado) {
        if(bpmActual > 10 && bpmActual < 220) {
          display.print((int)bpmActual);
          display.print(" BPM");
        } else {
          display.print("Coloque dedo...");
        }

        display.setCursor(24, 48);
        if(spo2Actual > 50 && spo2Actual <= 100) {
          display.print("SpO2: ");
          display.print(spo2Actual);
          display.print("%");
        } else {
          display.print("Leyendo...");
        }
      } else {
        display.print("Error Sensor");
        display.setCursor(24, 48);
        display.print("Reintentando...");
      }
      break;
    }
  }
  display.display(); 
}