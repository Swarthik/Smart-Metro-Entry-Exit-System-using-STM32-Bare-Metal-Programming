# Smart Metro Entry/Exit System using STM32 Bare-Metal Programming

A miniature metro-station gate system that counts people entering and leaving an area using ultrasonic sensors, authenticates entry using RFID, and controls an automatic gate using a stepper motor. An LCD displays the current count/status, while an RGB LED and buzzer provide visual/audio alerts.

> **Target board used for this reference implementation:** STM32 NUCLEO-F401RE (STM32F401RE, ARM Cortex-M4).  
> The code is written at register level and does **not** use the STM32 HAL.

## Demo / Prototype

The original prototype video was used as the reference for the project concept. Add your own demo video to the repository if you have it, for example:

```text
media/demo.mp4
```

## Project Overview

The system behaves like a simplified metro entry/exit counter:

- The **entry ultrasonic sensor** detects a person approaching the entrance.
- The controller waits for an **RFID card** to be presented.
- If the RFID card is accepted, the **stepper-motor gate opens**.
- The **green RGB LED** indicates normal/authorized operation.
- The number of people inside is increased by 1.
- The LCD continuously shows the number of people inside and system messages.
- If a person remains in the sensing area for more than **10 seconds**, the system enters a warning state: **red LED + buzzer + LCD warning**.
- The **exit ultrasonic sensor** detects a person leaving, decreases the count by 1, and opens the gate.
- The count is prevented from becoming negative.

## Main Features

1. RFID-based entry authorization
2. Automatic gate control using a stepper motor
3. Bidirectional people counting
4. Ultrasonic entry and exit detection
5. LCD status display
6. RGB status indication
7. 10-second obstruction/standing warning
8. Buzzer alert
9. Bare-metal STM32 peripheral programming
10. No Arduino framework and no STM32 HAL in the application code

## Hardware Required

| Component | Qty | Purpose |
|---|---:|---|
| STM32 NUCLEO-F401RE | 1 | Main controller |
| RC522 RFID reader | 1 | RFID authentication |
| RFID card/tag | 1+ | User identification |
| HC-SR04 ultrasonic sensor | 2 | Entry and exit detection |
| 16x2 HD44780 LCD | 1 | Count/status display |
| RGB LED | 1 | Green/red status |
| Buzzer | 1 | Warning indication |
| 28BYJ-48 stepper motor | 1 | Gate movement |
| ULN2003 stepper driver | 1 | Drives stepper motor |
| 5 V supply | 1 | Ultrasonic/stepper/LCD as required |
| Jumper wires/breadboard | - | Connections |

## Block Diagram

```text
                  +----------------------+
                  |  STM32 NUCLEO-F401RE|
                  |    ARM Cortex-M4     |
                  +----------+-----------+
                             |
       +---------------------+----------------------+
       |             |             |        |        |
       v             v             v        v        v
   RC522 RFID   Entry HC-SR04  Exit HC-SR04 LCD   RGB/Buzzer
       |             |             |        |        |
       +-------------+-------------+--------+--------+
                             |
                             v
                     Stepper Motor Driver
                             |
                             v
                         Gate Motor
```

## Operating Logic

### Entry

```text
Person detected by Entry US
          |
          v
    Wait for RFID card
          |
          v
   RFID card detected?
      /          \
    NO            YES
    |              |
    |              v
    |       Increase count
    |              |
    |              v
    |       Green LED ON
    |              |
    |              v
    |       Open gate
    |              |
    |              v
    |       Wait -> Close gate
    |
    +----> If occupied > 10 s:
             Red LED + Buzzer
             LCD warning
```

### Exit

```text
Person detected by Exit US
          |
          v
      Count > 0 ?
          |
          v
       Count - 1
          |
          v
      Green LED ON
          |
          v
       Open gate
          |
          v
       Close gate
```

## Pin Configuration Used by the Example Code

### RC522 / SPI1

| RC522 | STM32 |
|---|---|
| SDA/SS | PA4 |
| SCK | PA5 / SPI1_SCK |
| MISO | PA6 / SPI1_MISO |
| MOSI | PA7 / SPI1_MOSI |
| RST | PB0 |
| 3.3V | 3.3V |
| GND | GND |

### Ultrasonic Sensors

| Function | TRIG | ECHO |
|---|---|---|
| Entry sensor | PB10 | PB11 |
| Exit sensor | PB12 | PB13 |

### LCD 16x2 — 4-bit parallel mode

| LCD | STM32 |
|---|---|
| D4 | PC0 |
| D5 | PC1 |
| D6 | PC2 |
| D7 | PC3 |
| RS | PC4 |
| EN | PC5 |
| RW | GND |

### RGB LED / Buzzer

| Device | STM32 |
|---|---|
| RGB Red | PB6 |
| RGB Green | PB7 |
| RGB Blue | PB8 |
| Buzzer | PB9 |

### Stepper Motor

The STM32 does **not** drive the stepper coil directly.

```text
STM32 PA8  -> ULN2003 IN1
STM32 PA9  -> ULN2003 IN2
STM32 PA10 -> ULN2003 IN3
STM32 PA11 -> ULN2003 IN4

ULN2003 -> 28BYJ-48 stepper motor
```

