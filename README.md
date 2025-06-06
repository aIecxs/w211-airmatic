# AIRmatic Air Suspension Level Control DIY Kit

**Description**  
This project provides a DIY electronics kit for Mercedes-Benz vehicles equipped with the AIRmatic air suspension system.  
It enables the individual adjustment of suspension level by manipulating the level sensor signals (0.5 V – 4.5 V range) by up to ±1 V.  
Separate level settings can be configured for different driving modes (Comfort, Sport, Sport 2).

---

**Disclaimer**  
This project is published solely as an informational DIY guide for private, non-commercial use.  
No products, kits, or components are sold, supplied, or placed on the market.  
The author does not act as a manufacturer, distributor, or operator placing products on the market under applicable product safety laws (such as EU Product Safety Directive or national laws).  
Use at your own risk. No warranty is provided.  
**This modification is intended for motorsport or off-road use only and is not approved for use on public roads.**  
Installing this modification may **invalidate your vehicle's operating license and type approval**, and may be subject to local laws and regulations.  
**The user assumes full responsibility for any legal consequences, including but not limited to liability for safety, damages, and compliance with regulations.**

---

**License**  
This project is licensed under the MIT License.  
See the [LICENSE](LICENSE) file for full details.

---

**Credits**

- **Concept and development**  
  Project Author: [aIecxs](https://github.com/aIecxs)  

- **MCP2515 Arduino Library**  
  Library for SPI communication with MCP2515 CAN controllers.  
  [GitHub – autowp/arduino-mcp2515](https://github.com/autowp/arduino-mcp2515)

- **Mercedes-Benz CAN Bus Research**  
  CAN bus reverse engineering by [@rnd-ash](https://github.com/rnd-ash).  
  [GitHub – rnd-ash/mercedes-hacking-docs](https://github.com/rnd-ash/mercedes-hacking-docs/blob/master/Chapter%207%20In%20Car%20Entertainment.md)
  
  ---
  
**`⚠️ UNFINISHED DRAFT – UNDER DEVELOPMENT – CODE BASE FOR PERSONAL ARCHIVING ONLY – LOOSE COLLECTION OF INFORMATION ⚠️`**


  **Offset Voltage**

| PWM  | Res | Duty | mm  | Offset Voltage (V) |
|------|-----|------|-----|--------------------|
| 0%   | 255 | 0    | -50 | -1.0               |
| 5%   | 255 | 11   | -45 | -0.9               |
| 9%   | 255 | 23   | -40 | -0.8               |
| 14%  | 255 | 34   | -35 | -0.7               |
| 18%  | 255 | 46   | -30 | -0.6               |
| 23%  | 255 | 57   | -25 | -0.5               |
| 27%  | 255 | 69   | -20 | -0.4               |
| 32%  | 255 | 80   | -15 | -0.3               |
| 36%  | 255 | 92   | -10 | -0.2               |
| 41%  | 255 | 103  | -5  | -0.1               |
| 45%  | 255 | 115  | 0   | 0.0                |
| 50%  | 255 | 126  | 5   | 0.1                |
| 54%  | 255 | 138  | 10  | 0.2                |
| 59%  | 255 | 149  | 15  | 0.3                |
| 63%  | 255 | 161  | 20  | 0.4                |
| 68%  | 255 | 172  | 25  | 0.5                |
| 72%  | 255 | 184  | 30  | 0.6                |
| 77%  | 255 | 195  | 35  | 0.7                |
| 81%  | 255 | 207  | 40  | 0.8                |
| 86%  | 255 | 218  | 45  | 0.9                |
| 90%  | 255 | 230  | 50  | 1.0                |

---

### AIRmatic → NHRS2 only until 05/2005 or AMG models

#### Front Left Level Sensor (B22/8 → N51)

| Pin | Signal | Pin    | Color | Description |
|-----|--------|--------|--------|-------------|
| 1   | NVLM   | 6      | GYVT (grey-violet) | Ground front left level sensor |
| 5   | NVLV   | 27     | BNWH (brown-white) | Supply front left level sensor |
| 4   | NVLS1  | 38     | GNRD (green-red)   | Signal 1 front left level sensor |
| 6   | NVLS2  | 34     | BKBU (black-blue)  | Signal 2 front left level sensor *(optional)* |

#### Front Right Level Sensor (B22/9 → N51)

| Pin | Signal | Pin    | Color | Description |
|-----|--------|--------|--------|-------------|
| 1   | NVRM   | 30     | BNBK (brown-black) | Ground front right level sensor |
| 5   | NVRV   | 3      | WHBU (white-blue)  | Supply front right level sensor |
| 4   | NVRS1  | 40     | BKYE (black-yellow)| Signal 1 front right level sensor |
| 6   | NVRS2  | 36     | BNRD (brown-red)   | Signal 2 front right level sensor *(optional)* |

#### Rear Axle Level Sensor (B22/3 → N51)

| Pin | Signal | Pin    | Color | Description |
|-----|--------|--------|--------|-------------|
| 1   | NHRM   | 24     | BNWH (brown-white) | Ground rear axle level sensor |
| 5   | NHRV   | 27     | BUWH (blue-white)  | Supply rear axle level sensor |
| 4   | NHRS1  | 25     | BNGY (brown-grey)  | Signal 1 rear axle level sensor |
| 6   | NHRS2  | 21     | YEBK (yellow-black)| Signal 2 rear axle level sensor *(optional)* |

---

### ENR Variant (Swapped NHRS1 / NHRV?) — B22/3 → N97

| Pin | Signal | Pin    | Color | Description |
|-----|--------|--------|--------|-------------|
| 1   | NHRM   | 2      | BNGY (brown-grey)  | Ground rear axle level sensor |
| 4   | NHRV   | 13     | BUWH (blue-white)  | Supply rear axle level sensor *(swapped?)* |
| 5   | NHRS1  | 12     | BNWH (brown-white) | Signal 1 rear axle level sensor *(swapped?)* |
| 6   | NHRS2  | 3      | YEBK (yellow-black)| Signal 2 rear axle level sensor |

---

### Color Code Reference

| Code | Color (EN) | Farbe (DE)     |
|------|------------|----------------|
| BK   | Black      | Schwarz        |
| BN   | Brown      | Braun          |
| BU   | Blue       | Blau           |
| GN   | Green      | Grün           |
| GY   | Grey       | Grau           |
| PK   | Pink       | Rosa           |
| RD   | Red        | Rot            |
| TR   | Transparent| Transparent    |
| VT   | Violet     | Violett        |
| WH   | White      | Weiß           |
| YE   | Yellow     | Gelb           |

---

### Signal Prefix Key

| Prefix | Meaning                    |
|--------|----------------------------|
| N      | Level Sensor (Niveausensor) |
| V      | Front (Vorne)              |
| HR     | Rear Axle (Hinterachse)    |
| L      | Left (Links)               |
| R      | Right (Rechts)             |
| M      | Ground (Masse)             |
| V      | Supply (Versorgung)        |
| S1     | Signal 1                   |
| S2     | Signal 2                   |

---

### PCB Design

The printed circuit board (PCB) layout for this project was created using the **Maker Edition of TARGET 3001!** generously provided for non-commercial DIY use.

> PCB design made with the **TARGET 3001! Maker Edition**  
> With kind permission of **IBF Electronic** – www.target3001.com

Download full schematic and PCB [(LM2902N.T3001)](XGerber.zip)

This license allows free usage exclusively for personal, non-commercial projects. Redistribution or commercial use of the PCB layout created with this software may require a separate license.

---

## Installation

To compile and upload the firmware to your ESP32, follow these steps:

1. **ESP32 Board Support**  
   Make sure you have installed the [ESP32 board package](https://randomnerdtutorials.com/esp32-pinout-reference-gpios) in Arduino IDE.

2. **LittleFS support**  
   For file system operations (e.g. storing presets), install the [ESP32 LittleFS tool for Arduino IDE 2.x](https://randomnerdtutorials.com/arduino-ide-2-install-esp32-littlefs) Add-on.

3. **Fake CH341 Driver (optional)**  
   If you encounter serial driver conflicts under Windows, you may need to install fake CH341 drivers.
   [FakeCH341DriverFixer by SHWotever](https://github.com/SHWotever/FakeCH340DriverFixer)

---

## Wiring

- **Power Supply:**  
  Use a DC-DC buck converter (12 V → 5 V) to supply power to the microcontroller (MCU).

- **Connect PCB to control unit:**  
  Wire battery voltage, ground, and the level sensor signal lines between the PCB and the vehicle's air suspension ECU.
  For a visual guide on wiring, refer to the following video:  
  [Wiring Overview Video](https://www.youtube.com/watch?v=JCMEg7JYTig)

- **Connect MCU to CAN bus module:**  
  Use the SPI interface (MOSI, MISO, SCK, CS, INT) to communicate with the MCP2515 CAN module.

- **Connect MCU to PCB:**  
  Link the MCU PWM output pins to the PCB for analog signal offset control. (refer to [AIRmatic.ino](AIRmatic.ino) for pins)

