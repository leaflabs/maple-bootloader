#include "stm32f10x_type.h"
#include "cortexm3_macro.h"
#include "common.h"
#include "hardware.h"


static inline void setupLED (void) {
  // todo, swap out hardcoded pin/bank with macro
  u32 rwmVal; /* read-write-modify place holder var */

  /* Setup APB2 (GPIOA) */
  rwmVal =  GET_REG(RCC_APB2ENR);
  rwmVal |= 0x00000004;
  SET_REG(RCC_APB2ENR,rwmVal);

  /* Setup GPIOA Pin 5 as PP Out */
  SET_REG(GPIO_CRL(GPIOA), 0x00100000);

  rwmVal =  GET_REG(GPIO_CRL(GPIOA));
  rwmVal &= 0xFF0FFFFF;
  rwmVal |= 0x00100000;
  SET_REG(GPIO_CRL(GPIOA),rwmVal);

  setPin(GPIOA,5);
}

static inline void setupCLK (void) {
  /* enable HSE */
  SET_REG(RCC_CR,GET_REG(RCC_CR) | 0x00010001);
  while ((GET_REG(RCC_CR) & 0x00020000) == 0); /* for it to come on */

  /* enable flash prefetch buffer */
  SET_REG(FLASH_ACR, 0x00000012);

  /* Configure PLL */
  SET_REG(RCC_CFGR,GET_REG(RCC_CFGR) | 0x001D0400);  /* pll=72Mhz,APB1=36Mhz,AHB=72Mhz */
  SET_REG(RCC_CR,GET_REG(RCC_CR)     | 0x01000000);  /* enable the pll */
  while ((GET_REG(RCC_CR) & 0x03000000) == 0);         /* wait for it to come on */

  /* Set SYSCLK as PLL */
  SET_REG(RCC_CFGR,GET_REG(RCC_CFGR) | 0x00000002);
  while ((GET_REG(RCC_CFGR) & 0x00000008) == 0); /* wait for it to come on */
}

static inline void setupBUTTON (void) {
  // todo, swap out hardcoded pin/bank with macro
  u32 rwmVal; /* read-write-modify place holder var */

  /* Setup APB2 (GPIOC) */
  rwmVal =  GET_REG(RCC_APB2ENR);
  rwmVal |= 0x00000010;
  SET_REG(RCC_APB2ENR,rwmVal);

  /* Setup GPIOC Pin 9 as PP Out */
  rwmVal =  GET_REG(GPIO_CRH(GPIOC));
  rwmVal &= 0xFFFFFF0F;
  rwmVal |= 0x00000040;
  SET_REG(GPIO_CRH(GPIOC),rwmVal);

}


static void inline setupUSB (void) {
  u32 rwmVal; /* read-write-modify place holder var */

  /* Setup the USB DISC Pin */
  rwmVal  = GET_REG(RCC_APB2ENR);
  rwmVal |= 0x00000010;
  SET_REG(RCC_APB2ENR,rwmVal);

  // todo, macroize usb_disc pin
  /* Setup GPIOC Pin 12 as OD out */
  rwmVal  = GET_REG(GPIO_CRH(GPIOC));
  rwmVal &= 0xFFF0FFFF;
  rwmVal |= 0x00050000;
  setPin (GPIOC,12);
  SET_REG(GPIO_CRH(GPIOC),rwmVal);

  pRCC->APB1ENR |= 0x00800000;

  /* initialize the usb application */
  resetPin (GPIOC,12);  /* present ourselves to the host */

}


/* executed before actual jump to user code */
void boardTeardown()
{
  setPin(GPIOC,12); // disconnect usb from host. todo, macroize pin
}

/* This is a common routine to setup the board */
void boardInit()
{
	setupCLK();
	setupLED();
	setupBUTTON();
	setupUSB();
}
