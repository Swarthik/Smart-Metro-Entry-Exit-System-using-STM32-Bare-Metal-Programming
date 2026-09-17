/*
 * Metro / Smart Gate People Counter
 * Target: STM32 NUCLEO-F401RE (STM32F401RE, Cortex-M4)
 * Bare-metal register-level code; no STM32 HAL.
 *
 * Assumptions:
 *  - RC522 RFID reader on SPI1, 3.3 V
 *  - 2 x HC-SR04 ultrasonic sensors
 *  - 16x2 HD44780 LCD in 4-bit mode
 *  - Common-cathode RGB LED
 *  - Active buzzer
 *  - 28BYJ-48 stepper + ULN2003 driver
 *
 * IMPORTANT:
 * HC-SR04 ECHO is normally 5 V. Use a voltage divider/level shifter
 * before connecting ECHO to the STM32 3.3 V GPIO.
 */

#include "stm32f4xx.h"
#include <stdint.h>

/* -------------------- Pin map --------------------
 * RFID RC522 / SPI1:
 * PA5 SCK, PA6 MISO, PA7 MOSI, PA4 NSS, PB0 RST
 *
 * Entry HC-SR04: PB10 TRIG, PB11 ECHO
 * Exit  HC-SR04: PB12 TRIG, PB13 ECHO
 *
 * LCD 16x2 4-bit:
 * PC0 D4, PC1 D5, PC2 D6, PC3 D7, PC4 RS, PC5 EN
 *
 * RGB LED: PB6 R, PB7 G, PB8 B
 * Buzzer : PB9
 *
 * Stepper ULN2003 IN1..IN4: PA8..PA11
 * -------------------------------------------------*/

/* ---------- Basic GPIO helpers ---------- */
#define SET_BIT(reg, bit) ((reg) |= (1U << (bit)))
#define CLR_BIT(reg, bit) ((reg) &= ~(1U << (bit)))

static void gpio_output(GPIO_TypeDef *g, uint8_t pin)
{
    g->MODER &= ~(3U << (2U * pin));
    g->MODER |=  (1U << (2U * pin));
    g->OTYPER &= ~(1U << pin);
    g->OSPEEDR |= (3U << (2U * pin));
}

static void gpio_input(GPIO_TypeDef *g, uint8_t pin)
{
    g->MODER &= ~(3U << (2U * pin));
}

static void gpio_af(GPIO_TypeDef *g, uint8_t pin, uint8_t af)
{
    g->MODER &= ~(3U << (2U * pin));
    g->MODER |=  (2U << (2U * pin));
    if (pin < 8) {
        g->AFR[0] &= ~(0xFU << (4U * pin));
        g->AFR[0] |=  ((uint32_t)af << (4U * pin));
    } else {
        uint8_t p = pin - 8U;
        g->AFR[1] &= ~(0xFU << (4U * p));
        g->AFR[1] |=  ((uint32_t)af << (4U * p));
    }
}

static void gpio_write(GPIO_TypeDef *g, uint8_t pin, uint8_t value)
{
    if (value) g->BSRR = (1U << pin);
    else       g->BSRR = (1U << (pin + 16U));
}

static uint8_t gpio_read(GPIO_TypeDef *g, uint8_t pin)
{
    return (uint8_t)((g->IDR >> pin) & 1U);
}

/* ---------- Time base ---------- */
volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

static void systick_init(void)
{
    SysTick->LOAD = 16000000U / 1000U - 1U; /* assumes 16 MHz HSI */
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

static uint32_t millis(void)
{
    return ms_ticks;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = millis();
    while ((millis() - start) < ms) {}
}

/* DWT gives microsecond timing on Cortex-M4 */
static void dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < cycles) {}
}

