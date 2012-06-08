#include "stm32f10x_type.h"
#include "cortexm3_macro.h"
#include "common.h"
#include "hardware.h"


void setupLED (void) {
	/* enable LED pin */
	pRCC->APB2ENR |= RCC_APB2ENR_LED;

	/* Setup LED pin as output open drain */
	SET_REG(LED_BANK_CR,(GET_REG(LED_BANK_CR) & LED_CR_MASK) | LED_CR_OUTPUT);
	setPin(LED_BANK, LED);
}

static void setupCLK (void) {
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

void setupBUTTON (void) {
	/* enable button pin */
	pRCC->APB2ENR |= RCC_APB2ENR_BUT;

	/* Setup button pin as floating input */
	SET_REG(BUT_BANK_CR,(GET_REG(BUT_BANK_CR) & BUT_CR_MASK) | BUT_CR_OUTPUT);
	setPin(BUTTON_BANK, BUTTON);
}

void setupUSB (void) {
	/* enable USB DISC Pin */
	pRCC->APB2ENR |= RCC_APB2ENR_USB;

	/* Setup USB DISC pin as output open drain */
	SET_REG(USB_DISC_CR,
		(GET_REG(USB_DISC_CR) & USB_DISC_CR_MASK) | USB_DISC_CR_OUTPUT_OD);
	setPin(USB_DISC_BANK, USB_DISC);

	/* turn on the USB clock */
	pRCC->APB1ENR |= 0x00800000;

	/* initialize the USB application */
	resetPin(USB_DISC_BANK, USB_DISC); /* present ourselves to the host */

}


/* executed before actual jump to user code */
void boardTeardown()
{
	setPin(USB_DISC_BANK, USB_DISC); /* unpresent ourselves to the host */
}

/* This is a common routine to setup the board */
void boardInit()
{
	setupCLK();
	setupLED();
	setupBUTTON();
	setupUSB();
}
