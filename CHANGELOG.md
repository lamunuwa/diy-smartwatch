# CHANGELOG
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
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

## [Unreleased]
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
