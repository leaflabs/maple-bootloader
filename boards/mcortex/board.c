#include "stm32f10x_type.h"
#include "cortexm3_macro.h"
#include "common.h"
#include "hardware.h"


void setupLED (void) {
  // todo, swap out hardcoded pin/bank with macro
  u32 rwmVal; /* read-write-modify place holder var */

  /* Setup APB2 (GPIOG) */
  rwmVal =  GET_REG(RCC_APB2ENR);
  rwmVal |= 0x00000100;
  SET_REG(RCC_APB2ENR,rwmVal);

  /* Setup GPIOA Pin 5 as PP Out */
//   SET_REG(GPIO_CRL(GPIOG), 0x00100000);

//   rwmVal =  GET_REG(GPIO_CRL(GPIOG));
//   rwmVal &= 0xFF0FFFFF;
//   rwmVal |= 0x00100000;
//   SET_REG(GPIO_CRL(GPIOG),rwmVal);

//   setPin(GPIOA,5);
}


/* Motor Cortex uses 12Mhz quartz */
void setupCLK (void) {
  /* enable HSE */
  SET_REG(RCC_CR,GET_REG(RCC_CR) | 0x00010001);
  while ((GET_REG(RCC_CR) & 0x00020000) == 0); /* for it to come on */

  /* enable flash prefetch buffer */
  SET_REG(FLASH_ACR, 0x00000012);

  /* Configure PLL */
  SET_REG(RCC_CFGR,GET_REG(RCC_CFGR) | 0x00110400);  /* pll=72Mhz,APB1=36Mhz,AHB=72Mhz */
  SET_REG(RCC_CR,GET_REG(RCC_CR)     | 0x01000000);  /* enable the pll */
  while ((GET_REG(RCC_CR) & 0x03000000) == 0);         /* wait for it to come on */

  /* Set SYSCLK as PLL */
  SET_REG(RCC_CFGR,GET_REG(RCC_CFGR) | 0x00000002);
  while ((GET_REG(RCC_CFGR) & 0x00000008) == 0); /* wait for it to come on */
}

void setupUSB (void) {
  u32 rwmVal; /* read-write-modify place holder var */

  /* Setup the USB DISC Pin */
  rwmVal  = GET_REG(RCC_APB2ENR);
  rwmVal |= 0x00000010;
  SET_REG(RCC_APB2ENR,rwmVal);

  // todo, macroize usb_disc pin
  /* Setup GPIOC Pin 12 as OD out */
  rwmVal  = GET_REG(GPIO_CRH(GPIOA));
  rwmVal &= 0xFFF0FFFF;
  rwmVal |= 0x00050000;
  setPin (GPIOA,12);
  SET_REG(GPIO_CRH(GPIOA),rwmVal);
  pRCC->APB1ENR |= 0x00800000;
  /* initialize the usb application */
  resetPin (GPIOA,12);  /* present ourselves to the host */
  u32 c = 120000000;
  while (c--);
}


/* executed before actual jump to user code */
void boardTeardown()
{
	//resetPin(GPIOB,5); // disconnect usb from host. todo, macroize pin
}

/* This is a common routine to setup the board */
void boardInit()
{
	setupCLK();
	setupLED();
	setupUSB();
}
