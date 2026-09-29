#include "stm32f4xx.h"
#include "stepper.h"

static const uint16_t seq[4]={0x0100,0x0200,0x0400,0x0800};
static void delay(void){for(volatile uint32_t i=0;i<60000;i++);}
static void write_seq(uint16_t p){GPIOA->BSRR=(uint32_t)p;delay();GPIOA->BSRR=((uint32_t)p<<16);}
void Stepper_Init(void){
    RCC->AHB1ENR|=RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER &= ~(0xFFFFU<<16);
    GPIOA->MODER |=  (0x5555U<<16);
}
static void move(int dir){
    for(int n=0;n<256;n++) for(int i=0;i<4;i++){int k=dir?i:3-i;write_seq(seq[k]);}
}
void Stepper_Open(void){move(1);}
void Stepper_Close(void){move(0);}
