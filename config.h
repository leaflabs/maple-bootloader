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

// todo, hook these into git versioning
#define VERSION_MAJ 0x0006
#define VERSION_MIN 0x0000

#define LED_BANK GPIOA
#define LED      5
#define BLINK_FAST 0x50000
#define BLINK_SLOW 0x100000

#define BUTTON_BANK GPIOC
#define BUTTON      9

#define STARTUP_BLINKS  5
#define RESET_BLINKS    3
#define BOOTLOADER_WAIT 6

#define USER_CODE_RAM     ((u32)0x20000C00)
#define USER_CODE_FLASH   ((u32)0x08005000)
#define TOTAL_RAM         ((u32)0x00005000)  // 20KB, todo - extern this value to the linker for dynamic compute
#define TOTAL_FLASH       ((u32)0x0001E000)  // 120KB, todo - extern this value to the linker
#define TOTAL_BOOT_RAM    ((u32)0x00000C00)  // todo, get the linker to set this
#define TOTAL_BOOT_FLASH  ((u32)0x00005000)  // todo, get the linker to set this
#define FLASH_START       ((u32)0x00000000)
#define FLASH_START_ALT   ((u32)0x08000000)
#define RAM_START         ((u32)0x20000000)


#define VEND_ID0 0xAF
#define VEND_ID1 0x1E
#define PROD_ID0 0x03
#define PROD_ID1 0x00

/* serial protocol related constants, SP - Serial Protocol */
// todo, ifdef some of these values based on chip being used
#define SP_START       0x1B
#define SP_ENDIANNESS  0
#define SP_PAGE_SIZE     0x400       // 1KB
#define SP_MAX_READ_LEN  0x400
#define SP_MAX_WRITE_LEN 0x400
#define SP_MAX_MSG_LEN   (0x400 + 0x05)      // max payload + write_bytes overhead

#define SP_TOTAL_RAM   (TOTAL_RAM - TOTAL_BOOT_RAM)
#define SP_TOTAL_FLASH (TOTAL_FLASH - TOTAL_BOOT_FLASH)

// temp defines due to time constraints
#define SP_SIZEOF_PHEADER 5
#define SP_SIZEOF_PFOOTER 4        // just the 4 byte checksum
#define SP_BYTE_TIMEOUT   10000000 // roughly, code branching makes this not a fixed "time". TODO make this dynamic or even specified as part of the comm. transaction. SINGE BYTE timeout
#define SP_LEN_CHECKSUM   4        // its already fixed as uint32, but this ought to be a parameter
#define SP_TOKEN       0x7F

#endif