/* ---------- GPIO clock + application GPIO ---------- */
static void gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |
                    RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN;

    /* Ultrasonic */
    gpio_output(GPIOB, 10);
    gpio_input(GPIOB, 11);
    gpio_output(GPIOB, 12);
    gpio_input(GPIOB, 13);

    /* LCD */
    for (uint8_t p = 0; p <= 5; p++) gpio_output(GPIOC, p);

    /* RGB + buzzer */
    for (uint8_t p = 6; p <= 9; p++) gpio_output(GPIOB, p);

    /* Stepper */
    for (uint8_t p = 8; p <= 11; p++) gpio_output(GPIOA, p);

    gpio_write(GPIOB, 6, 0);
    gpio_write(GPIOB, 7, 0);
    gpio_write(GPIOB, 8, 0);
    gpio_write(GPIOB, 9, 0);
}

/* ---------- LCD 16x2 ---------- */
static void lcd_pulse(void)
{
    gpio_write(GPIOC, 5, 1);
    delay_us(2);
    gpio_write(GPIOC, 5, 0);
    delay_us(50);
}

static void lcd_nibble(uint8_t n)
{
    gpio_write(GPIOC, 0, (n >> 0) & 1);
    gpio_write(GPIOC, 1, (n >> 1) & 1);
    gpio_write(GPIOC, 2, (n >> 2) & 1);
    gpio_write(GPIOC, 3, (n >> 3) & 1);
    lcd_pulse();
}

static void lcd_cmd(uint8_t c)
{
    gpio_write(GPIOC, 4, 0);
    lcd_nibble(c >> 4);
    lcd_nibble(c & 0x0F);
    if (c == 0x01 || c == 0x02) delay_ms(2);
}

static void lcd_data(uint8_t d)
{
    gpio_write(GPIOC, 4, 1);
    lcd_nibble(d >> 4);
    lcd_nibble(d & 0x0F);
}

static void lcd_init(void)
{
    delay_ms(20);
    gpio_write(GPIOC, 4, 0);
    lcd_nibble(0x03); delay_ms(5);
    lcd_nibble(0x03); delay_ms(1);
    lcd_nibble(0x03); delay_ms(1);
    lcd_nibble(0x02); delay_ms(1);

    lcd_cmd(0x28); /* 4-bit, 2-line */
    lcd_cmd(0x0C); /* display on */
    lcd_cmd(0x06); /* entry mode */
    lcd_cmd(0x01); /* clear */
}

static void lcd_goto(uint8_t row, uint8_t col)
{
    lcd_cmd((row == 0 ? 0x80 : 0xC0) + col);
}

static void lcd_print(const char *s)
{
    while (*s) lcd_data((uint8_t)*s++);
}

static void lcd_print_num(uint32_t n)
{
    char b[11];
    uint8_t i = 0;
    if (n == 0) { lcd_data('0'); return; }
    while (n && i < sizeof(b)-1) {
        b[i++] = (char)('0' + (n % 10U));
        n /= 10U;
    }
    while (i) lcd_data((uint8_t)b[--i]);
}

/* ---------- RGB + buzzer ---------- */
static void rgb_off(void)
{
    gpio_write(GPIOB, 6, 0);
    gpio_write(GPIOB, 7, 0);
    gpio_write(GPIOB, 8, 0);
}

static void rgb_green(void)
{
    rgb_off();
    gpio_write(GPIOB, 7, 1);
}

static void rgb_red(void)
{
    rgb_off();
    gpio_write(GPIOB, 6, 1);
}

static void buzzer_on(void)  { gpio_write(GPIOB, 9, 1); }
static void buzzer_off(void) { gpio_write(GPIOB, 9, 0); }

/* ---------- Ultrasonic ---------- */
static uint32_t hcsr04_distance_cm(GPIO_TypeDef *trig_g, uint8_t trig,
                                   GPIO_TypeDef *echo_g, uint8_t echo)
{
    gpio_write(trig_g, trig, 0);
    delay_us(3);
    gpio_write(trig_g, trig, 1);
    delay_us(10);
    gpio_write(trig_g, trig, 0);

    uint32_t timeout = millis() + 30;
    while (!gpio_read(echo_g, echo)) {
        if ((int32_t)(millis() - timeout) >= 0) return 999;
    }

    uint32_t start = DWT->CYCCNT;
    uint32_t max_cycles = 30000U * (SystemCoreClock / 1000000U);
    while (gpio_read(echo_g, echo)) {
        if ((DWT->CYCCNT - start) > max_cycles) return 999;
    }

    uint32_t cycles = DWT->CYCCNT - start;
    uint32_t us = cycles / (SystemCoreClock / 1000000U);

    return us / 58U;
}

