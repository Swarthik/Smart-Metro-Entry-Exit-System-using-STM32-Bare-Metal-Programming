#include "stm32f4xx.h"
#include "rc522.h"

#define CS (1U<<4)
#define RST (1U<<0)

static void spi_delay(void){for(volatile int i=0;i<20;i++);}
static uint8_t spi_transfer(uint8_t d){
    while(!(SPI1->SR&SPI_SR_TXE));
    *((volatile uint8_t*)&SPI1->DR)=d;
    while(!(SPI1->SR&SPI_SR_RXNE));
    return *((volatile uint8_t*)&SPI1->DR);
}
static void write_reg(uint8_t a,uint8_t v){GPIOA->BSRR=(CS<<16);spi_transfer((a<<1)&0x7E);spi_transfer(v);GPIOA->BSRR=CS;}
void RC522_Init(void){
    RCC->AHB1ENR|=RCC_AHB1ENR_GPIOAEN|RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR|=RCC_APB2ENR_SPI1EN;
    GPIOA->MODER &= ~((3U<<(4*2))|(3U<<(5*2))|(3U<<(6*2))|(3U<<(7*2)));
    GPIOA->MODER |= (1U<<(4*2))|(2U<<(5*2))|(2U<<(6*2))|(2U<<(7*2));
    GPIOA->AFR[0]|=(5U<<(5*4))|(5U<<(6*4))|(5U<<(7*4));
    GPIOA->BSRR=CS;
    SPI1->CR1=SPI_CR1_MSTR|SPI_CR1_SSM|SPI_CR1_SSI|SPI_CR1_BR_1|SPI_CR1_BR_0;
    SPI1->CR1|=SPI_CR1_SPE;
    GPIOB->MODER|=(1U<<(0*2)); GPIOB->BSRR=RST;
    write_reg(0x01,0x0F);
    write_reg(0x2A,0x8D); write_reg(0x2B,0x3E);
}
uint8_t RC522_IsCardPresent(void){
    /* Recreated placeholder interface: a full anti-collision/UID exchange can be added after hardware validation. */
    return 0;
}
