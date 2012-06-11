#include "stm32f10x_type.h"
#include "cortexm3_macro.h"
#include "common.h"
#include "hardware.h"
#include "xsscu.h"

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


static void mdelay(int n)
{
        int i = n*100;                                           
        while (i-- > 0) {
                asm("nop");                                                                                  
        }
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
}

#define AFIO_MAPR (u32) (0x40000000 + 0x10000 + 0x4)



/*
JRST CCLK PB4
JTDO DIN PB3
JTCK PROGB PA14

JTDI INITB PA15
JTMS DONE PA13
*/

#define sr_func(func,g,p)				\
	static void func(char m) {			\
		m ? setPin(g,p) : resetPin(g,p);	\
	}						

sr_func(clk_ctl,GPIOB,4);
sr_func(progb_ctl,GPIOA,14);
sr_func(sout_ctl,GPIOB,3);

static char get_done()
{
        //return (char) GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)
	return readPin(GPIOA,1);
}

static char get_initb()
{
        //return (char) GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)
	return readPin(GPIOA,1);
}


const struct xsscu_unit fpga0 = {
.name = "Xilinx XS3S100E",
.clk_ctl = clk_ctl,
.sout_ctl = sout_ctl,
.progb_ctl = progb_ctl,
.get_initb = get_initb,
.get_done = get_done,
.delay = mdelay,
.delay_pb = 1200,
.delay_clk = 0
};


inline void setupXSSCU()
{
	/* We need to remap some stuff */
	/* Enable AFIO clock */
	 u32 rwmVal  = GET_REG(RCC_APB2ENR);
	 rwmVal |= 0x000000D;
	 /* Remap stuff */
	 SET_REG(RCC_APB2ENR,rwmVal);
	 rwmVal = GET_REG(AFIO_MAPR);
	 rwmVal&=0xF8FFFFFF;
	 rwmVal|=0x4000000;
	 SET_REG(AFIO_MAPR,rwmVal);
	 /* Now, let's setup GPIOS */
	 /* GPIOB */
	 rwmVal  = GET_REG(GPIO_CRL(GPIOB));
	 rwmVal&=0xFFF00FFF;
	 rwmVal|=0x00011000;
	 SET_REG(GPIO_CRL(GPIOB),rwmVal);
	 /* Now, let's setup GPIOS */
	 /* GPIOA */
	 rwmVal  = GET_REG(GPIO_CRH(GPIOA));
	 rwmVal&=0xF0FFFFFF;
	 rwmVal|=0x01000000;
	 SET_REG(GPIO_CRH(GPIOA),rwmVal);
}

/* executed before actual jump to user code */
void boardTeardown()
{

}

/* This is a common routine to setup the board */
void boardInit()
{
	setupCLK();
	setupXSSCU();
	setupLED();
	setupUSB();
}
