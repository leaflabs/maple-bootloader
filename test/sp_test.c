/*
  Basic test utility to implement a modified version of the serial
  bootloader command line protocol and verify it is working

  should not be run as a standalone program, instead it should be
  spawned by mapledude.py

*/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "sp_test.h"

/* socket related stuff */
#include <sys/socket.h> /* for socket() and socket functions*/
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <string.h>     /* for memset() */


#define LOG_FILE "test_log.txt"
#define ERR_DUMP test_log

#define S_REPORT_F(s, fmt, args...) fprintf(s,"%04d:%-*s "fmt"\n",__LINE__,25,__func__,args)
#define S_REPORT(s, str)            fprintf(s,"%04d:%-*s %s\n", __LINE__,25,__func__,str)

//#define REPORT_F(fmt, args...) S_REPORT_F(stderr, fmt, args); S_REPORT_F(test_log, fmt, args)
//#define REPORT(fmt) S_REPORT(stderr, fmt); S_REPORT(test_log, fmt)

#define REPORT_F(fmt, args...) S_REPORT_F(ERR_DUMP, fmt, args)
#define REPORT(fmt) S_REPORT(ERR_DUMP, fmt)

FILE *test_log;

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define PORT       5323
int servSock;                    /* Socket descriptor for server */
int clntSock;                    /* Socket descriptor for client */
struct sockaddr_in echoServAddr; /* Local address */
struct sockaddr_in echoClntAddr; /* Client address */
unsigned short echoServPort;     /* Server port */
unsigned int clntLen;            /* Length of client address data structure */

static void term_trap(int sig) {
  //  REPORT_F("Trapped UNIX Signal 0x%X",sig);
  if (test_log) {
    fclose(test_log);
  }
}

#define SIG(x) signal(x,&term_trap)
int main() 
{ 
  test_log = fopen(LOG_FILE,"w");
  SIG(SIGHUP);
  SIG(SIGINT);
  SIG(SIGQUIT);
  SIG(SIGABRT);
  SIG(SIGTERM);

  REPORT("Maple Bootloader Test Utility");
  REPORT("Running sp_run(0)");

  /* setup the socket */
  tcp_init_server();

  while (1) {
    tcp_get_client();

    SP_PacketStatus status = sp_run(0); // will block until successfull JUMP_TO_USER,SOFT_RESET,or EXIT
    REPORT("CMD Interface Finished...");

    char* status_str;
    switch (status) {
    case SP_ERR_TIMEOUT:
      status_str = "TIMEOUT";
      break;
    case SP_ERR_START:
      status_str = "ERR_START";
      break;
    case SP_EXIT:
      status_str = "EXIT";
      break;
    case SP_JUMP_RAM:
      status_str = "JUMP_RAM";
      break;
    case SP_JUMP_FLASH:
      status_str = "JUMP_FLASH";
      break;
    case SP_SYS_RESET:
      status_str = "SYS_RESET";
      break;

    default: 
      status_str = "Unexpected return val";
      break;
    }
  
    REPORT_F("CMD Interface exit status:  %s",status_str);

    close(clntSock);    /* Close client socket */
  }

  return 0; 
}


