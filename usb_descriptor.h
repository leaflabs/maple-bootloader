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

#ifndef __MAPLE_USB_DESC_H
#define __MAPLE_USB_DESC_H

#include "common.h"
#include "usb_lib.h"
#include "usb.h"

#define USB_WCID_REQUEST_CODE 0x20

#define NUM_ALT_SETTINGS 2
#define STR_DESC_LEN 6


typedef struct wcid_compat_id_descriptor {
    u32 dwLength;
    u16 wVersion;
    u16 wIndex;
    u8  bNumSections;
    u8  bReserved[7];
    u8  bInterfaceNumber;
    u8  bReserved2;
    u8  bCompatibleId[8];
    u8  bSubCompatibleId[8];
    u8  bReserved3[6];
} __attribute((__packed__)) wcid_compat_id_descriptor;

extern ONE_DESCRIPTOR wcid_Descriptor;
extern ONE_DESCRIPTOR iMS_Descriptor;
extern ONE_DESCRIPTOR  usbDeviceDescriptorDFU;
extern ONE_DESCRIPTOR  usbConfigDescriptorDFU;
extern ONE_DESCRIPTOR  usbStringDescriptor[6];
extern ONE_DESCRIPTOR  usbFunctionalDescriptor;

#endif
