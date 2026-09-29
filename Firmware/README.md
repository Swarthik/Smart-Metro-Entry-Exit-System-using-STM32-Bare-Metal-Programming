# Firmware

STM32F401RE firmware for the Smart Metro Entry/Exit System.

## Target
- STM32 NUCLEO-F401RE
- ARM Cortex-M4
- Bare-metal/register-level peripheral approach

## Modules
- GPIO and timing
- 16x2 LCD
- RC522 RFID over SPI1
- HC-SR04 entry/exit sensing
- 28BYJ-48 stepper motor through ULN2003
- RGB LED and buzzer
- Entry/exit people-counting state machine

## Pin mapping
RC522: PA4-PA7, RST PB0  
Entry ultrasonic: TRIG PB10, ECHO PB11  
Exit ultrasonic: TRIG PB12, ECHO PB13  
LCD: PC0-PC5  
RGB: PB6-PB8  
Buzzer: PB9  
Stepper: PA8-PA11

The source in this directory is a recreated firmware implementation based on the project's documented hardware design. It should be validated on the physical hardware before being treated as production firmware.