static uint8_t person_detected(uint32_t cm)
{
    return (cm > 2U && cm < 80U);
}

/* ---------- Stepper motor ---------- */
static const uint8_t step_seq[4] = {0x09, 0x0C, 0x06, 0x03};
static uint8_t step_index = 0;

static void stepper_output(uint8_t p)
{
    gpio_write(GPIOA, 8, (p >> 0) & 1);
    gpio_write(GPIOA, 9, (p >> 1) & 1);
    gpio_write(GPIOA,10, (p >> 2) & 1);
    gpio_write(GPIOA,11, (p >> 3) & 1);
}

static void stepper_step(int dir)
{
    if (dir > 0) step_index = (step_index + 1) & 3;
    else         step_index = (step_index + 3) & 3;
    stepper_output(step_seq[step_index]);
    delay_ms(3);
}

static void gate_open(void)
{
    /* Tune this count for your mechanical gate. */
    for (uint16_t i = 0; i < 256; i++) stepper_step(1);
    stepper_output(0);
}

static void gate_close(void)
{
    for (uint16_t i = 0; i < 256; i++) stepper_step(-1);
    stepper_output(0);
}

static void gate_cycle(void)
{
    rgb_green();
    gate_open();
    delay_ms(2500);
    gate_close();
    rgb_off();
}

/* ---------- SPI1 / RC522 ---------- */
#define RC522_REG_COMMAND     0x01
#define RC522_REG_COMIEN      0x02
#define RC522_REG_DIVIEN      0x03
#define RC522_REG_COMIRQ      0x04
#define RC522_REG_DIVIRQ      0x05
#define RC522_REG_ERROR       0x06
#define RC522_REG_STATUS2     0x08
#define RC522_REG_FIFO_DATA   0x09
#define RC522_REG_FIFO_LEVEL  0x0A
#define RC522_REG_CONTROL     0x0C
#define RC522_REG_BIT_FRAMING 0x0D
#define RC522_REG_COLL        0x0E
#define RC522_REG_MODE        0x11
#define RC522_REG_TX_MODE     0x12
#define RC522_REG_RX_MODE     0x13
#define RC522_REG_TX_CONTROL  0x14
#define RC522_REG_TX_ASK      0x15
#define RC522_REG_CRC_RESULT1 0x21
#define RC522_REG_CRC_RESULT2 0x22
#define RC522_REG_T_MODE      0x2A
#define RC522_REG_T_PRESCALER 0x2B
#define RC522_REG_T_RELOAD_H  0x2C
#define RC522_REG_T_RELOAD_L  0x2D
#define RC522_REG_VERSION     0x37

#define RC522_CMD_IDLE        0x00
#define RC522_CMD_TRANSCEIVE  0x0C
#define RC522_CMD_SOFTRESET   0x0F
#define PICC_CMD_REQA         0x26
#define PICC_CMD_ANTICOLL    0x93

static void spi1_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    gpio_af(GPIOA, 5, 5);
    gpio_af(GPIOA, 6, 5);
    gpio_af(GPIOA, 7, 5);
    gpio_output(GPIOA, 4);
    gpio_output(GPIOB, 0);

    gpio_write(GPIOA, 4, 1);
    gpio_write(GPIOB, 0, 1);

    SPI1->CR1 = SPI_CR1_MSTR |
                SPI_CR1_SSM |
                SPI_CR1_SSI |
                SPI_CR1_BR_1; /* moderate SPI clock */
    SPI1->CR1 |= SPI_CR1_SPE;
}

static uint8_t spi1_txrx(uint8_t d)
{
    while (!(SPI1->SR & SPI_SR_TXE)) {}
    *((volatile uint8_t *)&SPI1->DR) = d;
    while (!(SPI1->SR & SPI_SR_RXNE)) {}
    return *((volatile uint8_t *)&SPI1->DR);
}

