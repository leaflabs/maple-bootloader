#ifndef __MAPLE_USB_H
#define __MAPLE_USB_H

#include "maple_lib.h"

/* USB configuration params */
#define BTABLE_ADDRESS  0x00
#define ENDP0_RXADDR    0x10
#define ENDP0_TXADDR    0x50 /* gives 64 bytes i/o buflen */

#define ISR_MSK (CNTR_CTRM  | \
                 CNTR_WKUPM | \
                 CNTR_SUSPM | \
                 CNTR_ERRM  | \
                 CNTR_SOFM  | \
                 CNTR_ESOFM | \
                 CNTR_RESETM)

/* public exported funcs */
void usbInit(void);

/* internal functions (as per the usb_core pProperty structure) */
void usbPowerUp(void);
void usbReset(void);

/* Interrupt setup/handling exposed only so that 
its obvious from main what interrupts are overloaded 
from c_only_startup.s (see the top of main.c) */
void usbSetupISR(void);
void usbEnbISTR(void)
void usbISTR(void);


/*
notes from manual:
USB Base = 0x4005C00
USB_CNTR = USB+0x40, resets to 0x03. bit 0 is rw FRES (force reset, used in DFU_DETACH)
must manually set AND clear FRES
 */

#endif
