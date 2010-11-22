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
 *  @file protocol.c
 *
 *  @brief The principle communications state machine as well as the data
 *  transfer callbacks accessed by the usb library
 *   
 *
 */

// todo implement timeouts

#include "protocol.h"
#include "common.h"

SP_PacketStatus sp_run(int delay) {
  uint8 sp_buffer[SP_MAX_MSG_LEN+SP_SIZEOF_PHEADER+SP_SIZEOF_PFOOTER];

  SP_PacketStatus status;
  bool done = FALSE;
  while (!done) {
    // since packets are processed atomically, we need only one function
    status = sp_handle_packet(delay, &sp_buffer[0], sizeof(sp_buffer)); 

    if (status == SP_ERR_TIMEOUT || 
        status == SP_EXIT        || 
        status == SP_ERR_START   ||
        status == SP_SYS_RESET   ||
        status == SP_JUMP_RAM    ||
        status == SP_JUMP_FLASH) {
      /* exist conditions for command interface */
      done = TRUE;
    }
    // todo if status != idle, consider cleaning up the packet, flushing the buffer etc.
  }

  return status;
  
}

SP_PacketStatus sp_handle_packet(int delay, uint8* pbuf, uint16 pbuf_len) {
  // assert(pbuf_len >= SP_MAX_MSG_LEN + SP_SIZEOF_PHEADER+SP_SIZEOF_PFOOTER
  bool permanent  = TRUE;
  SP_PacketStatus dispatch_status = SP_OK; // to handle special cases of jump and reset

  if (delay) {
    permanent = FALSE;
  } else {
    delay = 1; // set to non zero but never decrement again
  }

  while (delay > 0) {
    if (usbBytesAvailable() >= SP_SIZEOF_PHEADER) {
      usbReceiveBytes(pbuf,SP_SIZEOF_PHEADER);
      
      /* check the first byte before ANYTHING so we can jump out
         quickly if theres no bootloader traffic */
      if (pbuf[0] != SP_START) {
        return SP_ERR_START;
      }

      SP_PacketBuf this_packet = sp_create_pbuf_in(pbuf,pbuf_len);
      
      /* stk500 doesnt check sequence nums on incoming packets so skip that */


      /* check the token  */
      if (this_packet.token != SP_TOKEN) {
        return SP_ERR_TOKEN;
      }
      
      SP_PacketStatus status = sp_get_packet(&this_packet); // todo perhaps generate a unique status type for this level 
      if (status != SP_OK) {
        /* perhaps handle with cleanup or more care, this is really passing the buck. */
        return status;
      }

      uint16 msg_len;
      status = sp_dispatch_packet(&this_packet,&msg_len); // will modify packet in place to be the return!
      // todo, redesign so as to not have these corner cases for jump and reset
      if (status != SP_OK &&
          status != SP_SYS_RESET &&
          status != SP_JUMP_RAM &&
          status != SP_JUMP_FLASH) {
        return status; // err, timeout, or exit
      }
      dispatch_status = status; // stash this result, its either OK or special case

      sp_setup_pbuf_out(&this_packet,msg_len);

      /* send it out */
      status = sp_marshall_reply(&this_packet); // reset delay to give a fresh tx timeout
      if (status != SP_OK) {
        return status;
      }

      return dispatch_status; // pass the special command back to main

    } else {
      if (!permanent) {
        delay--;
      }
    }
  }

  return SP_ERR_TIMEOUT;
  
}

SP_PacketStatus sp_get_packet(SP_PacketBuf* p_packet) {
  /* Marshall the rest of the data payload into the packet */
  int timeout = SP_BYTE_TIMEOUT;

  while (p_packet->pindex != p_packet->total_len) {
    uint8 bytes_in = usbReceiveBytes(&p_packet->buffer[p_packet->pindex],p_packet->total_len-p_packet->pindex);
    if (!bytes_in) {
      if (timeout-- == 0) {
        // Ooops...
        return SP_ERR_TIMEOUT;
      }
    } else {
      p_packet->pindex += bytes_in;
    }
  }

  // reverse the checksum from big endian
  sp_reverse_bytes(&(p_packet->buffer[p_packet->pindex-SP_LEN_CHECKSUM]),SP_LEN_CHECKSUM);
  p_packet->checksum = (uint32) p_packet->buffer[p_packet->pindex-SP_LEN_CHECKSUM];

  return sp_check_sum(p_packet);
}

SP_PacketStatus sp_marshall_reply(SP_PacketBuf* p_packet) {
  /* loops over trying to send the response, can timeout */
  int timeout = SP_BYTE_TIMEOUT;
  while (p_packet->pindex != p_packet->total_len) {
    uint8 bytes_out = usbSendBytes(&p_packet->buffer[p_packet->pindex],p_packet->total_len - p_packet->pindex);
    if (!bytes_out) {
      if (timeout-- == 0) {
        // side effects not undone, but things like user_jump or soft_reset will not execute now
        return SP_ERR_TIMEOUT;
      }
    } else {
      p_packet->pindex += bytes_out;
    }
  }

  return SP_OK;
  
}


