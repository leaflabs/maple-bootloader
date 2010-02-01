#include "maple_usb_desc.h"

u8 u8_usbDeviceDescriptorAPP[18] = 
  {
    0x12,   /* bLength */
    0x01,   /* bDescriptorType */
    0x00,   /* bcdUSB, version 1.00 */
    0x01,
    0xEF,   /* bDeviceClass : EF for composite IAD device (experimental!) */
    0x02,   /* bDeviceSubClass : See interface*/
    0x01,   /* bDeviceProtocol : See interface */
    bMaxPacketSize, /* bMaxPacketSize0 0x40 = 64 */
    VEND_ID0,   /* idVendor     (0110) */
    VEND_ID1,
    PROD_ID0,   /* idProduct (0x1001) */
    PROD_ID1,
    0x00,   /* bcdDevice*/
    0x02,
    0x01,   /* iManufacturer : index of string Manufacturer  */
    0x02,   /* iProduct      : index of string descriptor of product*/
    0x03,   /* iSerialNumber : index of string serial number*/
    0x01    /*bNumConfigurations */
  };

ONE_DESCRIPTOR usbDeviceDescriptorAPP = 
  {
    u8_usbDeviceDescriptorAPP,
    0x12
  };

u8 u8_usbDeviceDescriptorDFU[18] = 
  {
    0x12,   /* bLength */
    0x01,   /* bDescriptorType */
    0x00,   /* bcdUSB, version 1.00 */
    0x01,
    0xFE,   /* bDeviceClass : See interface */
    0x01,   /* bDeviceSubClass : See interface*/
    0x00,   /* bDeviceProtocol : See interface */
    bMaxPacketSize, /* bMaxPacketSize0 0x40 = 64 */
    VEND_ID0,   /* idVendor     (0110) */
    VEND_ID1,

    PROD_ID0_DFU,   /* idProduct (0x1001 or 1002) */
    PROD_ID1,

    0x00,   /* bcdDevice*/
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
    0xFF,
    (wTransferSize & 0x00FF),
    (wTransferSize & 0xFF00) >> 8, /* TransferSize = 1024 Byte*/
    0x1A,                          /* bcdDFUVersion*/
    0x01
  };

ONE_DESCRIPTOR usbFunctionalDescriptor = 
  {
    u8_usbFunctionalDescriptor,
    0x09
  };


u8 u8_usbConfigDescriptorAPP[102] = 
  {
    0x09,   /* bLength: Configuation Descriptor size */
    0x02,   /* bDescriptorType: Configuration */
    0x5E,   /* wTotalLength: Bytes returned (94) */
    0x00,
    0x03,   /* bNumInterfaces: 2 virtual com + DFU interfaces */
    0x01,   /* bConfigurationValue: */
    0x00,   /* iConfiguration: */
    0x80,   /* bmAttributes: */
    0x32,   /* MaxPower 100 mA */
    /* 09 */

    /************ IAD Union descriptor, to combine the two cdc interfaces ******/
    0x08,   /* blength */
    0x0B,   /* bDescriptorType: 11, IAD */
    0x00,   /* bFirstInterface: 0 */
    0x02,   /* bInterfaceCount: Number of contiguous combined interfaces */
    0x02,   /* bFunctionClass: same as for CDC ACM */
    0x02,   /* bFunctionSubClass: same as for CDC ACM */
    0x00,   /* bFucntionProtocol: same as for CDC ACM */
    0x00,   /* iFunction: string index */

    /************ Descriptor of of Communication Interface Class *********/
    0x09,   /* bLength: Interface Descriptor size */
    0x04,   /* bDescriptorType: Interface */
    0x00,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x01,   /* bNumEndpoints: One endpoints used */
    0x02,   /* bInterfaceClass: Communication Interface Class */
    0x02,   /* bInterfaceSubClass: Abstract Control Model */
    0x01,   /* bInterfaceProtocol: Common AT commands */
    0x00,   /* iInterface: */
    /*Header Functional Descriptor*/
    0x05,   /* bLength: Endpoint Descriptor size */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x00,   /* bDescriptorSubtype: Header Func Desc */
    0x10,   /* bcdCDC: spec release number */
    0x01,
    /*Call Managment Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x01,   /* bDescriptorSubtype: Call Management Func Desc */
    0x00,   /* bmCapabilities: D0+D1 */
    0x01,   /* bDataInterface: 1 */
    /*ACM Functional Descriptor*/
    0x04,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
    0x02,   /* bmCapabilities */
    /*Union Functional Descriptor*/
    0x05,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x06,   /* bDescriptorSubtype: Union func desc */
    0x00,   /* bMasterInterface: Communication class interface */
    0x01,   /* bSlaveInterface0: Data Class Interface */
    /*Endpoint 2 Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    0x05,   /* bDescriptorType: Endpoint */
    0x82,   /* bEndpointAddress: (IN2) */
    0x03,   /* bmAttributes: Interrupt */
    0x08,   /* wMaxPacketSize: */
    0x00,
    0xFF,   /* bInterval: */
    /* 44 */

    /************ Descriptor of of Data Interface Class *********/
    0x09,   /* bLength: Endpoint Descriptor size */
    0x04,   /* bDescriptorType: */
    0x01,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x02,   /* bNumEndpoints: Two endpoints used */
    0x0A,   /* bInterfaceClass: CDC */
    0x00,   /* bInterfaceSubClass: */
    0x00,   /* bInterfaceProtocol: */
    0x00,   /* iInterface: */
    /*Endpoint 3 Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    0x05,   /* bDescriptorType: Endpoint */
    0x03,   /* bEndpointAddress: (OUT3) */
    0x02,   /* bmAttributes: Bulk */
    0x40,   /* wMaxPacketSize: */
    0x00,
    0x00,   /* bInterval: ignore for Bulk transfer */
    /*Endpoint 1 Descriptor*/
    0x07,   /* bLength: Endpoint Descriptor size */
    0x05,   /* bDescriptorType: Endpoint */
    0x81,   /* bEndpointAddress: (IN1) */
    0x02,   /* bmAttributes: Bulk */
    0x40,   /* wMaxPacketSize: */
    0x00,
    0x00,    /* bInterval */
    /* 67 */

    /************ Descriptor of DFU interface 0 Alternate setting 0 *********/
    0x09,   /* bLength: Interface Descriptor size */
    0x04,   /* bDescriptorType: */
    0x02,   /* bInterfaceNumber: Number of Interface */
    0x00,   /* bAlternateSetting: Alternate setting */
    0x00,   /* bNumEndpoints*/
    0xFE,   /* bInterfaceClass: DFU */
    0x01,   /* bInterfaceSubClass */
    0x01,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
    0x04,   /* iInterface: */
    /* 76 */

    /************ Descriptor of DFU interface 0 Alternate setting 1 *********/
    0x09,   /* bLength: Interface Descriptor size */
    0x04,   /* bDescriptorType: */
    0x02,   /* bInterfaceNumber: Number of Interface */
    0x01,   /* bAlternateSetting: Alternate setting */
    0x00,   /* bNumEndpoints*/
    0xFE,   /* bInterfaceClass: DFU */
    0x01,   /* bInterfaceSubClass */
    0x01,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
    0x05,   /* iInterface: */
    /* 85 */

    /******************** DFU Functional Descriptor********************/
    0x09,   /*blength = 7 Bytes*/
    0x21,   /* DFU Functional Descriptor*/
    0x01,   /*bmAttribute, can only download for now */
    0xFF,   /*DetachTimeOut= 255 ms*/
    0xFF,
    (wTransferSize & 0x00FF),
    (wTransferSize & 0xFF00) >> 8, /* TransferSize = 1024 Byte*/
    0x1A,                          /* bcdDFUVersion*/
    0x01
    /***********************************************************/
    /* 94 */
  };

