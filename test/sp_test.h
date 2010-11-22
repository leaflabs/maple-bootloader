/*
  Basic test utility to implement a modified version of the serial
  bootloader command line protocol and verify it is working

  should not be run as a standalone program, instead it should be
  spawned by mapledude.py

  note that this is NOT the same file/module as protocol.h/c
*/

#ifndef __PROTOCOL_H
#define __PROTOCOL_H


#include <stdint.h>
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;

typedef int32_t  int32;
typedef int16_t  int16;
typedef int8_t   int8;

typedef uint32   u32;
typedef uint16   u16;
typedef uint8    u8;

typedef int32    s32;
typedef int16    s16;
typedef int8     s8;

typedef enum {FALSE = 0, TRUE = !FALSE} bool;

#define DEBUG 1

/* todos on design - really ought to segment the protocol layering
   explicitly. That is, a single function for creating packets,
   multiple functions for creating packet payloads. Now, the actual
   command functions DO create the packet header */

#define SP_SUCCESS      1
#define SP_FAIL         0
#define SP_REGION_RAM   1
#define SP_REGION_FLASH 0

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

// temp defines due to time constraints (from config.h)
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

#define DBG(x) sp_debug_blink(x)
/* config constants are in config.h */

typedef enum {
  SP_INCOMING,
  SP_OUTGOING
} SP_Direction;

typedef enum {
  SP_GET_INFO,
  SP_ERASE_PAGE,
  SP_WRITE_BYTES,
  SP_READ_BYTES,
  SP_JUMP_TO_USER,
  SP_SOFT_RESET
} SP_Cmd; // should force this enum to be 1 byte

typedef enum {
  SP_OK,
  SP_ERR_TIMEOUT,
  SP_ERR_START,
  SP_ERR_TOKEN,
  SP_ERR_CHKSUM,
  SP_ERR_CMD,
  SP_EXIT,
  SP_JUMP_RAM,
  SP_JUMP_FLASH,
  SP_SYS_RESET
} SP_PacketStatus;

// todo, make a packet header type and rework the packetBuf object to include this new type

/* note that because of endianness, the byteorder of these struct may
   not match the byte order in which they are transacted */

/* standard packet struct  (currently unused, subsumed by manual stuffing of SP_PacketBuf */
typedef struct {
  /* doesnt include msg body or checksum, those get parsed out later
     and added to SP_PacketBuf */
  uint8  start;
  uint8  sequence_num;
  uint16 msg_len;
  uint8  token;
} __attribute__((__packed__)) SP_PHeader;



/* command structs. Even single byte payloads are typed for consistency */
typedef struct {
  SP_Cmd cmd;
} __attribute__((__packed__))  SP_GET_INFO_Q;

typedef struct {
  uint8   cmd;
  uint8   endianness;
  uint32  total_ram;
  uint32  total_flash;
  uint16  page_size;
  uint32  user_addr_flash; // skip casts later by making this pointer a uint32 instead
  uint32  user_addr_ram;   // skip casts later by making this pointer a uint32 instead
  uint32  version;
} __attribute__((__packed__))  SP_GET_INFO_R;



typedef struct {
  uint8   cmd;
  uint32  addr;
} __attribute__((__packed__))  SP_ERASE_PAGE_Q;

typedef struct {
  SP_Cmd cmd;
  uint8 success; // cant use bool without packing the bool enum, which is undesirable
} __attribute__((__packed__))  SP_ERASE_PAGE_R;



typedef struct {
  uint8   cmd;
  uint32* addr;
  //  uint8*  payload; // uses the c99 feature to extend structs via tail pointers
} __attribute__((__packed__))  SP_WRITE_BYTES_Q;

typedef struct {
  SP_Cmd cmd;
  uint8 success;
} __attribute__((__packed__))  SP_WRITE_BYTES_R;



typedef struct {
  uint8   cmd;
  uint32* addr;
  uint16  len;
} __attribute__((__packed__))  SP_READ_BYTES_Q;

typedef struct {
  uint8  cmd;
  //  uint8* payload; // uses c99 feature of extending struct by tail pointer
} __attribute__((__packed__))  SP_READ_BYTES_R;



typedef struct {
  uint8   cmd;
  uint8   user_region; // 0 for flash, 1 for ram
} __attribute__((__packed__))  SP_JUMP_TO_USER_Q;

