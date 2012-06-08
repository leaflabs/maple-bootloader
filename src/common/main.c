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
 *  @file main.c
 *
 *  @brief main loop and calling any hardware init stuff. timing hacks for EEPROM
 *  writes not to block usb interrupts. logic to handle 2 second timeout then
 *  jump to user code.
 *
 */

#include "common.h"
#include "dfu.h"

int main() {
	systemReset(); // peripherals but not PC
	boardInit();
	setupFLASH();
	usbAppInit();
	strobePin(LED_BANK,LED,STARTUP_BLINKS,BLINK_FAST);
#ifdef CONFIG_EXTRA_MAIN_CODE
	CONFIG_EXTRA_MAIN_CODE
#endif

/* This code now moved to CONFIG_EXTRA_MAIN_CODE */
//  bool no_user_jump = (((!checkUserCode(USER_CODE_FLASH)) && (!checkUserCode(USER_CODE_RAM))) || readPin(BUTTON_BANK,BUTTON));
//  #ifdef CONFIG_BOOTLOADER_WAIT
//  int delay_count = 0;
//  #endif

/* wait for host to upload program or halt bootloader */
		while (bootloaderCondition) {
			strobePin(LED_BANK,LED,1,BLINK_SLOW);
			if (dfuUploadStarted()) {
				dfuFinishUpload(); // systemHardReset from DFU once done
			}
#ifdef bootloaderExitCondition
			if (bootloaderExitCondition)
				break;
#endif
		}

	if (checkUserCode(USER_CODE_RAM)) {
		jumpToUser(USER_CODE_RAM);
	} else if (checkUserCode(USER_CODE_FLASH)) {
		jumpToUser(USER_CODE_FLASH);
	} else {
		// some sort of fault occurred, hard reset
		strobePin(LED_BANK,LED,5,BLINK_FAST);
		systemHardReset();
	}
	return 0;
}
