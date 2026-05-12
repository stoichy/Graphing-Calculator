# Graphing Calculator

A portable, embedded graphing calculator built with the **ATmega328P** microcontroller. This project combines mathematical computation with real-time graphical visualization on a TFT screen, featuring data logging and audio feedback.

## Introduction

### What does it do?
- **Basic math operations**: Addition (+), Subtraction (-), Multiplication (*), Division (/)
- **Simple cosine and sine graphs**: Just write the amplitude and the phase and you will see the corresponding graph.
- **Dynamic Interaction**: Allows you to zoom in or out using a potentiometer.

### What's its purpose?
Well, if you're trying to do a digital detox and want to feel like an old-school 20th-century math researcher, you have the perfect choice! Not only that, but with the graph options, you can visualize concepts like interference, phase shift, and amplitude without the need for an expensive oscilloscope. Or, if you are an engineer working with signals all day, it can be useful for quick visual verification of parameters before sending data to a Digital-to-Analog Converter (DAC).

### Why use this over alternatives?
Phones are tempting; when you try to do some quick math operations for an exam, you can get Instagram notifications or messages that easily interrupt your focus. If you want to get rid of all digital devices when studying, you can use this calculator. The ultimate goal is creating a full scientific calculator, which implies heavy software optimization due to the limited RAM of the microcontroller.

## Implementation Description
The system is coordinated by the **ATmega328P** microcontroller. The user inputs data using a **4×4 Keypad**. The input software module detects key presses and transmits them to the **Main Logic**, which validates the input and calls the **Display Driver** to update the graphical interface on the **TFT Screen** via the SPI bus. Simultaneously, the logic triggers the **Audio Generator** to emit a confirmation sound through the passive **Buzzer** using a PWM signal. The variable voltage read from the **Potentiometer** adjusts the zoom of the displayed graph. Operations history is written bidirectionally to the **SD Card Module**.

---
![System Diagram](./Graphics%20Computer%20Diagram.png)

## Hardware Design
This section outlines the physical implementation of the system, including components and communication protocols.

### Bill of Materials (BOM)
| Component | Description/Role |
| :--- | :--- |
| **ATmega328P Xplained Mini** | Main MCU (Microcontroller Unit) based on the AVR architecture |
| **1.8” Color TFT LCD (ST7735)** | High-resolution graphical output via SPI protocol |
| **MicroSD Card Module** | External storage for logs/data, sharing the SPI bus |
| **4×4 Matrix Keypad** | Digital input for user commands and data entry |
| **Slide Potentiometer (10kΩ)** | Analog input for variable control (Zoom) |
| **Passive Buzzer** | Audio output device for acoustic feedback and alerts |

### Pin Mapping and Connectivity
| Component | Pin Type | MCU Pin | Function |
| :--- | :--- | :--- | :--- |
| Keypad (Rows) | Digital | D2, D3, D4, D5 | Input scanning (Pull-up) |
| Keypad (Cols) | Digital | D6, D7, D8, D9 | Input scanning |
| Buzzer | PWM | D3 (shared) | Audio signal generation |
| TFT Screen (SCK) | SPI Clock | D13 | Shared SPI clock |
| TFT Screen (MOSI) | SPI Data | D11 | Master Out Slave In |
| TFT Screen (CS/DC/RST) | Control | D10, D9, D8 | Chip Select, Data/Cmd, Reset |
| SD Card (CS) | Control | D4 (shared) | Chip Select for SD Module |
| Potentiometer | Analog | A0 | 10-bit ADC conversion |

### Electrical Schematic Description
The system design follows a modular approach using the **SPI Bus** as the backbone. To avoid bus contention between the TFT Screen and SD Card, separate Chip Select (CS) lines are used.
- **Keypad**: Matrix scanning technique utilizing 8 digital pins.
- **Buzzer**: Connected in series to prevent overcurrent.
- **Potentiometer**: Acts as a voltage divider providing linear 0V-5V input to the ADC.

### Communication Protocols
- **SPI**: SCLK (Synchronization), MOSI (MCU to Peripherals), MISO (SD Card to MCU).
- **PWM**: Timer-based PWM for audio. Frequency defines the note, while a 50% duty cycle ensures clarity.

---

## Software Design
Firmware architecture and logic coordination.

### Development Environment
- **VS Code + PlatformIO**: Used for IntelliSense, unified debugging, and library management.

### 3rd-Party Libraries
- `Adafruit GFX Library`: Core graphics primitives.
- `Adafruit ST7735 Library`: Hardware-specific driver for the TFT display.
- `Keypad Library`: Matrix keypad scanning and debouncing.
- `SD Library`: FAT16/FAT32 file system operations.
- `SPI Library`: High-speed serial communication.

### Algorithms & Data Structures
- **Finite State Machine (FSM)**: Manages modes like `MENU_NAVIGATION`, `DATA_ENTRY`, and `PROCESSING`.
- **Non-Blocking Polling**: Ensures UI responsiveness during input detection.
- **SPI Bus Arbitration**: Manual CS management for shared bus peripherals.
- **Memory Optimization**: Use of the `F()` macro and `PROGMEM` for static strings to fit within 2KB SRAM.

### Key Functional Blocks
- `void setup()`: Initializes peripherals and communication buses.
- `void loop()`: Main cycle handling state transitions.
- `char readKeypad()`: Returns key presses with audio feedback.
- `void updateUI(...)`: High-level screen rendering.
- `bool logToSD(...)`: Encapsulates SD card I/O with timestamps.
- `void playTone(...)`: Drives the buzzer for user alerts.

---

## Obtained Results
- **Hardware Integration**: Successful SPI bus sharing between TFT and SD Card.
- **UI Responsiveness**: Smooth input handling with no blocking delays.
- **Audio Feedback**: Distinct PWM tones for different system states.
- **Stability**: Reliable performance within the strict 2KB RAM constraint.

## Future Improvements
- **Advanced UI**: Custom bitmaps loaded from the SD card.
- **New Modes**: Retro-style games or a secure vault system.

## Download
[GitHub Repository Link](https://github.com/stoichy/Graphing-Calculator)

## Calendar of Progress
| Date | Software | Hardware |
| :--- | :--- | :--- |
| **May 2nd** | Basic math & history implemented. Working on Sin/Cos accuracy. | All jumper wires connected. Planning perfboard soldering. |

## Bibliography / Resources
### Hardware
- [ATmega328P Datasheet](https://www.microchip.com/wwwproducts/en/ATmega328P)
- [ST7735 Controller Datasheet](https://www.adafruit.com/datasheets/ST7735R_V0.2.pdf)

### Software
- [PlatformIO Documentation](https://docs.platformio.org/)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
