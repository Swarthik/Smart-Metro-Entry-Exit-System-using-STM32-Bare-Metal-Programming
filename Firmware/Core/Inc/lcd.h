#ifndef LCD_H
#define LCD_H

#include "stm32f4xx.h"

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_Print(const char *text);
void LCD_PrintNumber(int value);

#endif
