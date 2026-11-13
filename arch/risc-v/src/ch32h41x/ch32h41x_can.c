/****************************************************************************
 * arch/risc-v/src/ch32h41x/ch32h41x_can.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

/* CAN1 lower-half driver for /dev/can0 (bxCAN-style controller).
 *
 * Bit timing for a 100MHz HCLK: prescaler 20, (1 + BS1(6) + BS2(3)) = 10
 * tq per bit -> 500kbit/s, sample point 70%.
 *
 * Mode: the driver comes up in LOOPBACK mode so it can be exercised
 * without a transceiver or any wiring (send -> own RX FIFO).  It can be
 * switched to normal mode at runtime via CANIOC_SET_CONNMODES.
 *
 * Vendor specifics cross-checked against the EVT reference:
 *  - The BTIMR prescaler field is 10 bits wide (BRP mask 0x3FF), wider
 *    than the STM32 bxCAN 6-bit field (EVT CAN_BTIMR_BRP).
 *  - BTIMR[31:30] holds the raw CAN_Mode value (Normal/LoopBack/Silent/
 *    Silent+LoopBack); the EVT CAN_Init() writes it as Mode << 30.
 *  - The filter banks are centralized in the CAN1 block (FCTLR/FMCFGR/
 *    FSCFGR/FAFIFOR/FWR at CAN1_BASE+0x200.., EVT CAN_FilterInit uses
 *    CAN1-> registers for every controller).  The EVT notes that the
 *    version-A silicon does not implement the hardware filters, so the
 *    driver configures a single accept-all bank and relies on the
 *    IP delivering everything to FIFO0.
 *  - CAN1 sits on the HB1 bus (EVT RCC_HB1Periph_CAN1, HBPCENR bit 25).
 *
 * TX completion is polled inside co_send (bounded); RX uses the
 * CAN1_RX0 interrupt to drain FIFO0 into the upper half.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <nuttx/arch.h>
#include <nuttx/can.h>
#include <nuttx/can/can.h>
#include <nuttx/kmalloc.h>
#include <nuttx/irq.h>

#include "riscv_internal.h"
#include "hardware/ch32h41x_memorymap.h"
#include "hardware/ch32h41x_rcc.h"
#include "hardware/ch32h41x_can.h"

#include "ch32h41x_internal.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CAN1_BASE  CH32H41X_CAN1_BASE

/* NuttX IRQ numbers (hardware vector + RISCV_IRQ_ASYNC(16)) */

#define CAN1_RX0_IRQ  (CH32H41X_IRQ_FIRST + 60)

/* Bounded waits expressed in poll iterations */

#define CAN_SPINLIMIT 2000000ul

/* Bit timing: 500kbit/s at 100MHz HCLK (see file header) */

#define CAN_BRP       20ul      /* prescaler            */
#define CAN_BS1_TQ    6ul       /* phase segment 1, tq  */
#define CAN_BS2_TQ    3ul       /* phase segment 2, tq  */
#define CAN_SJW_TQ    1ul       /* resync width, tq     */

