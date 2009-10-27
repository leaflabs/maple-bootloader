#ifndef __MAPLE_REGS_H
#define __MAPLE_REGS_H

/* macro'd register and peripheral definitions */
#define RCC   ((u32)0x40021000)
#define FLASH ((u32)0x40022000)
#define GPIOA ((u32)0x40010800)
#define GPIOC ((u32)0x40011000)

#define RCC_CR      RCC
#define RCC_CFGR    RCC + 0x04
#define RCC_CIR     RCC + 0x08
#define RCC_AHBENR  RCC + 0x14
#define RCC_APB2ENR RCC + 0x18
#define RCC_APB1ENR RCC + 0x16

#define FLASH_ACR     FLASH + 0x00
#define FLASH_KEYR    FLASH + 0x04
#define FLASH_OPTKEYR FLASH + 0x08
#define FLASH_SR      FLASH + 0x0C
#define FLASH_CR      FLASH + 0x10
#define FLASH_AR      FLASH + 0x14
#define FLASH_OBR     FLASH + 0x1C
#define FLASH_WRPR    FLASH + 0x20
#define FLASH_KEY1    0x45670123
#define FLASH_KEY2    0xCDEF89AB
#define FLASH_RDPRT   0x00A5

#define GPIO_CRL(port)  port
#define GPIO_CRH(port)  port+0x04
#define GPIO_ODR(port)  port+0x0c
#define GPIO_BSRR(port) port+0x10

#define SCS_BASE   ((u32)0xE000E000)
#define NVIC_BASE  (SCS_BASE + 0x0100)
#define SCB_BASE   (SCS_BASE + 0x0D00)


#define SCS   0xE000E000
#define NVIC  SCS+0x100
#define SCB   SCS+0xD00

#define TIM1_APB2_ENB ((u32)0x00000800)
#define TIM1          ((u32)0x40012C00)
#define TIM1_PSC      TIM1+0x28
#define TIM1_ARR      TIM1+0x2C
#define TIM1_RCR      TIM1+0x30
#define TIM1_CR1      TIM1+0x00
#define TIM1_CR2      TIM1+0x04
#define TIM1_DIER     TIM1+0x0C
#define TIM1_UP_IRQ_Channel ((u8)0x19)

#define USB_HP_IRQ  ((u8)0x13)
#define USB_LP_IRQ  ((u8)0x14)
#define TIM2_IRQ    ((u8)0x1C)


/* AIRCR  */
#define AIRCR_RESET         0x05FA0000
#define AIRCR_RESET_REQ     (AIRCR_RESET | (u32)0x04);

/* temporary copyage of example from kiel */
#define __VAL(__TIMCLK, __PERIOD) ((__TIMCLK/1000000UL)*__PERIOD)
#define __PSC(__TIMCLK, __PERIOD)  (((__VAL(__TIMCLK, __PERIOD)+49999UL)/50000UL) - 1)
#define __ARR(__TIMCLK, __PERIOD) ((__VAL(__TIMCLK, __PERIOD)/(__PSC(__TIMCLK, __PERIOD)+1)) - 1)

/* todo, wrap in do whiles for the natural ; */
#define SET_REG(addr,val) *(vu32*)(addr)=val
#define GET_REG(addr)     *(vu32*)(addr)


/* todo: there must be some major misunderstanding in how we access regs. The direct access approach (GET_REG)
   causes the usb init to fail upon trying to activate RCC_APB1 |= 0x00800000. However, using the struct approach
   from ST, it works fine...temporarily switching to that approach */
typedef struct 
{
  vu32 CR;
  vu32 CFGR;
  vu32 CIR;
  vu32 APB2RSTR;
  vu32 APB1RSTR;
  vu32 AHBENR;
  vu32 APB2ENR;
  vu32 APB1ENR;
  vu32 BDCR;
  vu32 CSR;
} RCC_RegStruct;
#define pRCC ((RCC_RegStruct *) RCC)
#endif