SP_PacketStatus sp_check_sum(SP_PacketBuf* p_packet) {
  /* checksum is the XOR of all 4 byte words. Interpret these words in
     network order (big endian). Pad if necessary. Although, since everything is big endian, 
     padding will have no effect on the resultant word value, so you can really do nothing */
  uint16 total_len = p_packet->total_len - SP_SIZEOF_PFOOTER;
  uint32 checksum = sp_compute_checksum(p_packet->buffer,total_len);

  if (checksum != p_packet->checksum) {
    return SP_ERR_CHKSUM;
  }

  return SP_OK;
}



SP_PacketStatus sp_dispatch_packet(SP_PacketBuf* p_packet,uint16* msg_len) {
  /* besides dispatching the request, this function should also modify
     the packet inline into the response packet. argument msg_len is
     set by this function */

  // todo, make use of the SP_Query type and the c99 tail-ptr trick and make these cmd functions eat a query type instead of a packet type
  SP_Cmd cmd = (SP_Cmd) p_packet->buffer[SP_SIZEOF_PHEADER];

  if (cmd == SP_GET_INFO)          {return sp_cmd_get_info     (p_packet, msg_len);}
  else if (cmd == SP_ERASE_PAGE)   {return sp_cmd_erase_page   (p_packet, msg_len);} 
  else if (cmd == SP_WRITE_BYTES)  {return sp_cmd_write_bytes  (p_packet, msg_len);} 
  else if (cmd == SP_READ_BYTES)   {return sp_cmd_read_bytes   (p_packet, msg_len);}
  else if (cmd == SP_JUMP_TO_USER) {return sp_cmd_jump_to_user (p_packet, msg_len);} 
  else if (cmd == SP_SOFT_RESET)   {return sp_cmd_soft_reset   (p_packet, msg_len);}
  else {
    // doing this implicitly says non recognized commands are simply ignored
    return SP_ERR_CMD;
  }
}


SP_PacketBuf sp_create_pbuf_in(uint8* pbuf, uint16 pbuf_len) {
  // rever anything big endian
  sp_reverse_bytes(&pbuf[2],2); // reverses msg_len

  //  uint16 msg_len      = *((uint16*)(&pbuf[2]));   perhpas alignment issues here, create manually
  uint16 msg_len = pbuf[2]+(pbuf[3]<<8);
  uint8  sequence_num = pbuf[1];
  uint8  token        = pbuf[4];

  SP_PacketBuf ret_packet;
  ret_packet.total_len    = msg_len + SP_SIZEOF_PHEADER+SP_SIZEOF_PFOOTER;
  ret_packet.pindex       = SP_SIZEOF_PHEADER; // index of the NEXT byte to be filled
  ret_packet.direction    = SP_INCOMING;
  ret_packet.sequence_num = sequence_num;
  ret_packet.token        = token;
  ret_packet.buffer       = pbuf;
  
  return ret_packet;
}

SP_PacketStatus sp_cmd_get_info (SP_PacketBuf* p_packet, uint16* msg_len) {
  // todo, accept a query type argument and simply reference the union  as .info and skip the beginning of this function

  uint8* msg_body = p_packet->buffer + SP_SIZEOF_PHEADER;

  SP_Cmd cmd = (SP_Cmd) msg_body[0];
  if (cmd != SP_GET_INFO) {
    return SP_ERR_CMD;
  }
  
  /* now construct the return packet */
  SP_GET_INFO_R* response = (SP_GET_INFO_R*)msg_body;
  response->cmd             = cmd;
  response->endianness      = SP_ENDIANNESS;
  response->total_ram       = SP_TOTAL_RAM;
  response->total_flash     = SP_TOTAL_FLASH;
  response->page_size       = SP_PAGE_SIZE;
  response->user_addr_flash = USER_CODE_FLASH;
  response->user_addr_ram   = USER_CODE_RAM;
  response->version         = VERSION_MIN + (VERSION_MAJ << 16);

  /* compute any endianness reversals needed */
  sp_reverse_bytes((u8*)&response->total_ram,       4);
  sp_reverse_bytes((u8*)&response->total_flash,     4);
  sp_reverse_bytes((u8*)&response->page_size,       2);
  sp_reverse_bytes((u8*)&response->user_addr_flash, 4);
  sp_reverse_bytes((u8*)&response->user_addr_ram,   4);
  sp_reverse_bytes((u8*)&response->version,         4);

  *msg_len = sizeof(SP_GET_INFO_R);
  return SP_OK;
  
}

