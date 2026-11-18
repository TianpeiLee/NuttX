/****************************************************************************
 * boards/risc-v/ch32h41x/ch32h41x-evb/boot/boot_main.c
 *
 * V3F boot loader - minimal startup for the dual-core split:
 *
 *   1. console + PB1 up at the reset clock (25MHz HSI), one-line banner
 *   2. the 400MHz SYSPLL sequence (V5F core = 400MHz, HCLK = 100MHz)
 *   3. hand the V5F its entry (flash 0x08010000, fetched through the
 *      0x00010000 zero-wait window): PFIC WAKEIP[1], then SCTLR.SYSRET
 *      starts the fetch (same sequence as the EVT NVIC_WakeUp_V5F)
 *   4. blink PB1 forever as the boot-alive indicator; the V5F NuttX
 *      image owns USART1 from the wake on
 *
 * The whole loader runs in well under a second: no staged validation
 * loops - the boot is proven, the LED is the indicator.
 *
 * BSHR is a write-only trigger register (read-back is 0, writing 0 is a
 * no-op): the LED toggles with direct single writes, never read-
 * modify-write, which would lose the reset bit and latch the pin.
 ****************************************************************************/

#include <stdint.h>

#include "hardware/ch32h41x_memorymap.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_usart.h"

/* PFIC absolute addresses - WAKEIP/SCTLR offsets per the EVT PFIC_Type
 * struct, calibrated against the chip (WAKEIP[0] = 0xE000E720). */

#define PFIC_WAKEIP_V5F   0xE000E724ul
#define PFIC_SCTLR        0xE000ED10ul

/* USART1: TX only, polled.  PA9 = TX (AF7). */

#define USART1            CH32H41X_USART1_BASE
#define USART_STATR       (USART1 + CH32H41X_USART_STATR)
#define USART_DATAR       (USART1 + CH32H41X_USART_DATAR)
#define USART_BRR         (USART1 + CH32H41X_USART_BRR)
#define USART_CTLR1       (USART1 + CH32H41X_USART_CTLR1)

#define USART_SR_TXE      CH32H41X_USART_SR_TXEF
#define USART_CR1_UE      CH32H41X_USART_CR1_UE
#define USART_CR1_TE      CH32H41X_USART_CR1_TE

/* Boot-alive LED = PB1 (the EVT EVB heartbeat pin; PA2 stays free for
 * the V5F image). */

#define BOOT_LED_BSHR     (CH32H41X_GPIOB_BASE + 0x10)
#define BOOT_LED_SET      0x0002ul            /* PB1 high (BSHR.BS1)  */
#define BOOT_LED_CLR      0x00020000ul        /* PB1 low  (BSHR.BR1)  */

static inline void putreg16(uint16_t v, uintptr_t a)
{
  *(volatile uint16_t *)a = v;
}

static inline void putreg32(uint32_t v, uintptr_t a)
{
  *(volatile uint32_t *)a = v;
}

static inline uint32_t getreg32(uintptr_t a)
{
  return *(volatile uint32_t *)a;
}

static inline void boot_modifyreg32(uintptr_t addr, uint32_t clrbits,
                                    uint32_t setbits)
{
  putreg32((getreg32(addr) & ~clrbits) | setbits, addr);
}

extern void boot_setclock_400m(void);
extern uint32_t boot_hclk_frequency(void);

static void boot_uart_init(uint32_t hbclock)
{
  uint32_t baud = 115200ul;
  uint32_t div = (25 * hbclock) / (4 * baud);
  uint32_t mantissa = (div / 100) << 4;
  uint32_t frac = div - 100 * (mantissa >> 4);

  frac = ((frac * 16) + 50) / 100;
  if (frac > 0xf)
    {
      mantissa += 1 << 4;
    }

  /* GPIOA + AFIO + USART1 clocks; PA9 = USART1 TX (AF7, push-pull) -
   * PA9's mode/cnf is CFGHR bits 7:4, its AF number (7) is AFHR bits
   * 7:4.  Both writes are required or the pin never reaches the UART. */

  boot_modifyreg32(CH32H41X_RCC_HB2PCENR, 0,
                   CH32H41X_RCC_HB2_GPIOA | CH32H41X_RCC_HB2_AFIO |
                   CH32H41X_RCC_HB2_USART1);
  boot_modifyreg32(CH32H41X_GPIOA_BASE + 0x04, 0xf0ul, 0xb0ul);
  boot_modifyreg32(0x40010008, 0xf0ul, 0x70ul);

  /* 16-bit register writes: 32-bit writes to these APB registers do
   * not take effect reliably (same class as the I2C DATAR issue). */

  putreg16((uint16_t)(USART_CR1_UE | USART_CR1_TE), USART_CTLR1);
  putreg16((uint16_t)(mantissa | frac), USART_BRR);
}

static void boot_putc(char c)
{
  while ((getreg32(USART_STATR) & USART_SR_TXE) == 0)
    {
    }

  putreg16((uint16_t)(uint8_t)c, USART_DATAR);
}

static void boot_puts(const char *s)
{
  while (*s)
    {
      if (*s == '\n')
        {
          boot_putc('\r');
        }

      boot_putc(*s++);
    }
}

static void boot_delay(volatile unsigned n)
{
  while (n--)
    {
    }
}

void boot_main(void)
{
  /* Clocks on GPIOA (console) and GPIOB (LED), PB1 as output, console
   * up at the reset clock, one-line banner. */

  boot_modifyreg32(CH32H41X_RCC_HB2PCENR, 0,
                   CH32H41X_RCC_HB2_GPIOA | CH32H41X_RCC_HB2_GPIOB);
  boot_modifyreg32(CH32H41X_GPIOB_BASE + 0x00, 0xf0ul, 0x30ul);
  boot_uart_init(25000000ul);
  boot_puts("V3F boot (HSI)\n");

  /* 400MHz SYSPLL + HCLK=100MHz (the V5F NuttX frequency-identifies
   * this configuration in its own start-up). */

  boot_setclock_400m();

  /* Hand the V5F its entry: WAKEIP[1] first, then SCTLR.SYSRET makes
   * the V5F fetch from it; the delay mirrors the DFU's Delay_Us(100)
   * after NVIC_WakeUp_V5F. */

  putreg32(0x00010000ul, PFIC_WAKEIP_V5F);
  boot_modifyreg32(PFIC_SCTLR, 0, 1ul << 5);
  boot_delay(2000);

  /* Boot-alive indicator: PB1 blinks forever. */

  for (;;)
    {
      putreg32(BOOT_LED_SET, BOOT_LED_BSHR);
      boot_delay(500000);
      putreg32(BOOT_LED_CLR, BOOT_LED_BSHR);
      boot_delay(500000);
    }
}
