# Smart Metro Entry/Exit System using STM32 Bare-Metal Programming

A miniature metro-station gate system that counts people entering and leaving an area using ultrasonic sensors, authenticates entry using RFID, and controls an automatic gate using a stepper motor. An LCD displays the current count/status, while an RGB LED and buzzer provide visual/audio alerts.

> **Target board:** STM32 NUCLEO-F401RE (STM32F401RE, ARM Cortex-M4).  
> This repository documents the hardware architecture, interfacing, and operating concept of the project.

## Demo / Prototype

The project was physically implemented as an STM32-based metro entry/exit prototype. The original firmware source is no longer available, so this repository focuses on the project design, hardware interfacing, and working principle.

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

## Hardware Pin Configuration

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

## Embedded / Bare-Metal Concepts

The project was developed around register-level STM32 concepts rather than an Arduino-style framework. The main technical areas involved are:

- GPIO configuration
- SPI1 communication for RFID
- SysTick-based timing
- DWT cycle counter concepts for microsecond timing
- LCD GPIO interfacing
- HC-SR04 pulse measurement
- RC522 SPI communication
- Stepper motor sequencing

These are the key embedded concepts to discuss in an interview. The original firmware source is not included because it is no longer available.

## Firmware Source Status

The original firmware source code is **not available anymore**. The hardware project was completed, but the original source files were lost.

For that reason, this repository intentionally does **not** present newly generated code as the original project firmware. If firmware is recreated in the future, it should be clearly labeled as recreated/reference firmware and tested on the hardware before being used as an exact project record.

## Repository Structure

```text
smart-metro-rfid-baremetal/
|
|-- README.md
|-- LICENSE
|-- docs/
|   |-- wiring_diagram.png
|   `-- project_overview.png
|
`-- media/
    `-- demo.mp4              # optional: add your own video
```

## Project Demonstration Flow

For an interview or demonstration, the intended sequence is:

1. Power the STM32 NUCLEO-F401RE and connected peripherals.
2. A person is detected by the entry HC-SR04 sensor.
3. The user presents an RFID card to the RC522 reader.
4. After authorization, the gate is opened using the stepper motor and ULN2003 driver.
5. The LCD displays the system status and people count.
6. The exit HC-SR04 detects a person leaving and the count is decreased, without allowing the count to become negative.
7. A prolonged obstruction in the sensing area produces the documented warning indication.

This section describes the project operation; it is not a claim that the currently stored repository contains the original firmware.

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

## Original Firmware Note

The project was completed as a hardware/embedded prototype, but the original firmware source is no longer available. Therefore:

- The repository does not claim that any recreated code is the original firmware.
- The documented pin mapping and system behavior are retained as project documentation.
- Any future recreated firmware should be labeled **recreated/reference firmware**.
- Hardware-specific timings, RFID UID handling, motor steps, and sensor thresholds should be validated on the actual prototype before use.

Being transparent about the missing source code keeps the repository aligned with the actual project history.

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

**Swarthik**  
Electronics and Communication Engineering  

Replace this section with your GitHub username, college, and project team members if applicable.

## License

MIT License — see `LICENSE`.