SP_PacketStatus sp_cmd_erase_page   (SP_PacketBuf* p_packet, uint16* msg_len) {
  // todo, accept a query type argument and simply reference the union  as .info and skip the beginning of this function

  uint8* msg_body = p_packet->buffer + SP_SIZEOF_PHEADER;

  SP_Cmd cmd = (SP_Cmd) msg_body[0];
  if (cmd != SP_ERASE_PAGE) {
    return SP_ERR_CMD;
  }

  SP_ERASE_PAGE_Q* query = (SP_ERASE_PAGE_Q*)msg_body;
  sp_reverse_bytes((uint8*)(&query->addr),4);

  SP_ERASE_PAGE_R* response = (SP_ERASE_PAGE_R*)msg_body;

  // todo, check for valid address

  /* unlock the flash */
  flashUnlock();

  /* erase the page containing address */
  flashErasePage(query->addr);
  flashLock();

  /* todo, confirm erasure */
  if (*((u32*)(query->addr)) != 0) {
    response->success = SP_FAIL;
  } else {
    response->success = SP_SUCCESS;
  }

  *msg_len = sizeof(SP_ERASE_PAGE_R);
  return SP_OK;
    
}

SP_PacketStatus sp_cmd_write_bytes  (SP_PacketBuf* p_packet, uint16* msg_len) {
  uint8* msg_body = p_packet->buffer + SP_SIZEOF_PHEADER;

  SP_Cmd cmd = (SP_Cmd) msg_body[0];
  if (cmd != SP_WRITE_BYTES) {
    // shouldnt ever get here since we switched off this value in disptach
    return SP_ERR_CMD;
  }

  SP_WRITE_BYTES_Q* query = (SP_WRITE_BYTES_Q*)msg_body;
  sp_reverse_bytes((uint8*)(&query->addr),4);

  SP_WRITE_BYTES_R* response = (SP_WRITE_BYTES_R*)msg_body;

  // todo check address
  // todo check that the total size requested is in bounds

  uint8* payload     = &msg_body[5];
  uint16 payload_len = p_packet->total_len - SP_SIZEOF_PHEADER - SP_SIZEOF_PFOOTER;

  uint32* write_addr = (u32*)query->addr;
  bool    use_flash  = !sp_addr_in_ram(write_addr);

  if (use_flash) {flashUnlock();}
  
  uint8 malign = payload_len % 4;
  int i,j;
  for (i=0;i<payload_len-malign;i+=4) {
    uint32 this_word = sp_maligned_cast_u32(payload+i);
    if (use_flash) {
      flashWriteWord((uint32)(write_addr++),this_word); // todo potential endianness bug, be advised - also handle boolean return from write
    } else {
      *(write_addr++) = this_word;
    }
  }

  if (malign) {
    uint32 final_word;
    for (j=0;j<malign;j++) {
      final_word += (payload[i+j] << j*8);
    }
    
    if (use_flash) {
      flashWriteWord((uint32)write_addr,final_word);
    } else {
      *(write_addr) = final_word;
    }
  }

  // todo readback to verify success
  if (use_flash) {flashLock();}


  response->success = SP_SUCCESS;
  *msg_len = sizeof(SP_WRITE_BYTES_R);
  return SP_OK;

}

SP_PacketStatus sp_cmd_read_bytes   (SP_PacketBuf* p_packet, uint16* msg_len) {
  uint8* msg_body = p_packet->buffer + SP_SIZEOF_PHEADER;
  // or perhaps   = &p_packet->buffer[SP_SIZEOF_PHEADER];

  SP_Cmd cmd = (SP_Cmd) msg_body[0];
  if (cmd != SP_READ_BYTES) {
    /* shouldnt ever get here since we switched off this value in
       disptach */
    return SP_ERR_CMD;
  }

  SP_READ_BYTES_Q* query = (SP_READ_BYTES_Q*)msg_body;
  sp_reverse_bytes((uint8*)(&query->addr),4);
  sp_reverse_bytes((uint8*)(&query->len),2);

  /* no need to create a response since weve manually stuffed the only
     field (payload) */
  uint8*  read_addr = (uint8*)query->addr;
  uint16  read_len  = query->len;
  uint8*  payload   = &msg_body[7];

  /* we can only send back 1024 bytes at a time, so any greater len must be bounded */
  if (read_len > SP_MAX_READ_LEN) {
    read_len = SP_MAX_READ_LEN;
  }

  /* stuff the payload */
  int i;
  for (i=0;i<read_len;i++) {
    *payload++ = *read_addr++;
  }

  *msg_len = sizeof(SP_READ_BYTES_R) + read_len;
  
  return SP_OK;
}

