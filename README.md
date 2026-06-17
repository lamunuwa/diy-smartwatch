# ExpWatch

**ExpWatch** (Experimental Watch) is an open-source ESP32-based wearable platform created to explore compact electronics, PCB design, embedded firmware, and the challenges involved in building a small smartwatch-like device.

Rather than becoming a commercial product, ExpWatch exists as a learning platform for students, makers, and anyone interested in understanding how wearable embedded systems are built.

Current status: **PCB routing in progress.**

---

## Why ExpWatch?

ExpWatch started as a student and personal challenge to understand how compact wearable devices are designed, from schematic capture and PCB layout to firmware development and final assembly.

The project focuses on:

* Learning wearable smartwatch-like systems.
* Exploring small PCB design.
* Working with SMD components.
* Sharing the entire development process with the community.

ExpWatch is intended for experimentation and education, not mass production.

---

## Hardware

| Component         | Model                |
| ----------------- | -------------------- |
| MCU               | ESP32-WROOM-32       |
| Pulse Sensor      | MAX30100             |
| RTC               | DS3231MZ+            |
| Display           | SSD1306 OLED 128×64  |
| USB-UART          | CH340X               |
| Battery Charger   | TP4056               |
| Voltage Regulator | AMS1117              |
| Battery           | 3.7V Li-Po 200mAh    |
| User Input        | Two buttons          |
| Haptic Feedback   | Coin vibration motor |

---

## Repository Structure

```text
hardware/
├── exports/
├── layout/
└── schematic/

src/
└── modules/

docs/
├── datasheets/
└── images/

fabrication/

.gitignore
AUTHORS.md
CONTRIBUTING.md
CHANGELOG.md
LICENSE
README.md
```
---

## Acknowledgments

ExpWatch is a personal portfolio project, but community ideas are always welcome.

Many future features may originate from suggestions submitted by other people, and implemented suggestions will always receive proper credit in the changelog.