typedef struct {
  SP_Cmd cmd;
  uint8 success;
} __attribute__((__packed__))  SP_JUMP_TO_USER_R;



typedef struct {
  SP_Cmd cmd;
} __attribute__((__packed__))  SP_SOFT_RESET_Q;

typedef struct {
  SP_Cmd cmd;
  uint8 success;
} __attribute__((__packed__))  SP_SOFT_RESET_R;


/* create a generic cmd type */
// may or may not be a good idea ;)
typedef union {
  SP_GET_INFO_R      info;
  SP_ERASE_PAGE_R    erase;
  SP_WRITE_BYTES_R   write;
  SP_READ_BYTES_R    read;
  SP_JUMP_TO_USER_R  jump;
  SP_SOFT_RESET_R    reset;
} __attribute__((__packed__)) SP_Query; // packing will apply to all members

typedef union {
  SP_GET_INFO_R      info;
  SP_ERASE_PAGE_R    erase;
  SP_WRITE_BYTES_R   write;
  SP_READ_BYTES_R    read;
  SP_JUMP_TO_USER_R  jump;
  SP_SOFT_RESET_R    reset;
} __attribute__((__packed__)) SP_Response; // packing will apply to all members

/* any packetbuf whose pindex != total_len should be filled or thrown
   out, never parsed */
typedef struct {
  int16        total_len; // total length of the WHOLE PACKET 
  int16        pindex;    // current write offset of the pbuffer (=total_len when full)
  SP_Direction direction; // 0 for incoming, 1 for outgoing
  uint8        sequence_num;
  uint8        token;
  uint8*       buffer;
  uint32       checksum;
} SP_PacketBuf;


/* Function Declarations */
SP_PacketStatus sp_run(int delay);

SP_PacketBuf    sp_create_pbuf_in (uint8* pbuf, uint16 pbuf_len);
void            sp_setup_pbuf_out (SP_PacketBuf* p_packet, uint16 msg_len);

SP_PacketStatus sp_handle_packet   (int delay, uint8* pbuf, uint16 pbuf_len);

SP_PacketStatus sp_get_packet      (SP_PacketBuf* p_packet);
SP_PacketStatus sp_dispatch_packet (SP_PacketBuf* p_packet,uint16* msg_len);
SP_PacketStatus sp_marshall_reply  (SP_PacketBuf* p_packet);
SP_PacketStatus sp_check_sum       (SP_PacketBuf* p_packet); // return status instead of bool here only for consistency/neatness

// todo, make these cmd functions eat a query type instead of a packet type
SP_PacketStatus sp_cmd_get_info     (SP_PacketBuf* p_packet, uint16* msg_len);
SP_PacketStatus sp_cmd_erase_page   (SP_PacketBuf* p_packet, uint16* msg_len);
SP_PacketStatus sp_cmd_write_bytes  (SP_PacketBuf* p_packet, uint16* msg_len);
SP_PacketStatus sp_cmd_read_bytes   (SP_PacketBuf* p_packet, uint16* msg_len);
SP_PacketStatus sp_cmd_jump_to_user (SP_PacketBuf* p_packet, uint16* msg_len);
SP_PacketStatus sp_cmd_soft_reset   (SP_PacketBuf* p_packet, uint16* msg_len);

void   sp_reverse_bytes(uint8* buf_in, uint16 len); // reverse bytes in place
uint32 sp_compute_checksum(uint8* buf_in, uint16 len); // todo consider not bothering with endianness here, since only consistency counts here - might as well go little endian
bool   sp_addr_in_ram(uint32* addr);
uint32 sp_maligned_cast_u32(uint8* start);
void   sp_maligned_copy_u32(uint32 val, uint8* dest);

// todo consider budling the various major and minor state vars into a monolithic struct, which might be useful
void sp_debug_blink(int i);

// reimplemented for the test
uint16 usbSendBytes(uint8* sendBuf,uint16 len);
uint8 usbBytesAvailable(void);
uint8 usbReceiveBytes(uint8* recvBuf, uint8 len);

void systemHardReset(void);
bool checkUserCode (u32 usrAddr);
void jumpToUser    (u32 usrAddr);

bool flashWriteWord  (u32 addr, u32 word);
bool flashErasePage  (u32 addr);
bool flashErasePages (u32 addr, u16 n);
void flashLock       (void);
void flashUnlock     (void);

#endif