#define CAN_MODE_LOOPBACK  1ul  /* EVT CAN_Mode_LoopBack (BTR bits 31:30) */
#define CAN_MODE_NORMAL    0ul

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ch32h41x_can_s
{
  FAR struct can_dev_s *dev;      /* upper-half device */
  bool loopback;                  /* current mode */
  bool ready;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct ch32h41x_can_s g_candev;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_can_initmode
 *
 * Description:
 *   Enter/leave bxCAN initialization mode (INRQ + INAK handshake,
 *   bounded).  Returns true when the requested state was reached.
 *
 ****************************************************************************/

static bool ch32h41x_can_initmode(uintptr_t base, bool enter)
{
  uint32_t timeout = CAN_SPINLIMIT;

  if (enter)
    {
      /* Wake the core first: a CAN core left in SLEEP mode ignores the
       * initialization request and INAK never asserts (EVT CAN_Init
       * clears CTLR.SLEEP immediately before setting INRQ) */

      modifyreg32(CH32H41X_CAN_CTLR(base), CH32H41X_CAN_CR_SLEEP, 0);
      modifyreg32(CH32H41X_CAN_CTLR(base), 0, CH32H41X_CAN_CR_INRQ);
      while ((getreg32(CH32H41X_CAN_STATR(base)) & CH32H41X_CAN_ST_INAK) == 0)
        {
          if (--timeout == 0)
            {
              return false;
            }
        }
    }
  else
    {
      modifyreg32(CH32H41X_CAN_CTLR(base), CH32H41X_CAN_CR_INRQ, 0);
      while ((getreg32(CH32H41X_CAN_STATR(base)) & CH32H41X_CAN_ST_INAK) != 0)
        {
          if (--timeout == 0)
            {
              return false;
            }
        }
    }

  return true;
}

/****************************************************************************
 * Name: ch32h41x_can_isr
 *
 * Description:
 *   FIFO0 drain: forward every pending frame to the upper half and
 *   release the mailbox.  Also clears stale overrun/full flags.
 *
 ****************************************************************************/

static int ch32h41x_can_isr(int irq, FAR void *context, FAR void *arg)
{
  uintptr_t base = CAN1_BASE;
  uint32_t rf = getreg32(CH32H41X_CAN_RFIFO0(base));

  /* Clear stale FIFO error flags first so they cannot wedge the FIFO */

  if ((rf & (CH32H41X_CAN_RF_FOVR | CH32H41X_CAN_RF_FULL)) != 0)
    {
      putreg32(rf | CH32H41X_CAN_RF_FOVR | CH32H41X_CAN_RF_FULL,
               CH32H41X_CAN_RFIFO0(base));
    }

  while ((getreg32(CH32H41X_CAN_RFIFO0(base)) & CH32H41X_CAN_RF_FMP_MASK) != 0)
    {
      struct can_hdr_s hdr;
      uint8_t data[8];
      uint32_t mir = getreg32(CH32H41X_CAN_RXMIR(base, 0));
      uint32_t dtr = getreg32(CH32H41X_CAN_RXMDTR(base, 0));
      unsigned dlc = (dtr & CH32H41X_CAN_DT_DLC_MASK) >>
                     CH32H41X_CAN_DT_DLC_SHIFT;
      unsigned i;

      if (dlc > 8)
        {
          dlc = 8;
        }

      memset(&hdr, 0, sizeof(hdr));

      if ((mir & CH32H41X_CAN_TI_IDE) != 0)
        {
          hdr.ch_extid = 1;
          hdr.ch_id = (mir & CH32H41X_CAN_TI_EXID_MASK) >>
                      CH32H41X_CAN_TI_EXID_SHIFT;
        }
      else
        {
          hdr.ch_extid = 0;
          hdr.ch_id = (mir & CH32H41X_CAN_TI_STID_MASK) >>
                      CH32H41X_CAN_TI_STID_SHIFT;
        }

      hdr.ch_rtr = (mir & CH32H41X_CAN_TI_RTR) != 0;
      hdr.ch_dlc = dlc;

      for (i = 0; i < dlc; i++)
        {
          uint32_t word = (i < 4)
            ? getreg32(CH32H41X_CAN_RXMDLR(base, 0))
            : getreg32(CH32H41X_CAN_RXMDHR(base, 0));

          data[i] = (uint8_t)((word >> ((i & 3) * 8)) & 0xfful);
        }

      /* Release the mailbox BEFORE delivering so the ISR cannot wedge
       * on a full FIFO if the upper half re-enters */

      modifyreg32(CH32H41X_CAN_RFIFO0(base), 0, CH32H41X_CAN_RF_RFOM);

      can_receive(g_candev.dev, &hdr, data);
    }

  return OK;
}

/****************************************************************************
 * Name: adc lower-half callbacks
 ****************************************************************************/

static void ch32h41x_can_reset(FAR struct can_dev_s *dev)
{
  g_candev.ready = false;
}

static int ch32h41x_can_setup(FAR struct can_dev_s *dev)
{
  uintptr_t base = CAN1_BASE;
  uint32_t mode;

  g_candev.dev = dev;

  /* Bus clock + one-time peripheral reset (HB1 bus, EVT RCC_HB1Periph_CAN1) */

  modifyreg32(CH32H41X_RCC_HB1PCENR, 0, CH32H41X_RCC_HB1_CAN1);
  modifyreg32(CH32H41X_RCC_HB1PRSTR, 0, CH32H41X_RCC_HB1R_CAN1);
  modifyreg32(CH32H41X_RCC_HB1PRSTR, CH32H41X_RCC_HB1R_CAN1, 0);

  /* Enter init mode */

  if (!ch32h41x_can_initmode(base, true))
    {
      return -EIO;
    }

  /* Operation modes: no auto wakeup/bus-off quirks, retransmission on
   * (NART = 0), transmit FIFO priority by identifier (TXFP = 0) */

  modifyreg32(CH32H41X_CAN_CTLR(base),
              CH32H41X_CAN_CR_TTCM | CH32H41X_CAN_CR_ABOM |
              CH32H41X_CAN_CR_AWUM | CH32H41X_CAN_CR_NART |
              CH32H41X_CAN_CR_RFLM | CH32H41X_CAN_CR_TXFP |
              CH32H41X_CAN_CR_SLEEP,
              0);

  /* Bit timing + mode.  Default: loopback (no transceiver wiring
   * needed for bring-up; switch at runtime via CANIOC_SET_CONNMODES).
   * BTR = mode[31:30] | SJW[27:24] | BS2[23:20] | BS1[19:16] | BRP[9:0],
   * field values are tq-1 (EVT CAN_Init). */

  mode = g_candev.loopback ? CAN_MODE_LOOPBACK : CAN_MODE_NORMAL;

  putreg32((mode << 30) |
           ((CAN_SJW_TQ - 1) << 24) |
           ((CAN_BS2_TQ - 1) << 20) |
           ((CAN_BS1_TQ - 1) << 16) |
           (CAN_BRP - 1),
           CH32H41X_CAN_BTIMR(base));

  /* One accept-all 32-bit mask filter into FIFO0 (bank 0).  The filter
   * banks live in the CAN1 block and must be edited in filter-init
   * mode (FCTLR.FINIT).  Field encodings per EVT CAN_FilterInit:
   * mask mode = FMCFGR bit CLEAR (set bit = list mode), 32-bit scale =
   * FSCFGR bit SET, FIFO0 = FAFIFOR bit CLEAR, active = FWR bit SET. */

  modifyreg32(CH32H41X_CAN_FCTLR(base), 0, CH32H41X_CAN_FM_FINITE);
  modifyreg32(CH32H41X_CAN_FMCFGR(base), 1ul, 0);   /* bank 0: mask mode  */
  modifyreg32(CH32H41X_CAN_FSCFGR(base), 0, 1ul);   /* bank 0: 32-bit     */
  modifyreg32(CH32H41X_CAN_FAFIFOR(base), 1ul, 0);  /* bank 0 -> FIFO 0   */
  putreg32(0, CH32H41X_CAN_FR(base, 0));            /* id  = 0            */
  putreg32(0, CH32H41X_CAN_FR2(base, 0));           /* mask = 0 (all)     */
  modifyreg32(CH32H41X_CAN_FWR(base), 0, 1ul);      /* bank 0 active      */
  modifyreg32(CH32H41X_CAN_FCTLR(base), CH32H41X_CAN_FM_FINITE, 0);

  /* Leave init mode */

  if (!ch32h41x_can_initmode(base, false))
    {
      return -EIO;
    }

  /* RX0 interrupt drains FIFO0 into the upper half */

  if (irq_attach(CAN1_RX0_IRQ, ch32h41x_can_isr, NULL) != OK)
    {
      return -EIO;
    }

  modifyreg32(CH32H41X_CAN_INTENR(base), 0, CH32H41X_CAN_IR_FMPIE0);
  up_enable_irq(CAN1_RX0_IRQ);

  g_candev.ready = true;
  return OK;
}

static void ch32h41x_can_shutdown(FAR struct can_dev_s *dev)
{
  uintptr_t base = CAN1_BASE;

  up_disable_irq(CAN1_RX0_IRQ);
  irq_detach(CAN1_RX0_IRQ);
  modifyreg32(CH32H41X_CAN_INTENR(base), CH32H41X_CAN_IR_FMPIE0, 0);
  g_candev.ready = false;
}

static void ch32h41x_can_rxint(FAR struct can_dev_s *dev, bool enable)
{
  uintptr_t base = CAN1_BASE;

  if (enable)
    {
      modifyreg32(CH32H41X_CAN_INTENR(base), 0, CH32H41X_CAN_IR_FMPIE0);
    }
  else
    {
      modifyreg32(CH32H41X_CAN_INTENR(base), CH32H41X_CAN_IR_FMPIE0, 0);
    }
}

static void ch32h41x_can_txint(FAR struct can_dev_s *dev, bool enable)
{
  /* TX completion is polled inside co_send */
}

static int ch32h41x_can_ioctl(FAR struct can_dev_s *dev, int cmd,
                              unsigned long arg)
{
  uintptr_t base = CAN1_BASE;
  FAR struct canioc_connmodes_s *conn;

  switch (cmd)
    {
      case CANIOC_SET_CONNMODES:
        {
          conn = (FAR struct canioc_connmodes_s *)((uintptr_t)arg);

          if (conn->bm_loopback == g_candev.loopback)
            {
              return OK;
            }

          if (!ch32h41x_can_initmode(base, true))
            {
              return -EIO;
            }

          if (conn->bm_loopback)
            {
              modifyreg32(CH32H41X_CAN_BTIMR(base), 0, CAN_MODE_LOOPBACK << 30);
            }
          else
            {
              modifyreg32(CH32H41X_CAN_BTIMR(base), CAN_MODE_LOOPBACK << 30, 0);
            }

          if (!ch32h41x_can_initmode(base, false))
            {
              return -EIO;
            }

          g_candev.loopback = conn->bm_loopback;
          return OK;
        }

      case CANIOC_GET_CONNMODES:
        {
          conn = (FAR struct canioc_connmodes_s *)((uintptr_t)arg);
          conn->bm_loopback = g_candev.loopback;
          conn->bm_silent = 0;
          return OK;
        }

      default:
        {
          return -ENOTTY;
        }
    }
}

static int ch32h41x_can_remoterequest(FAR struct can_dev_s *dev,
                                      uint16_t id)
{
  return -ENOTTY;
}

static int ch32h41x_can_send(FAR struct can_dev_s *dev,
                             FAR struct can_msg_s *msg)
{
  uintptr_t base = CAN1_BASE;
  uint32_t tstatr;
  uint32_t mir;
  uint32_t timeout = CAN_SPINLIMIT;
  unsigned mbox;
  unsigned n;

  if (!g_candev.ready)
    {
      return -EAGAIN;
    }

  /* Claim a free mailbox */

  tstatr = getreg32(CH32H41X_CAN_TSTATR(base));

  if ((tstatr & CH32H41X_CAN_TS_TME0) != 0)
    {
      mbox = 0;
    }
  else if ((tstatr & CH32H41X_CAN_TS_TME1) != 0)
    {
      mbox = 1;
    }
  else if ((tstatr & CH32H41X_CAN_TS_TME2) != 0)
    {
      mbox = 2;
    }
  else
    {
      return -EBUSY;
    }

  /* Fill the mailbox: identifier, control, DLC and data */

  if (msg->cm_hdr.ch_extid)
    {
      mir = ((msg->cm_hdr.ch_id << CH32H41X_CAN_TI_EXID_SHIFT) &
             CH32H41X_CAN_TI_EXID_MASK) | CH32H41X_CAN_TI_IDE;
    }
  else
    {
      mir = (msg->cm_hdr.ch_id << CH32H41X_CAN_TI_STID_SHIFT) &
            CH32H41X_CAN_TI_STID_MASK;
    }

  if (msg->cm_hdr.ch_rtr)
    {
      mir |= CH32H41X_CAN_TI_RTR;
    }

  putreg32(mir, CH32H41X_CAN_TXMIR(base, mbox));

  putreg32(msg->cm_hdr.ch_dlc & CH32H41X_CAN_DT_DLC_MASK,
           CH32H41X_CAN_TXMDTR(base, mbox));

  /* Merge the payload into whole 32-bit words: bytes 0..3 -> DTLR,
   * bytes 4..7 -> DTHR */

  {
    uint32_t low = getreg32(CH32H41X_CAN_TXMDLR(base, mbox));
    uint32_t high = getreg32(CH32H41X_CAN_TXMDHR(base, mbox));
    unsigned count = msg->cm_hdr.ch_dlc;

    if (count > 8)
      {
        count = 8;
      }

    for (n = 0; n < count; n++)
      {
        uint32_t *word = (n < 4) ? &low : &high;
        unsigned shift = (n & 3) * 8;

        *word &= ~(0xfful << shift);
        *word |= (uint32_t)msg->cm_data[n] << shift;
      }

    putreg32(low, CH32H41X_CAN_TXMDLR(base, mbox));
    putreg32(high, CH32H41X_CAN_TXMDHR(base, mbox));
  }

  /* Fire and wait for completion (loopback completes within a few
   * hundred bit times; the bound is generous) */

  modifyreg32(CH32H41X_CAN_TXMIR(base, mbox), 0, CH32H41X_CAN_TI_TXRQ);

  while ((getreg32(CH32H41X_CAN_TSTATR(base)) &
          (CH32H41X_CAN_TS_RQCP0 << (8 * mbox))) == 0)
    {
      if (--timeout == 0)
        {
          return -ETIMEDOUT;
        }
    }

  /* Read completion status for the mailbox group and clear RQCP */

  tstatr = getreg32(CH32H41X_CAN_TSTATR(base));
  putreg32(tstatr & (CH32H41X_CAN_TS_RQCP0 << (8 * mbox)),
           CH32H41X_CAN_TSTATR(base));

  if ((tstatr & (CH32H41X_CAN_TS_TXOK0 << (8 * mbox))) == 0)
    {
      return -EIO;
    }

  return OK;
}

static bool ch32h41x_can_txready(FAR struct can_dev_s *dev)
{
  return (getreg32(CH32H41X_CAN_TSTATR(CAN1_BASE)) &
          CH32H41X_CAN_TS_TME_MASK) != 0;
}

static bool ch32h41x_can_txempty(FAR struct can_dev_s *dev)
{
  return (getreg32(CH32H41X_CAN_TSTATR(CAN1_BASE)) &
          CH32H41X_CAN_TS_TME_MASK) == CH32H41X_CAN_TS_TME_MASK;
}

static const struct can_ops_s g_can_ops =
{
  .co_reset          = ch32h41x_can_reset,
  .co_setup          = ch32h41x_can_setup,
  .co_shutdown       = ch32h41x_can_shutdown,
  .co_rxint          = ch32h41x_can_rxint,
  .co_txint          = ch32h41x_can_txint,
  .co_ioctl          = ch32h41x_can_ioctl,
  .co_remoterequest  = ch32h41x_can_remoterequest,
  .co_send           = ch32h41x_can_send,
  .co_txready        = ch32h41x_can_txready,
  .co_txempty        = ch32h41x_can_txempty,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ch32h41x_can_initialize
 *
 * Description:
 *   Register CAN1 as /dev/can0 in loopback mode (500kbit/s).  Hardware
 *   setup runs on the first open (co_setup).
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

int ch32h41x_can_initialize(void)
{
  static FAR struct can_dev_s *g_canreg;
  FAR struct can_dev_s *dev;

  if (g_canreg != NULL)
    {
      return OK;
    }

  dev = kmm_zalloc(sizeof(struct can_dev_s));
  if (dev == NULL)
    {
      return -ENOMEM;
    }

  dev->cd_ops = &g_can_ops;

  if (can_register("/dev/can0", dev) != OK)
    {
      kmm_free(dev);
      return -EINVAL;
    }

  g_canreg = dev;
  g_candev.loopback = true;
  return OK;
}