SP_PacketStatus sp_cmd_jump_to_user (SP_PacketBuf* p_packet, uint16* msg_len) {
  SP_PacketStatus ret_status = SP_ERR_CMD;

  uint8* msg_body = p_packet->buffer + SP_SIZEOF_PHEADER;
  // or perhaps   = &p_packet->buffer[SP_SIZEOF_PHEADER];

  SP_Cmd cmd = (SP_Cmd) msg_body[0];
  if (cmd != SP_JUMP_TO_USER) {
    // shouldnt ever get here since we switched off this value in disptach
    return SP_ERR_CMD;
  }

  SP_JUMP_TO_USER_Q* query = (SP_JUMP_TO_USER_Q*)msg_body;

  if (query->user_region == SP_REGION_FLASH) {
    ret_status = SP_JUMP_FLASH;
  } else if (query->user_region == SP_REGION_RAM) {
    ret_status = SP_JUMP_RAM;
  }

  SP_JUMP_TO_USER_R* response = (SP_JUMP_TO_USER_R*)msg_body;
  if (ret_status != SP_ERR_CMD) {
    response->success = SP_SUCCESS;
  } else {
    response->success = SP_FAIL;
  }

  *msg_len = 1;
  return ret_status;
}

SP_PacketStatus sp_cmd_soft_reset   (SP_PacketBuf* p_packet, uint16* msg_len) {
  uint8* msg_body = p_packet->buffer + SP_SIZEOF_PHEADER;

  SP_Cmd cmd = (SP_Cmd) msg_body[0];
  if (cmd != SP_SOFT_RESET) {
    // shouldnt ever get here since we switched off this value in disptach
    return SP_ERR_CMD;
  }

  SP_SOFT_RESET_R* response = (SP_SOFT_RESET_R*)msg_body;
  response->success = SP_SUCCESS;
  *msg_len = 1;

  return SP_SYS_RESET;

}

void sp_setup_pbuf_out (SP_PacketBuf* p_packet, uint16 msg_len) {
  // todo, checks on overflow, ensure msg_len doesnt overflow the pbuffer!
  /* leave cmd, start, token, sequence_num unchanged */
  p_packet->total_len = msg_len + SP_SIZEOF_PHEADER + SP_SIZEOF_PFOOTER;
  p_packet->pindex     = 0;
  p_packet->direction = SP_OUTGOING;
 
  /* stuff the packet header in the buffer, only need to change msg_len! */
  SP_PHeader* header = (SP_PHeader*)p_packet->buffer;
  header->msg_len = msg_len;

  /* reverse any fields that need it before checksum */
  sp_reverse_bytes((uint8*)&header->msg_len,2);
  
  /* compute the checksum */
  uint32 checksum = sp_compute_checksum(p_packet->buffer,p_packet->total_len - SP_SIZEOF_PFOOTER);
  p_packet->checksum = checksum;

  /* stuff the checksum */
  uint8* p_checksum = &p_packet->buffer[SP_SIZEOF_PHEADER + msg_len];
  sp_maglined_copy_u32(checksum,p_checksum);
  // *(uint32*)p_checksum = checksum; // this may be a dangrous cast because of alignment
  

  /* and reverse it */
  sp_reverse_bytes(p_checksum,4);

}

// utility for dealing with endianness mismatch of certain fields
void sp_reverse_bytes(uint8* buf_in, uint16 len) {
  uint8 buf_tmp[len];
  int i;
  for (i=0;i<len;i++) {
    buf_tmp[i] = buf_in[len-1-i];
  }

  for (i=0;i<len;i++) {
    buf_in[i] = buf_tmp[i];
  }
}

uint32 sp_compute_checksum(uint8* buf_in, uint16 len) {
  /* take the xor of all 32 bit words in the buffer, in big endian
     order. But return the result in little endian */
  int i;
  uint8 this_word[4] = {0,0,0,0}; 
  for (i=0;i<len;i++) {
    this_word[i%4] ^= buf_in[i]; /* xor is bitwise, so we neednt actually decode the 4 byte words */
  }

  sp_reverse_bytes(this_word,4); // convert from big endian
  uint32 checksum = sp_maligned_cast_u32(this_word);
  
  return checksum;
}

bool sp_addr_in_ram(uint32* addr) {
  if ((uint32)addr > RAM_START && (uint32)addr < (RAM_START+TOTAL_RAM)) {
    return TRUE;
  }

  return FALSE;
}

uint32 sp_maligned_cast_u32(uint8* start) {
  /* perhaps this function is unecessary, but we can be certain this
     extra step will give us the proper little endian cast */
  uint32 ret = 0;
  int i;
  for (i=0;i<4;i++) {
    ret += *start << 8*i;
    start++;
  }
  return ret;
}

void sp_maglined_copy_u32(uint32 val, uint8* dest) {
  int i;
  for (i=0;i<4;i++) {
    *dest = ((val << (24-8*i)) >> 8*i);
  }
}
