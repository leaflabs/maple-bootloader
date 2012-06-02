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
 *  @file usb_descriptor.c
 *
 *  @brief aka application descriptor; big static struct and callbacks for sending
 *  the descriptor.
 *
 */


#include "usb_descriptor.h"

u8 u8_usbDeviceDescriptorDFU[18] =
  {
    0x12,   /* bLength */
    0x01,   /* bDescriptorType */
    0x00,   /* bcdUSB, version 1.00 */
    0x01,
    0x00,   /* bDeviceClass : See interface */
    0x00,   /* bDeviceSubClass : See interface*/
    0x00,   /* bDeviceProtocol : See interface */
    bMaxPacketSize, /* bMaxPacketSize0 0x40 = 64 */
    VEND_ID0,   /* idVendor     (0110) */
    VEND_ID1,

    PROD_ID0,   /* idProduct (0x1001 or 1002) */
    PROD_ID1,

    0x01,   /* bcdDevice*/
    0x02,
    0x01,   /* iManufacturer : index of string Manufacturer  */
    0x02,   /* iProduct      : index of string descriptor of product*/
    0x03,   /* iSerialNumber : index of string serial number*/
    0x01    /*bNumConfigurations */
  };

ONE_DESCRIPTOR usbDeviceDescriptorDFU =
  {
    u8_usbDeviceDescriptorDFU,
    0x12
  };

u8 u8_usbFunctionalDescriptor[9] =
  {
    /******************** DFU Functional Descriptor********************/
    0x09,   /*blength = 7 Bytes*/
    0x21,   /* DFU Functional Descriptor*/
    0x01,   /*bmAttribute, can only download for now */
    0xFF,   /*DetachTimeOut= 255 ms*/
    0x00,
    (wTransferSize & 0x00FF),
    (wTransferSize & 0xFF00) >> 8, /* TransferSize = 1024 Byte*/
    0x10,                          /* bcdDFUVersion = 1.1 */
    0x01
  };

ONE_DESCRIPTOR usbFunctionalDescriptor =
  {
    u8_usbFunctionalDescriptor,
    0x09
  };


u8 u8_usbConfigDescriptorDFU[] =
  {
    0x09,   /* bLength: Configuation Descriptor size */
    0x02,   /* bDescriptorType: Configuration */
    0x24,   /* wTotalLength: Bytes returned */
    0x00,
    0x01,   /* bNumInterfaces: 1 interface */
    0x01,   /* bConfigurationValue: */
    0x00,   /* iConfiguration: */
    0x80,   /* bmAttributes: */
    0x32,   /* MaxPower 100 mA */
    /* 09 */

    /************ Descriptor of DFU interface 0 Alternate setting 0 *********/
    0x09,   /* bLength: Interface Descriptor size */
    0x04,   /* bDescriptorType: */
    0x00,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x00,   /* bNumEndpoints*/
    0xFE,   /* bInterfaceClass: DFU */
    0x01,   /* bInterfaceSubClass */
    0x02,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
    0x04,   /* iInterface: */

    /************ Descriptor of DFU interface 0 Alternate setting 1 *********/
    0x09,   /* bLength: Interface Descriptor size */
    0x04,   /* bDescriptorType: */
    0x00,   /* bInterfaceNumber: Number of Interface */
    0x01,   /* bAlternateSetting: Alternate setting */
    0x00,   /* bNumEndpoints*/
    0xFE,   /* bInterfaceClass: DFU */
    0x01,   /* bInterfaceSubClass */
    0x02,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
    0x05,   /* iInterface: */

    /************ Descriptor of DFU interface 0 Alternate setting 2 *********/
    0x09,   /* bLength: Interface Descriptor size */
    0x04,   /* bDescriptorType: */
    0x00,   /* bInterfaceNumber: Number of Interface */
    0x02,   /* bAlternateSetting: Alternate setting */
    0x00,   /* bNumEndpoints*/
    0xFE,   /* bInterfaceClass: DFU */
    0x01,   /* bInterfaceSubClass */
    0x02,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
    0x06,   /* iInterface: */

    /******************** DFU Functional Descriptor********************/
    0x09,   /*blength = 7 Bytes*/
    0x21,   /* DFU Functional Descriptor*/
    0x01,   /*bmAttribute, can only download for now */
    0xFF,   /*DetachTimeOut= 255 ms*/
    0x00,
    (wTransferSize & 0x00FF),
    (wTransferSize & 0xFF00) >> 8, /* TransferSize = 1024 Byte*/
    0x10,                          /* bcdDFUVersion = 1.1 */
    0x01
    /***********************************************************/
    /*36*/
  };

ONE_DESCRIPTOR usbConfigDescriptorDFU =
  {
    u8_usbConfigDescriptorDFU,
    0x24
  };

u8 u8_usbStringLangId[0x04] =
  {
    0x04,
    0x03,
    0x09,
    0x04    /* LangID = 0x0409: U.S. English */
  };


u8 u8_usbStringVendor[] =
{
	0x16,
	0x03,
	'n', 0x0, 'c', 0x0, 'r', 0x0, 'm', 0x0, 'n', 0x0, 't', 0x0,
	'.', 0x0, 'o', 0x0, 'r', 0x0, 'g', 0x0,
};

u8 u8_usbStringProduct[] =
{
	0x1A,
	0x03,
	'M', 0x0, 'o', 0x0, 't', 0x0, 'o', 0x0, 'r', 0x0, ' ', 0x0,
	'C', 0x0, 'o', 0x0, 'r', 0x0, 't', 0x0, 'e', 0x0, 'x', 0x0,
};

u8 u8_usbStringSerial[] =
{
	0x1A,
	0x03,
	'0', 0x0, '.', 0x0, '9', 0x0, ' ', 0x0, 'A', 0x0, 'z', 0x0, 'r', 0x0,
	'a', 0x0, ' ', 0x0, 'O', 0x0, 'n', 0x0, 'e', 0x0,
};

u8 u8_usbStringAlt0[] =
{
	0x16,
	0x03,
	'D', 0x0, 'F', 0x0, 'U', 0x0, ' ', 0x0, 't', 0x0, 'o', 0x0, ' ', 0x0,
	'R', 0x0, 'A', 0x0, 'M', 0x0,
};

u8 u8_usbStringAlt1[] =
{
	0x1A,
	0x03,
	'D', 0x0, 'F', 0x0, 'U', 0x0, ' ', 0x0, 't', 0x0, 'o', 0x0, ' ', 0x0, 'F',
	0x0, 'l', 0x0, 'a', 0x0, 's', 0x0, 'h', 0x0,
};

u8 u8_usbStringAlt2[] =
{
	0x18,
	0x03,
	'D', 0x0, 'F', 0x0, 'U', 0x0, ' ', 0x0, 't', 0x0, 'o', 0x0, ' ', 0x0, 'I',
	0x0, 'N', 0x0, 'F', 0x0, 'O', 0x0,
};


u8 u8_usbStringInterface = 0;

ONE_DESCRIPTOR usbStringDescriptor[] =
  {
    { (u8*)u8_usbStringLangId,  0x04 },
    { (u8*)u8_usbStringVendor,  0x16 },
    { (u8*)u8_usbStringProduct, 0x1A },
    { (u8*)u8_usbStringSerial,  0x1A },
    { (u8*)u8_usbStringAlt0,    0x16 },
    { (u8*)u8_usbStringAlt1,    0x1A },
    { (u8*)u8_usbStringAlt2,    0x18 },
  };
