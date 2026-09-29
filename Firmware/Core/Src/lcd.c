#include "lcd.h"

#define LCD_D4 GPIO_PIN_0
#define LCD_D5 GPIO_PIN_1
#define LCD_D6 GPIO_PIN_2
#define LCD_D7 GPIO_PIN_3
#define LCD_RS GPIO_PIN_4
#define LCD_EN GPIO_PIN_5

#define GPIO_PIN_0 (1U<<0)
#define GPIO_PIN_1 (1U<<1)
#define GPIO_PIN_2 (1U<<2)
#define GPIO_PIN_3 (1U<<3)
#define GPIO_PIN_4 (1U<<4)
#define GPIO_PIN_5 (1U<<5)

static void delay_short(void){ for(volatile uint32_t i=0;i<3000;i++); }
static void set_data(uint8_t n){
    GPIOC->ODR = (GPIOC->ODR & ~0x0FU) | (n & 0x0FU);
}
static void pulse(void){ GPIOC->BSRR=LCD_EN; delay_short(); GPIOC->BSRR=(LCD_EN<<16); delay_short(); }
static void cmd4(uint8_t n){ set_data(n); pulse(); }
static void send4(uint8_t value){
    GPIOC->BSRR=(LCD_RS<<16);
    cmd4(value>>4); cmd4(value&0x0F);
}
static void command(uint8_t c){ send4(c); delay_short(); }
static void data(uint8_t c){
    GPIOC->BSRR=LCD_RS; cmd4(c>>4); cmd4(c&0x0F);
}
void LCD_Init(void){
    RCC->AHB1ENR|=RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER &= ~(0xFFFU);
    GPIOC->MODER |= 0x555U;
    delay_short();
    GPIOC->BSRR=(LCD_RS<<16);
    cmd4(0x03); cmd4(0x03); cmd4(0x03); cmd4(0x02);
    command(0x28); command(0x0C); command(0x06); command(0x01);
}
void LCD_Clear(void){ command(0x01); delay_short(); }
void LCD_SetCursor(uint8_t row,uint8_t col){ command((row?0xC0:0x80)+col); }
void LCD_Print(const char *s){ while(*s) data((uint8_t)*s++); }
void LCD_PrintNumber(int v){
    char b[12]; int i=0;
    if(v==0){data('0');return;}
    if(v<0){data('-');v=-v;}
    while(v){b[i++]=(char)('0'+v%10);v/=10;}
    while(i) data((uint8_t)b[--i]);
}