SP_PacketStatus sp_run(int delay) {
  uint8 sp_buffer[SP_MAX_MSG_LEN+SP_SIZEOF_PHEADER+SP_SIZEOF_PFOOTER];

  SP_PacketStatus status;
  bool done = FALSE;
  while (!done) {
    REPORT("Cmd Idle");

    // since packets are processed atomically, we need only one function
    status = sp_handle_packet(delay, &sp_buffer[0], sizeof(sp_buffer)); 
    REPORT("Dispatch Finished");

    if (status == SP_ERR_TIMEOUT || 
        status == SP_EXIT        || 
        //        status == SP_ERR_START   ||
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
      usbReceiveBytes(pbuf,1);
      
      /* check the first byte before ANYTHING so we can jump out
         quickly if theres no bootloader traffic */
      if (pbuf[0] != SP_START) {
        if (WAIT_UNTIL_START) {
          if (!permanent) {
            delay--;
          }
          continue;
        } else {
          REPORT_F("Incorrect START field: expected 0x%X, got 0x%X",SP_START,pbuf[0]);
          return SP_ERR_START;
        }
      }

      REPORT("Receiving remaining header bytes");
      /* get the rest of the header */
      usbReceiveBytes(pbuf+1,SP_SIZEOF_PHEADER-1);
      SP_PacketBuf this_packet = sp_create_pbuf_in(pbuf,pbuf_len);
      
      /* stk500 doesnt check sequence nums on incoming packets so skip that */


      /* check the token  */
      if (this_packet.token != SP_TOKEN) {
        REPORT_F("Incorrect Token field: expected 0x%X, got 0x%X",SP_TOKEN,this_packet.token);
        return SP_ERR_TOKEN;
      }
      
      SP_PacketStatus status = sp_get_packet(&this_packet); // todo perhaps generate a unique status type for this level 
      if (status != SP_OK) {
        /* perhaps handle with cleanup or more care, this is really passing the buck. */
        REPORT_F("sp_get_packet not OK, return with status: 0x%X",status);
        sp_debug_dump_packet(&this_packet);
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
      REPORT("Sending Reply Packet");
      status = sp_marshall_reply(&this_packet); // reset delay to give a fresh tx timeout
      if (status != SP_OK) {
        REPORT_F("Error sending reply packet, status = 0x%X",status);
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

  REPORT_F("Waiting to receive packet body, 0x%X bytes",(p_packet->total_len-p_packet->pindex));
  while (p_packet->pindex != p_packet->total_len) {    
    uint8 bytes_in = usbReceiveBytes(&p_packet->buffer[p_packet->pindex],p_packet->total_len-p_packet->pindex);
    if (!bytes_in) {
      if (timeout-- == 0) {
        // Ooops...
        REPORT_F("TIMED OUT while getting packet body, got 0x%X bytes total",(p_packet->pindex - SP_SIZEOF_PHEADER));
        return SP_ERR_TIMEOUT;
      }
    } else {
      p_packet->pindex += bytes_in;
    }
  }

  // reverse the checksum from big endian
  uint8* p_checksum = p_packet->buffer+p_packet->pindex-SP_LEN_CHECKSUM;
  uint32 checksum = sp_maligned_cast_u32(p_checksum);
  sp_reverse_bytes((uint8*)&checksum,4);
  p_packet->checksum = checksum;

  /* finally, print the buffer */
  REPORT("Packet Buffer Dump (post endian swap):");
  REPORT_F("\t\tLen: %i",p_packet->total_len);
  char buf_str[1024]; // lets set up our stupid little program for buffer overflow vulnerability!
  int  offset = 0;
  int  i;
  buf_str[offset++] = '\t';
  buf_str[offset++] = '\t';
  buf_str[offset++] = '[';
  for (i=0;i<p_packet->total_len;i++) {
    offset += sprintf(buf_str+offset,"0x%X, ",p_packet->buffer[i]);
  }
  offset -= 2;
  buf_str[offset++] = ']';
  buf_str[offset++] = 0;
  REPORT(buf_str);



  return sp_check_sum(p_packet);
}

SP_PacketStatus sp_marshall_reply(SP_PacketBuf* p_packet) {
  /* loops over trying to send the response, can timeout */
  int timeout = SP_BYTE_TIMEOUT;
  while (p_packet->pindex < p_packet->total_len) {
    uint8 bytes_out = usbSendBytes(&p_packet->buffer[p_packet->pindex],p_packet->total_len - p_packet->pindex);
    if (!bytes_out) {
      if (timeout-- == 0) {
        REPORT_F("TIMED OUT while sending response packet, sent 0x%X bytes out of 0x%X",p_packet->pindex,p_packet->total_len);
        // side effects not undone, but things like user_jump or soft_reset will not execute now
        return SP_ERR_TIMEOUT;
      }
    } else {
      p_packet->pindex += bytes_out;
    }
  }

  REPORT("Packet Buffer Dump (outgoing):");
  REPORT_F("\t\tLen: %i",p_packet->total_len);
  char buf_str[1024]; // lets set up our stupid little program for buffer overflow vulnerability!
  int  offset = 0;
  int  i;
  buf_str[offset++] = '\t';
  buf_str[offset++] = '\t';
  buf_str[offset++] = '[';
  for (i=0;i<p_packet->total_len;i++) {
    offset += sprintf(buf_str+offset,"0x%X, ",p_packet->buffer[i]);
  }
  offset -= 2;
  buf_str[offset++] = ']';
  buf_str[offset++] = 0;
  REPORT(buf_str);

  return SP_OK;
  
}


SP_PacketStatus sp_check_sum(SP_PacketBuf* p_packet) {
  /* checksum is the XOR of all 4 byte words. Interpret these words in
     network order (big endian). Pad if necessary. Although, since everything is big endian, 
     padding will have no effect on the resultant word value, so you can really do nothing */

  uint16 total_len = p_packet->total_len - SP_SIZEOF_PFOOTER;
  uint32 checksum = sp_compute_checksum(p_packet->buffer,total_len);

  if (checksum != p_packet->checksum) {
    REPORT_F("Checksum FAIL! got 0x%X, but expected 0x%X, len bytes %i",checksum,p_packet->checksum,total_len);
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
  uint16 msg_len = (pbuf[2] << 8) + pbuf[3];
  uint8  sequence_num = pbuf[1];
  uint8  token        = pbuf[4];

  SP_PacketBuf ret_packet;
  ret_packet.total_len    = msg_len + SP_SIZEOF_PHEADER+SP_SIZEOF_PFOOTER;
  ret_packet.pindex       = SP_SIZEOF_PHEADER; // index of the NEXT byte to be filled
  ret_packet.direction    = SP_INCOMING;
  ret_packet.sequence_num = sequence_num;
  ret_packet.token        = token;
  ret_packet.buffer       = pbuf;
  
  REPORT("Created new SP_PacketBuf:");
  REPORT_F("\t\ttotal_len:    0x%X",ret_packet.total_len);
  REPORT_F("\t\tpindex:       0x%X",ret_packet.pindex);
  REPORT_F("\t\tdirection:    0x%X",ret_packet.direction);
  REPORT_F("\t\tsequence_num: 0x%X",ret_packet.sequence_num);
  REPORT_F("\t\ttoken:        0x%X",ret_packet.token);
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
  *msg_len = sizeof(SP_ERASE_PAGE_R);

  // todo, better check for valid address
  if ((query->addr) % 4 != 0) {
    response->success = SP_FAIL;
    return SP_OK;
  }

  /* unlock the flash */
  flashUnlock();

  /* erase the page containing address */
  flashErasePage(query->addr);
  flashLock();

  /* todo, confirm erasure */
  if (*((uint8*)query->addr) != 0) {
    response->success = SP_FAIL;
  } else {
    response->success = SP_SUCCESS;
  }

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
      // *(write_addr++) = this_word; do nothing for the test program
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
  sp_reverse_bytes((uint8*)&header->msg_len,2);

  /* compute the checksum */
  uint32 checksum = sp_compute_checksum(p_packet->buffer,p_packet->total_len - SP_SIZEOF_PFOOTER);
  p_packet->checksum = checksum;

  /* stuff the checksum */
  uint8* p_checksum = &p_packet->buffer[SP_SIZEOF_PHEADER + msg_len];
  sp_maligned_copy_u32(checksum,p_checksum);
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
  uint32 checksum=0;
  uint32 this_word=0;
  int i;
  int shift = 24;
  for (i=0;i<len;i++) {
    shift = 24-8*(i%4);
    this_word |= (buf_in[i]  << shift);

    if (shift == 0) {
      checksum ^= this_word;
      this_word = 0;
    }
  }

  if ((len%4) != 0) {
    checksum ^= this_word;
  }
  REPORT_F("computed checksum: %x", checksum);
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

void sp_maligned_copy_u32(uint32 val, uint8* dest) {
  int i;
  for (i=0;i<4;i++) {
    uint32 mask = 0xFF << 8*i;
    uint32 mask_word = (val&mask) >> 8*i;
    *dest++ = (uint8)mask_word; 
  }
}

/* the necessary mechanics for debugging out of uart2 */
//void sp_debug_
void sp_debug_blink(int i) {
}

void sp_debug_dump_packet(SP_PacketBuf* p_packet) {
  uint8 buflen = (p_packet->total_len)*8;
  uint8 print_buf[buflen];

  int i;
  uint8 offset=0;
  print_buf[offset++] = '[';
  for (i=0;i<p_packet->total_len;i++) {
    if (offset > buflen-5) {
      break; // stop overflow
    }
    offset += sprintf(&print_buf[offset],"0x%X, ", p_packet->buffer[i]);
  }
  offset -= 2;
  print_buf[offset++] = ']';

  REPORT_F("Packet Dump:\n\t\tGot %i bytes\n\t\t%s",p_packet->total_len,&print_buf[0]);
}

/* functions that were provided by other portions of the bootloader */
uint16 usbSendBytes(uint8* sendBuf,uint16 len) {
  if (send(clntSock, sendBuf, len, 0) != len)
    DieWithError("send() failed");
    
  return len;
}

uint8 usbBytesAvailable(void) {
  return 255;
}

uint8 usbReceiveBytes(uint8* recvBuf, uint8 len) {
  int bytesIn = 0;                    /* Size of received message */

  /* Receive message from client */
  while (bytesIn < len) {
    if ((bytesIn += recv(clntSock, &recvBuf[bytesIn], len-bytesIn, 0)) <= 0)
      DieWithError("recv() failed, connection lost");
  }

  return bytesIn;
}


void systemHardReset(void) {
  REPORT("System Hard Reset executed");
}

bool checkUserCode (u32 usrAddr) {
  REPORT("Checking User Code at addr:");
  fprintf(test_log,"\t\t0x%X\n",usrAddr);
}

void jumpToUser    (u32 usrAddr) {
  REPORT("Jump to User executed at addr:");
  REPORT_F("\t\t0x%X\n",usrAddr);
}

bool flashWriteWord  (u32 addr, u32 word) {
  REPORT_F("\t\tWrote: 0x%X -> 0x%X\n",addr,word);
}

bool flashErasePage  (u32 addr) {
  REPORT_F("Erased Page Containing 0x%X\n",addr);
}

bool flashErasePages (u32 addr, u16 n){
  REPORT_F("Erased %i Pages Starting at 0x%X\n",n,addr);
}

void flashLock       (void) {
}

void flashUnlock     (void) {
}

void tcp_init_server() {
  echoServPort = PORT;

  /* Create socket for incoming connections */
  if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithError("socket() failed");
      
  /* Construct local address structure */
  memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
  echoServAddr.sin_family = AF_INET;                /* Internet address family */
  echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
  echoServAddr.sin_port = htons(echoServPort);      /* Local port */

  /* Bind to the local address */
  if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
    DieWithError("bind() failed");

  /* Mark the socket so it will listen for incoming connections */
  if (listen(servSock, MAXPENDING) < 0)
    {
      DieWithError("listen() failed");
    }
}

void tcp_get_client() {
  /* Set the size of the in-out parameter */
  clntLen = sizeof(echoClntAddr);

  /* Wait for a client to connect */
  if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0)
    DieWithError("accept() failed");

  /* clntSock is connected to a client! */
  REPORT_F("Connected over TCP to %s", inet_ntoa(echoClntAddr.sin_addr));
}

void DieWithError(char *errorMessage)
{
  REPORT(errorMessage);
  perror(errorMessage);
  exit(1);
}
