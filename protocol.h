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

/* config constants are in config.h */

typedef enum {
  SP_GET_INFO,
  SP_ERASE_PAGE,
  SP_WRITE_BYTES,
  SP_READ_BYTES,
  SP_JUMP_TO_USER,
  SP_SOFT_RESET
} SP_Cmd; // should force this enum to be 1 byte

/* note that because of endianness, the byteorder of these struct may
   not match the byte order in which they are transacted */

/* standard packet struct */
typedef struct {
  uint8  start;
  uint8  sequence_num;
  uint16 msg_len;
  uint8  token;
  uint8* msg; // uses the c99 feature of extending struct via tail pointer
  // checksum not included in struct to use the c99 last-field-ptr trick
} __attribute__((__packed__)) SP_Packet;



/* command structs. Even single byte payloads are typed for consistency */
typedef struct {
  SP_Cmd cmd;
}  SP_GET_INFO_Q;

typedef struct {
  uint8   cmd;
  uint8   endianness;
  uint32  total_ram;
  uint32  total_flash;
  uint16  page_size;
  uint32* user_addr_flash;
  uint32* user_addr_ram;
  uint32  version;
}  SP_GET_INFO_R;



typedef struct {
  uint8   cmd;
  uint32* addr;
}  SP_ERASE_PAGE_Q;

typedef struct {
  SP_Cmd cmd;
  uint8 success; // cant use bool without packing the bool enum, which is undesirable
}  SP_ERASE_PAGE_R;



typedef struct {
  uint8   cmd;
  uint32* addr;
  uint8*  payload; // uses the c99 feature to extend structs via tail pointers
}  SP_WRITE_BYTES_Q;

typedef struct {
  SP_Cmd cmd;
  uint8 success;
}  SP_WRITE_BYTES_R;



typedef struct {
  uint8   cmd;
  uint32* addr;
  uint16  len;
}  SP_READ_BYTES_R;

typedef struct {
  uint8  cmd;
  uint8* payload; // uses c99 feature of extending struct by tail pointer
}  SP_READ_BYTES_Q;



typedef struct {
  uint8   cmd;
  uint8   user_region; // 0 for flash, 1 for ram
}  SP_JUMP_TO_USER_Q;

typedef struct {
  SP_Cmd cmd;
  uint8 success;
}  SP_JUMP_TO_USER_R;



typedef struct {
  SP_Cmd cmd;
}  SP_SOFT_RESET_Q;

typedef struct {
  SP_Cmd cmd;
  uint8 success;
}  SP_SOFT_RESET_R;


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


#endif
