/****************************************************************************
 * boards/risc-v/ch32h41x/ch32h41x-evb/boot/boot_main.c
 *
 * V3F boot loader - staged observable bring-up:
 *
 *   Stage 1: PB1 blinks + 'A' heartbeat at the reset clock (25MHz HSI).
 *   Stage 2: 400MHz SYSPLL sequence, then USART1 re-init at 100MHz,
 *            banner + blinks continue at HCLK=100MHz.
 *   Stage 3: PFIC.WAKEIP[1] = 0x00010000 (+ SCTLR.SYSRET when
 *            BOOT_RELEASE_V5F) hands the V5F NuttX image at flash
 *            0x08010000 its entry; the V3F keeps blinking PB1.
 *
 * Every stage is visible on the LED and the serial, so a stuck boot
 * shows exactly which stage never completed.
 ****************************************************************************/

#include <stdint.h>

#include "hardware/ch32h41x_memorymap.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_usart.h"

/* PFIC absolute addresses - WAKEIP/SCTLR offsets per the EVT PFIC_Type
 * struct, calibrated against the chip (WAKEIP[0] = 0xE000E720). */

#define PFIC_WAKEIP_V5F   0xE000E724ul
#define PFIC_SCTLR        0xE000ED10ul

/* Boot-alive LED = PB1 (the EVT EVB heartbeat pin; PA2 stays free for
 * the V5F image).  BSHR is a write-only trigger register: read-back is
 * 0 and writing 0 is a no-op, so toggling must be direct single writes
 * (bits 0-15 set the pin, bits 16-31 reset it) - never read-modify-
 * write, which loses the reset bit and latches the pin forever. */

#define BOOT_LED_BSHR     (CH32H41X_GPIOB_BASE + 0x10)
#define BOOT_LED_SET      0x0002ul            /* PB1 high (BSHR.BS1)  */
#define BOOT_LED_CLR      0x00020000ul        /* PB1 low  (BSHR.BR1)  */

/* Release the V5F only for the combined image (flash 0x00010000 holds
 * the V5F NuttX); 0 keeps the V5F asleep during the V3F-only boot
 * validation.  Same sequence as the EVT NVIC_WakeUp_V5F: WAKEIP[1]
 * store, then SCTLR bit 5 (SYSRET) starts the fetch. */

#ifndef BOOT_RELEASE_V5F
#  define BOOT_RELEASE_V5F 1
#endif

/* USART1: TX only, polled.  PA9 = TX (AF7). */

#define USART1            CH32H41X_USART1_BASE
#define USART_STATR       (USART1 + CH32H41X_USART_STATR)
#define USART_DATAR       (USART1 + CH32H41X_USART_DATAR)
#define USART_BRR         (USART1 + CH32H41X_USART_BRR)
#define USART_CTLR1       (USART1 + CH32H41X_USART_CTLR1)

#define USART_SR_TXE      CH32H41X_USART_SR_TXEF
#define USART_CR1_UE      CH32H41X_USART_CR1_UE
#define USART_CR1_TE      CH32H41X_USART_CR1_TE

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

/* One PB1 blink + one 'A' on the wire */

static void boot_beat(void)
{
  putreg32(BOOT_LED_SET, BOOT_LED_BSHR);   /* PB1 high */
  boot_delay(250000);
  putreg32(BOOT_LED_CLR, BOOT_LED_BSHR);   /* PB1 low */
  boot_delay(250000);
  boot_putc('A');
}

void boot_main(void)
{
  unsigned i;

  /* Stage 1: PB1 config + heartbeat at the reset clock.  From the
   * first blink onward the loader is provably alive. */

  boot_modifyreg32(CH32H41X_RCC_HB2PCENR, 0,
                   CH32H41X_RCC_HB2_GPIOA | CH32H41X_RCC_HB2_GPIOB);
  boot_modifyreg32(CH32H41X_GPIOB_BASE + 0x00, 0xf0ul, 0x30ul);
  boot_uart_init(25000000ul);
  boot_puts("V3F boot (HSI)\n");

  for (i = 0; i < 15; i++)
    {
      boot_beat();
    }

  /* Stage 2: 400MHz SYSPLL + USART1 re-init at HCLK=100MHz */

  boot_setclock_400m();
  boot_uart_init(boot_hclk_frequency());
  boot_puts("V3F: 100MHz\n");

  for (i = 0; i < 10; i++)
    {
      boot_beat();
    }

  /* Stage 3: wake the V5F NuttX image at flash 0x08010000 (the V5F
   * fetches it through the zero-wait window at 0x00010000).  The V3F
   * keeps blinking forever as the boot-alive indicator. */

  putreg32(0x00010000ul, PFIC_WAKEIP_V5F);   /* V5F entry -> WAKEIP[1] */

#if BOOT_RELEASE_V5F
  boot_modifyreg32(PFIC_SCTLR, 0, 1ul << 5); /* SYSRET: V5F fetches it */
  boot_delay(2000);                     /* ~100us, as the DFU gives the
                                           V5F after NVIC_WakeUp_V5F */
  boot_puts("V3F: V5F woken\n");
#else
  boot_puts("V3F idle\n");
#endif

  /* The V5F NuttX owns USART1 from here: the V3F stops printing
   * and only blinks PB1 as the boot-alive indicator. */

  for (;;)
    {
      putreg32(BOOT_LED_CLR, BOOT_LED_BSHR);
      boot_delay(500000);
      putreg32(BOOT_LED_SET, BOOT_LED_BSHR);
      boot_delay(500000);
    }
}
