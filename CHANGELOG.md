# CHANGELOG
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## 2026-06-27
### Added
#### Firmware
- Migrated timekeeping from software-based `RTC_Millis` to hardware-based `RTC_DS3231` for high-precision PCB timing.
- Implemented a Finite State Machine (FSM) for navigation using a `EstadosPantalla` enum with three defined states: `MENU_HORA`, `MENU_ALARMAS` y `MENU_OXIMETRO`.
- Added hardware button control (`BTN_ADELANTE` and `BTN_ATRAS`) featuring basic software debouncing and cyclic menu iteration via modular arithmetic.
- Refactored the display rendering architecture; `actualizarPantalla` now utilizes a `switch` block to independently render active menus, including visual placeholders for upcoming alarm and oximeter sensor integrations.

---

## 2026-06-25
### Added
#### Firmware
- Integrated digital clock functionality using the DS3231 RTC module and `millis()` for timekeeping.
- Configured the SSD1306 OLED display to show the current time and date in real-time.
- Created initial firmware baseline to verify ESP32 core functionality and I2C bus communication with the SSD1306 OLED screen.

### Fixed
#### Documentation
- Corrected typos, titles, and general text formatting across project PDF files.

---

## 2026-06-17
### Added
#### Documentation
- Created `docs/` directory for project reference materials.
- Added `docs/datasheets/` containing the DS3231MZ+ datasheet and the official Espressif ESP32 Hardware Design Guidelines PDF.
- Added `docs/images/` directory and included an image asset for future use.

#### Manufacturing Files
- Created `fabrication/` directory containing all files required for PCB production:
  - Gerber files (`.zip` archive).
  - Bill of Materials (BOM).
  - Pick and Place files for both sides of the PCB (Top/Bottom).

### Exported
#### Documentation
- Exported a 3D view PDF (`3D.pdf`) showing both sides of the PCB layout, saved in `hardware/exports/`.

## 2026-06-11
### Added
#### Hardware Layout
- The same modules and components listed in the latest CHANGELOG update have been added and configured routed.
- The schematic and layout have been completed, and the design has been prepared for the assembly.
- Added `hardware/layout/project.fbrd`.

### Exported
#### Documentation
- Exported the layout PDF from Fusion 360.
- Added the generated file to the `exports/` directory.

## 2026-06-09
### Added
#### Hardware Schematic
- Added all required modules and components for the first hardware revision of the device.
- Integrated:
  - ESP32
  - MAX30100 sensor module
  - DS3131MZ+ RTC
  - CH340X USB-to-UART converter
  - TP4056 Li-Ion battery charger
  - AMS1117 voltage regulator
  - OLED screen module
- Completed schematic connectivity and prepared the design for PCB routing.
- Added `hardware/schematic/project.fsch`.

### Exported
#### Documentation
- Exported the schematic PDF from Fusion 360.
- Added the generated file to the `exports/` directory.