static void rc522_write(uint8_t reg, uint8_t value)
{
    gpio_write(GPIOA, 4, 0);
    spi1_txrx((uint8_t)((reg << 1) & 0x7E));
    spi1_txrx(value);
    gpio_write(GPIOA, 4, 1);
}

static uint8_t rc522_read(uint8_t reg)
{
    uint8_t v;
    gpio_write(GPIOA, 4, 0);
    spi1_txrx((uint8_t)(((reg << 1) & 0x7E) | 0x80));
    v = spi1_txrx(0x00);
    gpio_write(GPIOA, 4, 1);
    return v;
}

static void rc522_set_bitmask(uint8_t reg, uint8_t mask)
{
    rc522_write(reg, rc522_read(reg) | mask);
}

static void rc522_clear_bitmask(uint8_t reg, uint8_t mask)
{
    rc522_write(reg, rc522_read(reg) & (uint8_t)~mask);
}

static void rc522_antenna_on(void)
{
    if (!(rc522_read(RC522_REG_TX_CONTROL) & 0x03))
        rc522_set_bitmask(RC522_REG_TX_CONTROL, 0x03);
}

static void rc522_init(void)
{
    gpio_write(GPIOB, 0, 0);
    delay_ms(2);
    gpio_write(GPIOB, 0, 1);
    delay_ms(50);

    rc522_write(RC522_REG_COMMAND, RC522_CMD_SOFTRESET);
    delay_ms(50);

    rc522_write(RC522_REG_T_MODE, 0x8D);
    rc522_write(RC522_REG_T_PRESCALER, 0x3E);
    rc522_write(RC522_REG_T_RELOAD_L, 30);
    rc522_write(RC522_REG_T_RELOAD_H, 0);
    rc522_write(RC522_REG_TX_ASK, 0x40);
    rc522_write(RC522_REG_MODE, 0x3D);

    rc522_antenna_on();
}

static uint8_t rc522_transceive(const uint8_t *send, uint8_t send_len,
                                 uint8_t *recv, uint8_t *recv_len)
{
    uint8_t irq, err, last_bits;
    uint32_t timeout = millis() + 30;

    rc522_write(RC522_REG_COMMAND, RC522_CMD_IDLE);
    rc522_write(RC522_REG_COMIRQ, 0x7F);
    rc522_write(RC522_REG_FIFO_LEVEL, 0x80);

    for (uint8_t i = 0; i < send_len; i++)
        rc522_write(RC522_REG_FIFO_DATA, send[i]);

    rc522_write(RC522_REG_COMMAND, RC522_CMD_TRANSCEIVE);
    rc522_set_bitmask(RC522_REG_BIT_FRAMING, 0x80);

    do {
        irq = rc522_read(RC522_REG_COMIRQ);
        if ((int32_t)(millis() - timeout) >= 0) {
            rc522_clear_bitmask(RC522_REG_BIT_FRAMING, 0x80);
            return 0;
        }
    } while (!(irq & 0x30));

    rc522_clear_bitmask(RC522_REG_BIT_FRAMING, 0x80);

    err = rc522_read(RC522_REG_ERROR);
    if (err & 0x1B) return 0;

    uint8_t n = rc522_read(RC522_REG_FIFO_LEVEL);
    last_bits = rc522_read(RC522_REG_CONTROL) & 0x07;
    if (last_bits) *recv_len = (n - 1U) * 8U + last_bits;
    else           *recv_len = n * 8U;

    if (n > 16) n = 16;
    for (uint8_t i = 0; i < n; i++)
        recv[i] = rc522_read(RC522_REG_FIFO_DATA);

    return 1;
}

static uint8_t rfid_read_uid(uint8_t uid[5])
{
    uint8_t cmd = PICC_CMD_REQA;
    uint8_t atqa[2];
    uint8_t bits = 0;

    rc522_write(RC522_REG_BIT_FRAMING, 0x07);
    if (!rc522_transceive(&cmd, 1, atqa, &bits)) return 0;
    if (bits != 16) return 0;

    rc522_write(RC522_REG_BIT_FRAMING, 0x00);

    uint8_t anticoll[2] = {PICC_CMD_ANTICOLL, 0x20};
    uint8_t response[10];
    uint8_t response_bits = 0;

    if (!rc522_transceive(anticoll, 2, response, &response_bits))
        return 0;

    if (response_bits < 40) return 0;

    uint8_t crc = response[0] ^ response[1] ^ response[2] ^ response[3];
    if (crc != response[4]) return 0;

    for (uint8_t i = 0; i < 5; i++) uid[i] = response[i];

    return 1;
}

