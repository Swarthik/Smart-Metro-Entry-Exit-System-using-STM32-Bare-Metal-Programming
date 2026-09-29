#include "ultrasonic.h"

#define ENTRY_TRIG (1U<<10)
#define ENTRY_ECHO (1U<<11)
#define EXIT_TRIG  (1U<<12)
#define EXIT_ECHO  (1U<<13)

static void delay_us(uint32_t us){ while(us--){ for(volatile uint32_t i=0;i<16;i++); } }
static uint32_t measure(uint32_t trig,uint32_t echo){
    GPIOB->BSRR=(trig<<16); delay_us(2); GPIOB->BSRR=trig; delay_us(10); GPIOB->BSRR=(trig<<16);
    uint32_t timeout=60000;
    while(!(GPIOB->IDR&echo) && --timeout); if(!timeout)return 999;
    uint32_t t=0; while((GPIOB->IDR&echo) && t<60000){delay_us(1);t++;}
    return t/58;
}
void Ultrasonic_Init(void){
    RCC->AHB1ENR|=RCC_AHB1ENR_GPIOBEN;
    GPIOB->MODER &= ~((3U<<(10*2))|(3U<<(11*2))|(3U<<(12*2))|(3U<<(13*2)));
    GPIOB->MODER |= (1U<<(10*2))|(1U<<(12*2));
}
uint32_t Ultrasonic_ReadEntryCm(void){return measure(ENTRY_TRIG,ENTRY_ECHO);}
uint32_t Ultrasonic_ReadExitCm(void){return measure(EXIT_TRIG,EXIT_ECHO);}
