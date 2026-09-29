#include "stm32f4xx.h"
#include "lcd.h"
#include "ultrasonic.h"
#include "stepper.h"
#include "rc522.h"

#define RED   (1U<<6)
#define GREEN (1U<<7)
#define BLUE  (1U<<8)
#define BUZZ  (1U<<9)
#define ENTRY_DISTANCE_CM 40
#define EXIT_DISTANCE_CM 40

static void delay_ms(uint32_t ms){while(ms--)for(volatile uint32_t i=0;i<16000;i++);}
static void status_init(void){
    RCC->AHB1ENR|=RCC_AHB1ENR_GPIOBEN;
    GPIOB->MODER &= ~(0xFFFFU<<12);
    GPIOB->MODER |=  (0x5555U<<12);
}
static void green(void){GPIOB->BSRR=GREEN;GPIOB->BSRR=(RED<<16)|(BLUE<<16);}
static void red_alarm(void){GPIOB->BSRR=RED|BUZZ;GPIOB->BSRR=(GREEN<<16)|(BLUE<<16);}
static void normal(void){GPIOB->BSRR=(RED<<16)|(GREEN<<16)|(BLUE<<16)| (BUZZ<<16);}
int main(void){
    status_init(); LCD_Init(); Ultrasonic_Init(); Stepper_Init(); RC522_Init();
    int people=0;
    LCD_Clear(); LCD_SetCursor(0,0); LCD_Print("METRO SYSTEM"); delay_ms(1000);
    while(1){
        uint32_t in=Ultrasonic_ReadEntryCm();
        uint32_t out=Ultrasonic_ReadExitCm();
        if(in<ENTRY_DISTANCE_CM){
            LCD_Clear();LCD_SetCursor(0,0);LCD_Print("SCAN RFID");
            /* RFID_IsCardPresent() must be connected to the project's real UID logic. */
            if(RC522_IsCardPresent()){
                people++; green(); Stepper_Open(); delay_ms(1500); Stepper_Close();
            }
        }
        if(out<EXIT_DISTANCE_CM && people>0){
            people--; green(); Stepper_Open(); delay_ms(1500); Stepper_Close();
        }
        if(in<ENTRY_DISTANCE_CM && out<EXIT_DISTANCE_CM){
            /* Reserved for obstruction/overlap handling. */
        }
        LCD_Clear();LCD_SetCursor(0,0);LCD_Print("PEOPLE: ");LCD_PrintNumber(people);
        normal(); delay_ms(150);
    }
}
