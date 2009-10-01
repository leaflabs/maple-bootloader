#include "maple_usb_desc.h"

u8* u8_usbDeviceDescriptor = 
  {
    0x12,   /* bLength */
    0x01,   /* bDescriptorType */
    0x00,   /* bcdUSB, version 1.00 */
    0x01,
    0x00,   /* bDeviceClass : See interface */
    0x00,   /* bDeviceSubClass : See interface*/
    0x00,   /* bDeviceProtocol : See interface */
    bMaxPacketSize, /* bMaxPacketSize0 0x40 = 64 */
    0x10,   /* idVendor     (1001) */
    0x01,
    0x01,   /* idProduct (0x0110) */
    0x10,
    0x00,   /* bcdDevice*/
    0x02,
    0x00,   /* iManufacturer : index of string Manufacturer  */
    0x00,   /* iProduct      : index of string descriptor of product*/
    0x00,   /* iSerialNumber : index of string serial number*/
    0x01    /*bNumConfigurations */
  };

ONE_DESCRIPTOR usbDeviceDescriptor = 
  {
    u8_usbDeviceDescriptor,
    0x12
  };

u8* u8_usbConfigDescriptor = 
  {
    0x09,   /* bLength: Configuation Descriptor size */
    0x02,   /* bDescriptorType: Configuration */
    0x1B,   /* wTotalLength: Bytes returned */
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
    0x01,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
    0x00,   /* iInterface: */

    /******************** DFU Functional Descriptor********************/
    0x09,   /*blength = 7 Bytes*/
    0x21,   /* DFU Functional Descriptor*/
    0x03,   /*bmAttribute */
    0xFF,   /*DetachTimeOut= 255 ms*/
    0xFF,
    (wTransferSize & 0x00FF),
    (wTransferSize & 0xFF00) >> 8, /* TransferSize = 1024 Byte*/
    0x1A,                          /* bcdDFUVersion*/
    0x01
    /***********************************************************/
    /*27*/
  };

ONE_DESCRIPTOR usbConfigDescriptor = 
  {
    u8_usbConfigDescriptor,
    0x1B
  };

ONE_DESCRIPTOR* usbStringDescriptor = 
  {{u8_usbStringLangId,
    0x01}
  };

u8* u8_usbStringLandId    = {'0'};
u8* u8_usbStringVendor    = {'0'};
u8* u8_usbStringProduct   = {'0'};
u8* u8_usbStringSerial    = {'0'};
u8* u8_usbStringInterface = {'0'};
