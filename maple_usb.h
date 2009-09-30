#ifndef __MAPLE_USB_H
#define __MAPLE_USB_H

#include "maple_lib.h"
#include "maple_usb_desc.h"

/* USB configuration params */
#define BTABLE_ADDRESS  0x00
#define ENDP0_RXADDR    0x10
#define ENDP0_TXADDR    0x50    /* gives 64 bytes i/o buflen */
#define bMaxPacketSize  0x40    /* 64B,  maximum for usb FS devices */
#define wTransferSize   0x0400  /* 1024B, want: maxpacket < wtransfer < 10KB (to ensure everything can live in ram */
#define NUM_ENDPTS      0x01

#define ISR_MSK ( CNTR_CTRM  | \ /* defines which interrupts are handled */
                 CNTR_WKUPM  | \
                 CNTR_SUSPM  | \
                 CNTR_ERRM   | \
                 CNTR_SOFM   | \
                 CNTR_ESOFM  | \
                 CNTR_RESETM )


/* any structs or enums */
typedef enum _RESUME_STATE
 {
   RESUME_EXTERNAL,
   RESUME_INTERNAL,
   RESUME_LATER,
   RESUME_WAIT,
   RESUME_START,
   RESUME_ON,
   RESUME_OFF,
   RESUME_ESOF,
 } RESUME_STATE;

typedef enum _DEVICE_STATE
  {
    UNCONNECTED,
    ATTACHED,
    POWERED,
    SUSPENDED,
    ADDRESSED,
    CONFIGURED
  } DEVICE_STATE;

/* public exported funcs */
void usbAppInit(void); /* singleton usb initializer */

/* internal usb HW layer power management */
void usbSuspend(void);
void usbResumeInit(void);
void usbResume(RESUME_STATE);
RESULT usbPowerOn(void);
RESULT usbPowerOff(void);

/* internal functions (as per the usb_core pProperty structure) */
void usbInit(void);
void usbReset(void);
void usbStatusIn(void);
void usbStatusOut(void);
RESULT usbDataSetup(u8);
RESULT usbNoDataSetup(u8);
u8* usbGetInterfaceSetting(u8,u8);
u8* usbGetDeviceDescriptor(u16);
u8* usbGetConfigDescriptor(u16);
u8* usbGetStringDescriptor(u16);

/* internal callbacks to respond to standard requests */
void usbGetConfiguration(void);
void usbSetConfiguration(void);
void usbGetInterface(void);
void usbSetInterface(void);
void usbGetStatus(void);
void usbClearFeature(void);
void usbSetEndpointFeature(void);
void usbSetDeviceFeature(void);
void usbSetDeviceAddress(void);

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
