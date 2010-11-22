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

int main() {
  systemReset(); // peripherals but not PC
  setupCLK();
  setupLED();
  setupUSB();
  setupBUTTON();
  setupFLASH();

  strobePin(LED_BANK,LED,STARTUP_BLINKS,BLINK_FAST);

  /* wait for host to upload program or halt bootloader */
  bool no_user_jump = (!checkUserCode(USER_CODE_FLASH) && !checkUserCode(USER_CODE_RAM)) || readPin(BUTTON_BANK,BUTTON);

  int delay;
  if (no_user_jump) {delay = 0;}
  else              {delay = SP_BYTE_TIMEOUT;}

  // todo custom type, instead of SP_PacketStatus
  SP_PacketStatus status = sp_run(delay); // will return on timeout or never if delay=0

  // todo check usercode before exiting with a jump status
  if (status == SP_SYS_RESET) {
    /* wait a small delay for the host to clean up the com port and reset */
    strobePin(LED_BANK,LED,RESET_BLINKS,BLINK_FAST);
  } else if (status == SP_JUMP_RAM) {
    if (checkUserCode(USER_CODE_RAM)) {
      jumpToUser(USER_CODE_RAM);
    }

  } else if (status == SP_JUMP_FLASH) {
    if (checkUserCode(USER_CODE_FLASH)) {
      jumpToUser(USER_CODE_FLASH);
    }

  }

  // otherwise fallback on existing code
  if (checkUserCode(USER_CODE_RAM)) {
    jumpToUser(USER_CODE_RAM);
  } else if (checkUserCode(USER_CODE_FLASH)) {
    jumpToUser(USER_CODE_FLASH);
  } else {
    // some sort of fault occurred, hard reset
    strobePin(LED_BANK,LED,5,BLINK_FAST);
    systemHardReset();
  }
  
}
