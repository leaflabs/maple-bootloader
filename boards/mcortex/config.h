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

#define NUM_ALT_SETTINGS 3
#define STR_DESC_LEN 7

//#define CONFIG_RUNAPP_ALT 0
#define CONFIG_INFO_ALT

//No LED strobing whatsoever - saves space
//#define CONFIG_INHIBIT_STROBE

//Replace USB suspend/resume code with dummies.
//Saves a few bytes more

#define CONFIG_DISABLE_USB_SUSPEND

/* F*ck, that costed me an hour of lifetime to figure out! */
#define FLASH_PAGE_SIZE  0x800

#define LED_BANK GPIOG
#define LED      8
#define BLINK_FAST 0x50000
#define BLINK_SLOW 0x100000

#define BUTTON_BANK GPIOC
#define BUTTON      9

#define STARTUP_BLINKS 5
#define BOOTLOADER_WAIT 16

#define USER_CODE_RAM     ((u32)  0x20000C00)
#define USER_CODE_FLASH   ((u32)  0x08005000)
#define USER_INFO_FLASH	  ((u32) 0x08004000)

#define VEND_ID0 0xAD
#define VEND_ID1 0xDE
#define PROD_ID0 0xDE
#define PROD_ID1 0xC0


//Any extra code for main, e.g. for timeouts
//#define CONFIG_EXTRA_MAIN_CODE

/* while this is '1' we're looping in the bootloader */
#define CONFIG_EXTRA_MAIN_CODE \
	bool no_user_jump = (((!checkUserCode(USER_CODE_FLASH)) && (!checkUserCode(USER_CODE_RAM))) || readPin(BUTTON_BANK,BUTTON)); \
	int delay_count =0;

#define bootloaderCondition (delay_count++ < BOOTLOADER_WAIT)

/* define to 0 to never exit, undefine to save space */
	//#define bootloaderExitCondition 0

#endif
