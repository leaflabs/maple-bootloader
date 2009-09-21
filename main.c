#include "stm32f10x_type.h"

#define RCC   0x40021000
#define FLASH 0x40022000
#define GPIOA 0x40010800
#define CPIOC 0x40011000

#define RCC_CR      RCC
#define RCC_CFGR    RCC + 0x04
#define RCC_CIR     RCC + 0x08
#define RCC_AHBENR  RCC + 0x14
#define RCC_APB2ENR RCC + 0x18
#define RCC_APB1ENR RCC + 0x16

#define FLASH_KEYR FLASH + 0x04
#define FLASH_ACR  FLASH + 0x00

#define GPIO_CRL(port)  port
#define GPIO_CRH(port)  port+0x04
#define GPIO_ODR(port)  port+0x0c
#define GPIO_BSRR(port) port+0x10

/* todo, wrap in do whiles for the natural ; */
#define SET_REG(addr,val) *(vu32*)(addr)=val
#define GET_REG(addr)     *(vu32*)(addr)

void systemReset(void) {
  SET_REG(RCC_CR, GET_REG(RCC_CR)     | 0x00000001);
  SET_REG(RCC_CFGR, GET_REG(RCC_CFGR) & 0xF8FF0000);
  SET_REG(RCC_CR, GET_REG(RCC_CR)     & 0xFEF6FFFF);
  SET_REG(RCC_CR, GET_REG(RCC_CR)     & 0xFFFBFFFF);
  SET_REG(RCC_CFGR, GET_REG(RCC_CFGR) & 0xFF80FFFF);

  SET_REG(RCC_CIR, 0x00000000);  /* disable all interrupts */
}

int main (void) {
  systemReset();

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

  /* Setup APB2 (GPIOA) */
  SET_REG(RCC_APB2ENR,0x00000004);

  /* Setup GPIOA Pin 5 as PP Out */
  SET_REG(GPIO_CRL(GPIOA), 0x00100000);
  SET_REG(GPIO_BSRR(GPIOA),0x00000020);

  /* all done ! */
  
  u32 c = 0;
  while (1) {
    for (c=0;c<1000000;c++){}
    SET_REG(GPIO_BSRR(GPIOA),0x00200000);
    for (c=0;c<1000000;c++){}
    SET_REG(GPIO_BSRR(GPIOA),0x00000020);
  }
}
