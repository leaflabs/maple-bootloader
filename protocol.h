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

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "stm32f10x_type.h"

/* todos on design - really ought to segment the protocol layering
   explicitly. That is, a single function for creating packets,
   multiple functions for creating packet payloads. Now, the actual
   command functions DO create the packet header */

#define SP_SUCCESS      1
#define SP_FAIL         0
#define SP_REGION_RAM   1
#define SP_REGION_FLASH 0
/* config constants are in config.h */

typedef enum {
  INCOMING,
  OUTGOING
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
void            sp_setup_pbuf_out (SP_Packet* p_packet, uint16 msg_len);

SP_PacketStatus sp_handle_packet   (int delay, uint8* pbuf, uint16 pbuf_len);

SP_PacketStatus sp_get_packet      (SP_PacketBuf* p_packet);
SP_PacketStatus sp_dispatch_packet (SP_PacketBuf* p_packet);
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

// todo consider budling the various major and minor state vars into a monolithic struct, which might be useful

#endif
