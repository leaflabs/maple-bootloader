#ifndef __MAPLE_TYPES_H
#define __MAPLE_TYPES_H

/* exposed library structs */
typedef void (*FuncPtr)(void);

typedef struct _LINE_CODING
{
  u32 bitrate;
  u8 format;
  u8 paritytype;
  u8 datatype;
} LINE_CODING;

typedef struct {
  vu32 ISER[2];
  u32  RESERVED0[30];
  vu32 ICER[2];
  u32  RSERVED1[30];
  vu32 ISPR[2];
  u32  RESERVED2[30];
  vu32 ICPR[2];
  u32  RESERVED3[30];
  vu32 IABR[2];
  u32  RESERVED4[62];
  vu32 IPR[15];
} NVIC_TypeDef;

typedef struct {
  u8 NVIC_IRQChannel;
  u8 NVIC_IRQChannelPreemptionPriority;
  u8 NVIC_IRQChannelSubPriority;
  bool NVIC_IRQChannelCmd; /* TRUE for enable */
} NVIC_InitTypeDef;

typedef struct {
  vuc32 CPUID;
  vu32 ICSR;
  vu32 VTOR;
  vu32 AIRCR;
  vu32 SCR;
  vu32 CCR;
  vu32 SHPR[3];
  vu32 SHCSR;
  vu32 CFSR;
  vu32 HFSR;
  vu32 DFSR;
  vu32 MMFAR;
  vu32 BFAR;
  vu32 AFSR;
} SCB_TypeDef;

typedef struct {
  FuncPtr user_serial_tx_cb;
  FuncPtr user_serial_rx_cb;
  FuncPtr user_serial_linecoding_cb;
  u32* serial_count_in;
  u32* serial_count_out;
  u8*  serial_buffer_out;
  LINE_CODING* user_serial_linecoding;
  u8 major_rev_number;
  u8 minor_rev_number;

  /* this might be a tad overkill, but it 
     allows us to completely steal the usb
     from the bootloader in userland in the 
     event the library one day includes more
     generic usb support! */
  void *usb_device_ptr;

  void *reserved0;
  void *reserved1;
  void *reserved2;
  void *reserved3;
} MAPLE_VectorTable;

#endif //__MAPLE_TYPES_H