See `docs/wiring_diagram.png` for the visual wiring reference.

## Important Electrical Notes

### 1. HC-SR04 ECHO voltage

The HC-SR04 commonly operates from 5 V and its ECHO output can reach 5 V. The STM32 GPIO is a 3.3 V device. **Do not connect a 5 V ECHO signal directly to a non-5-V-tolerant STM32 pin.** Use a resistor divider or suitable level shifter.

Example divider:

```text
HC-SR04 ECHO --- 1 kΩ ---+--- STM32 ECHO pin
                         |
                        2 kΩ
                         |
                        GND
```

This gives approximately 3.33 V from a 5 V input.

### 2. RC522

Use **3.3 V logic/power** for the RC522 module unless your particular module explicitly supports another voltage.

### 3. Stepper motor

Use the ULN2003 driver board and a suitable external motor supply. Do not power the stepper coils directly from an STM32 GPIO.

### 4. RGB LED

Use current-limiting resistors for the RGB LED channels.

## Bare-Metal Implementation

The project demonstrates register-level programming for:

- GPIO
- SPI1
- SysTick
- DWT cycle counter for microsecond timing
- LCD GPIO interface
- HC-SR04 pulse measurement
- RC522 SPI communication
- Stepper motor sequencing

Instead of functions such as `HAL_GPIO_WritePin()`, the program directly accesses STM32 peripheral registers such as:

```c
GPIOB->BSRR = ...;
RCC->AHB1ENR |= ...;
SPI1->CR1 = ...;
```

This makes the project useful for learning how MCU peripherals work below the abstraction layer.

## Software Requirements

- STM32CubeIDE or another ARM GCC-based STM32 development environment
- STM32F4 CMSIS/device header files
- ST-LINK debugger/programmer integrated in the Nucleo board
- `stm32f4xx.h` from the STM32F4 device package

The supplied `main.c` is intended to be placed inside an STM32F401RE project generated with the appropriate startup code and linker script.

## Repository Structure

```text
smart-metro-rfid-baremetal/
|
|-- README.md
|-- LICENSE
|-- src/
|   `-- main.c
|
|-- docs/
|   |-- wiring_diagram.png
|   `-- project_overview.png
|
`-- media/
    `-- demo.mp4              # optional: add your own video
```

## How to Use

1. Create an STM32CubeIDE project for **NUCLEO-F401RE / STM32F401RE**.
2. Configure the project for the device's CMSIS headers and startup files.
3. Replace the generated application `main.c` with the supplied `src/main.c`.
4. Build the project.
5. Connect the peripherals according to the pin table.
6. Flash the Nucleo board using ST-LINK.
7. Present an RFID card at the RC522 reader.
8. Walk through the entry ultrasonic sensing area.
9. Observe the LCD count, RGB LED, and motorized gate.
10. Walk through the exit sensor and verify that the count decreases.

## RFID Configuration

The reference code is initially configured to accept any readable RFID card:

```c
#define RFID_ACCEPT_ANY_CARD 1
```

For a real access-control system, change this to `0` and place the authorized UID in the UID comparison section.

**Do not publish private credentials or sensitive production access data in a public repository.**

## Expected LCD Messages

Example messages include:

```text
PEOPLE INSIDE
COUNT: 3
```

```text
SCAN RFID CARD
ENTRY
```

```text
PLEASE MOVE
DOOR BLOCKED!
```

## Alert Condition

If the system detects that a person remains in the sensing area for approximately 10 seconds, it activates:

- Red RGB LED
- Buzzer
- LCD warning message

This is intended to demonstrate abnormal-condition detection, similar to an automated metro gate monitoring system.

## Limitations of This Reference Implementation

This code was generated from the project concept and component list because the original source code was not available. Therefore, it should be treated as a **reference implementation** and tested/tuned on the actual hardware before being described as the exact original firmware.

The following may require adjustment for your hardware:

- Ultrasonic sensor detection distance
- Stepper motor direction
- Number of steps required for the physical gate angle
- RFID card UID
- LCD wiring
- RGB LED common-anode/common-cathode logic
- Buzzer active/high or active/low behavior
- Timing values

## Future Improvements

- Use a proper state machine for entry/exit sequencing.
- Store authorized RFID UIDs in non-volatile memory.
- Add EEPROM/Flash-based count recovery after reset.
- Add maximum-capacity control.
- Add an emergency stop/manual override.
- Add door-position limit switches.
- Add a real-time clock for access logging.
- Send entry/exit logs to a PC/cloud server through UART/Wi-Fi.
- Use a dedicated motor driver and mechanical gate feedback.
- Add anti-tailgating detection using multiple sensors.

## Learning Outcomes

This project provides practical experience with:

- ARM Cortex-M microcontrollers
- STM32 register-level programming
- GPIO configuration
- SPI communication
- RFID interfacing
- Ultrasonic distance measurement
- LCD interfacing
- Stepper motor control
- Embedded timing
- Finite-state-machine concepts
- Hardware/software integration

## Author

**Your Name**  
Electronics and Communication Engineering  

Replace this section with your GitHub username, college, and project team members if applicable.

## License

MIT License — see `LICENSE`.