ONE_DESCRIPTOR usbConfigDescriptorAPP = 
  {
    u8_usbConfigDescriptorAPP,
    0x66
  };


u8 u8_usbConfigDescriptorDFU[36] = 
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

#if COMM_ENB
    0x02,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
#else
    0x01,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
#endif

    0x04,   /* iInterface: */

    /************ Descriptor of DFU interface 0 Alternate setting 1 *********/
    0x09,   /* bLength: Interface Descriptor size */
    0x04,   /* bDescriptorType: */
    0x00,   /* bInterfaceNumber: Number of Interface */
    0x01,   /* bAlternateSetting: Alternate setting */
    0x00,   /* bNumEndpoints*/
    0xFE,   /* bInterfaceClass: DFU */
    0x01,   /* bInterfaceSubClass */

#if COMM_ENB
    0x02,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
#else
    0x01,   /* nInterfaceProtocol, switched to 0x02 while in dfu_mode */
#endif

    0x05,   /* iInterface: */

    /******************** DFU Functional Descriptor********************/
    0x09,   /*blength = 7 Bytes*/
    0x21,   /* DFU Functional Descriptor*/
    0x01,   /*bmAttribute, can only download for now */
    0xFF,   /*DetachTimeOut= 255 ms*/
    0xFF,
    (wTransferSize & 0x00FF),
    (wTransferSize & 0xFF00) >> 8, /* TransferSize = 1024 Byte*/
    0x1A,                          /* bcdDFUVersion*/
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

u8 u8_usbStringVendor[0x12] = 
  {
    0x12,
    0x03,
    'L',0,'e',0,'a',0,'f',0,'L',0,'a',0,'b',0,'s',0
  };

u8 u8_usbStringProduct[0x14] = 
  {
    0x14,
    0x03,
    'M',0,'a',0,'p',0,'l',0,'e',0,' ',0,'0',0,'0',0,'1',0
  };

u8 u8_usbStringSerial[0x10] = 
  {
    0x10,
    0x03,
    'L',0,'L',0,'M',0,' ',0,'0',0,'0',0,'1',0
  };

u8 u8_usbStringAlt0[0x36] = 
  {
    0x10,
    0x03,
    'D',0,'F',0,'U',0,' ',0,'P',0,'r',0,'o',0,'g',0,'r',0,
    'a',0,'m',0,' ',0,'R',0,'A',0,'M',0,' ',0,'0',0,'x',0,
    '2',0,'0',0,'0',0,'0',0,'0',0,'C',0,'0',0,'0',0
  };

u8 u8_usbStringAlt1[0x3A] = 
  {
    0x3A,
    0x03,
    'D',0,'F',0,'U',0,' ',0,'P',0,'r',0,'o',0,'g',0,'r',0,
    'a',0,'m',0,' ',0,'F',0,'L',0,'A',0,'S',0,'H',0,' ',0,
    '0',0,'x',0,'0',0,'8',0,'0',0,'0',0,'5',0,'0',0,'0',0,
    '0',0
  };

u8 u8_usbStringInterface = NULL;

ONE_DESCRIPTOR usbStringDescriptor[6] =
  {
    { (u8*)u8_usbStringLangId,  0x04 },
    { (u8*)u8_usbStringVendor,  0x12 },
    { (u8*)u8_usbStringProduct, 0x20 },
    { (u8*)u8_usbStringSerial,  0x10 },
    { (u8*)u8_usbStringAlt0,    0x36 },
    { (u8*)u8_usbStringAlt1,    0x3A }
  };