/* Set to 0 if you want to replace "any card accepted" with a UID check. */
#define RFID_ACCEPT_ANY_CARD 1

static uint8_t rfid_card_authorized(const uint8_t uid[5])
{
#if RFID_ACCEPT_ANY_CARD
    (void)uid;
    return 1;
#else
    const uint8_t allowed_uid[4] = {0xDE,0xAD,0xBE,0xEF};
    return (uid[0] == allowed_uid[0] &&
            uid[1] == allowed_uid[1] &&
            uid[2] == allowed_uid[2] &&
            uid[3] == allowed_uid[3]);
#endif
}

/* ---------- UI ---------- */
static void display_count(uint32_t count)
{
    lcd_cmd(0x01);
    lcd_goto(0, 0);
    lcd_print("PEOPLE INSIDE");
    lcd_goto(1, 0);
    lcd_print("COUNT: ");
    lcd_print_num(count);
}

static void warning_display(void)
{
    lcd_cmd(0x01);
    lcd_goto(0, 0);
    lcd_print("PLEASE MOVE");
    lcd_goto(1, 0);
    lcd_print("DOOR BLOCKED!");
}

/* ---------- Main application ---------- */
int main(void)
{
    /* Use default HSI = 16 MHz for this simple bare-metal example. */
    SystemCoreClockUpdate();

    gpio_init();
    systick_init();
    dwt_init();
    lcd_init();
    spi1_init();
    rc522_init();

    uint32_t people = 0;

    uint8_t entry_active = 0;
    uint8_t exit_active  = 0;
    uint32_t entry_start = 0;
    uint32_t exit_start  = 0;

    display_count(people);
    rgb_off();
    buzzer_off();

    while (1)
    {
        uint32_t entry_cm = hcsr04_distance_cm(GPIOB, 10, GPIOB, 11);
        uint32_t exit_cm  = hcsr04_distance_cm(GPIOB, 12, GPIOB, 13);

        uint8_t entry_seen = person_detected(entry_cm);
        uint8_t exit_seen  = person_detected(exit_cm);

        /* -------- ENTRY SIDE --------
         * Person is detected -> wait for RFID -> gate opens -> count +1.
         */
        if (entry_seen && !entry_active) {
            entry_active = 1;
            entry_start = millis();

            lcd_cmd(0x01);
            lcd_goto(0,0);
            lcd_print("SCAN RFID CARD");
            lcd_goto(1,0);
            lcd_print("ENTRY");
        }

        if (entry_active) {
            uint8_t uid[5];

            if (rfid_read_uid(uid) && rfid_card_authorized(uid)) {
                people++;
                display_count(people);
                gate_cycle();
                entry_active = 0;
            }
            else if ((millis() - entry_start) >= 10000U) {
                rgb_red();
                buzzer_on();
                warning_display();
            }
        }

        /* Re-arm entry after person leaves the sensing zone. */
        if (!entry_seen && entry_active) {
            if ((millis() - entry_start) < 10000U) {
                entry_active = 0;
                display_count(people);
            }
        }

        /* -------- EXIT SIDE --------
         * Person detected -> count -1 -> gate opens.
         */
        if (exit_seen && !exit_active) {
            exit_active = 1;
            exit_start = millis();

            if (people > 0) people--;

            display_count(people);
            gate_cycle();
        }

        if (exit_active) {
            if ((millis() - exit_start) >= 10000U) {
                rgb_red();
                buzzer_on();
                warning_display();
            }

            if (!exit_seen) {
                exit_active = 0;
                buzzer_off();
                rgb_off();
                display_count(people);
            }
        }

        /* Clear warning once entry person leaves and no exit alarm is active. */
        if (!entry_seen && !exit_seen) {
            buzzer_off();
            rgb_off();
        }

        delay_ms(80);
    }
}
