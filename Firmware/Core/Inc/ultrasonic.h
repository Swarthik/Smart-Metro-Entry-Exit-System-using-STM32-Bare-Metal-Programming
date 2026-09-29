#ifndef ULTRASONIC_H
#define ULTRASONIC_H
#include "stm32f4xx.h"
void Ultrasonic_Init(void);
uint32_t Ultrasonic_ReadEntryCm(void);
uint32_t Ultrasonic_ReadExitCm(void);
#endif
