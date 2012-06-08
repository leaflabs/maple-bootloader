#include "stm32f10x_type.h"
#include "cortexm3_macro.h"
#include "common.h"
#include "hardware.h"


inline void setupLED (void) {
  // todo, swap out hardcoded pin/bank with macro
  u32 rwmVal; /* read-write-modify place holder var */

  /* Setup APB2 (GPIOG) */
  rwmVal =  GET_REG(RCC_APB2ENR);
  rwmVal |= 0x00000100;
  SET_REG(RCC_APB2ENR,rwmVal);

  rwmVal =  GET_REG(GPIO_CRH(GPIOG));
  rwmVal &= 0xFFFFFFF0;
  rwmVal |= 0x00000001;

  SET_REG(GPIO_CRH(GPIOG),rwmVal);

}


/* Motor Cortex uses 12Mhz quartz */
inline void setupCLK (void) {
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


/* We simulate disconnection by pulling PA12(USB) to the ground */
/* This saves us a pin */
inline void setupUSB (void) {
  u32 rwmVal; /* read-write-modify place holder var */
  rwmVal  = GET_REG(RCC_APB2ENR);
  rwmVal |= 0x0000004;
  SET_REG(RCC_APB2ENR,rwmVal);
  rwmVal  = GET_REG(GPIO_CRH(GPIOA));
  rwmVal &= 0xFFF00FFF;
  rwmVal |= 0x00011000;
  SET_REG(GPIO_CRH(GPIOA),rwmVal);
  resetPin (GPIOA,12);
  pRCC->APB1ENR |= 0x00800000;
}


/* executed before actual jump to user code */
void boardTeardown()
{

}

/* This is a common routine to setup the board */
void boardInit()
{
	setupCLK();
	setupLED();
	setupUSB();
}
