#include "maple_lib.h"

void systemReset(void) {
  SET_REG(RCC_CR, GET_REG(RCC_CR)     | 0x00000001);
  SET_REG(RCC_CFGR, GET_REG(RCC_CFGR) & 0xF8FF0000);
  SET_REG(RCC_CR, GET_REG(RCC_CR)     & 0xFEF6FFFF);
  SET_REG(RCC_CR, GET_REG(RCC_CR)     & 0xFFFBFFFF);
  SET_REG(RCC_CFGR, GET_REG(RCC_CFGR) & 0xFF80FFFF);

  SET_REG(RCC_CIR, 0x00000000);  /* disable all interrupts */
}

void setupCLK (void) {
  /* enable HSE */
  SET_REG(RCC_CR,GET_REG(RCC_CR) | 0x00010001);
  while (GET_REG(RCC_CR) & 0x00020000 == 0); /* for it to come on */
  
  /* enable flash prefetch buffer */
  SET_REG(FLASH_ACR, 0x00000012);

  /* unlock the flash */
  SET_REG(FLASH_KEYR,0x45670123);
  SET_REG(FLASH_KEYR,0xCDEF89AB);

  /* Configure PLL */
  SET_REG(RCC_CFGR,GET_REG(RCC_CFGR) | 0x001D0400);  /* pll=72Mhz,APB1=36Mhz,AHB=72Mhz */
  SET_REG(RCC_CR,GET_REG(RCC_CR)     | 0x01000000);  /* enable the pll */
  while (GET_REG(RCC_CR) & 0x03000000 == 0);         /* wait for it to come on */
  
  /* Set SYSCLK as PLL */
  SET_REG(RCC_CFGR,GET_REG(RCC_CFGR) | 0x00000002);
  while (GET_REG(RCC_CFGR) & 0x00000008 == 0); /* wait for it to come on */
}

void setupLED (void) {
  /* Setup APB2 (GPIOA) */
  SET_REG(RCC_APB2ENR,0x00000004);

  /* Setup GPIOA Pin 5 as PP Out */
  SET_REG(GPIO_CRL(GPIOA), 0x00100000);
  SET_REG(GPIO_BSRR(GPIOA),0x00000020);
}
