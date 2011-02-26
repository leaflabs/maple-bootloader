/* *****************************************************************************
 * The MIT License
 *
 * Copyright (c) 2010 LeafLabs LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * ****************************************************************************/

/**
 *  @file config.h
 *
 *  @brief bootloader settings and macro defines
 *
 *
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include "common.h"

/* High density devices have 2KB flash pages */
#define FLASH_PAGE_SIZE  0x800

/* On the Maple RET6, LED is PA5 */
#define LED_BANK         GPIOA
#define LED              5
#define LED_BANK_CR      GPIO_CRL(LED_BANK)
#define LED_CR_MASK      0xFF0FFFFF
#define LED_CR_OUTPUT    0x00100000
#define RCC_APB2ENR_LED  0x00000004 /* enable PA */

/* On the Maple RET6, BUT is PC9 */
#define BUTTON_BANK      GPIOC
#define BUTTON           9
#define BUT_BANK_CR      GPIO_CRH(BUTTON_BANK)
#define BUT_CR_MASK      0xFFFFFF0F
#define BUT_CR_OUTPUT    0x00000040
#define RCC_APB2ENR_BUT  0x00000010 /* enable PC */

/* USB Disc Pin Setup.  On the Maple RET6, USB_DISC is PC12 */
#define USB_DISC_BANK         GPIOC
#define USB_DISC              12
#define USB_DISC_CR           GPIO_CRH(USB_DISC_BANK)
#define USB_DISC_CR_MASK      0xFFF0FFFF
#define USB_DISC_CR_OUTPUT_OD 0x00050000
#define RCC_APB2ENR_USB       0x00000010

/* Controls for strobing the LED pin during bootloader startup */
#define BLINK_FAST       0x50000
#define BLINK_SLOW       0x100000
#define STARTUP_BLINKS   5
#define BOOTLOADER_WAIT  6

/* Addresses to place user code, in RAM and in Flash builds. */
#define USER_CODE_RAM    0x20000C00
#define USER_CODE_FLASH  0x08005000

#define VEND_ID0         0xAF
#define VEND_ID1         0x1E
#define PROD_ID0         0x03
#define PROD_ID1         0x00

#endif
