#include "maple_dfu.h"

/* DFU globals */
u32 userAppAddr = 0x20000600; /* default RAM user code location */
DFUStatus dfuAppStatus;       /* includes state */
bool copyLock = FALSE;

u8 recvBuffer[wTransferSize];
u8 currentBlock = 0;
u32* userSpace = (u32*) USER_CODE_RAM;
u32 userFirmwareLen = 0;
u32* lastUserSpace = (u32*) USER_CODE_RAM;
u16 localBlockCount = 0;
bool code_copy_lock = FALSE;
u16 thisBlockLen = 0;;

/* todo: force dfu globals to be singleton to avoid re-inits? */
void dfuInit(void) {
  dfuAppStatus.bStatus = OK;
  dfuAppStatus.bwPollTimeout0 = 0x00;  /* todo: unionize the 3 bytes */
  dfuAppStatus.bwPollTimeout1 = 0x00;  /* todo: unionize the 3 bytes */
  dfuAppStatus.bwPollTimeout2 = 0x00;  /* todo: unionize the 3 bytes */
  dfuAppStatus.bState = appIDLE;
  dfuAppStatus.iString = 0x00;          /* all strings must be 0x00 until we make them! */
}

bool dfuUpdateByRequest(void) {
  /* were using the global pInformation struct from usb_lib here,
     see comment in maple_dfu.h around DFUEvent struct */

  if ((pInformation->USBbRequest == DFU_DNLOAD)
      && (pInformation->USBwValues.w == 0)) {
    userSpace = (u32*) USER_CODE_RAM;
  }
  if ((pInformation->USBbRequest == DFU_UPLOAD)
      && (pInformation->USBwValues.w == 0)) {
    userSpace = (u32*) USER_CODE_RAM;
  }

  
  u8 startState = dfuAppStatus.bState;
  /* often leaner to nest if's then embed a switch/case */
  if (startState == appIDLE)                      {
    /* device running outside of DFU Mode */

    if (pInformation->USBbRequest == DFU_DETACH) {
      dfuAppStatus.bState  = appDETACH;
      /* todo, start detach timer */
    } else if (pInformation->USBbRequest == DFU_GETSTATUS) {
      dfuAppStatus.bState  = appIDLE;
    } else if (pInformation->USBbRequest == DFU_GETSTATE) {
      dfuAppStatus.bState  = appIDLE;
    } else {
      dfuAppStatus.bState  = appIDLE;
      dfuAppStatus.bStatus = errSTALLEDPKT;      
    }

  } else if (startState == appDETACH)              {
    /* device has received DETACH, awaiting usb reset */
    
    /* todo, add check for timer timeout once supported */
    if (pInformation->USBbRequest == DFU_GETSTATUS) {
      dfuAppStatus.bState  = appDETACH;
    } else if (pInformation->USBbRequest == DFU_GETSTATE) {
      dfuAppStatus.bState  = appDETACH;
    } else {
      dfuAppStatus.bState  = appIDLE;
      dfuAppStatus.bStatus = errSTALLEDPKT;      
    }    

  } else if (startState == dfuIDLE)                {
    /*  device running inside DFU mode */

    if (pInformation->USBbRequest == DFU_DNLOAD) {
      if (pInformation->USBwLengths.w > 0) {
	userFirmwareLen = 0;
	dfuAppStatus.bState  = dfuDNLOAD_SYNC;
	dfuAppStatus.bwPollTimeout0 = 0xFF; /* throw in constant wait */
      } else {
	dfuAppStatus.bState  = dfuERROR;
	dfuAppStatus.bStatus = errNOTDONE;      
      }
    } else if (pInformation->USBbRequest == DFU_UPLOAD) {
      dfuAppStatus.bState  = dfuUPLOAD_IDLE;
    } else if (pInformation->USBbRequest == DFU_ABORT) {
      dfuAppStatus.bState  = dfuIDLE;
      dfuAppStatus.bStatus = OK;  /* are we really ok? we were just aborted */    
    } else if (pInformation->USBbRequest == DFU_GETSTATUS) {
      dfuAppStatus.bState  = dfuIDLE;
    } else if (pInformation->USBbRequest == DFU_GETSTATE) {
      dfuAppStatus.bState  = dfuIDLE;
    } else {
      dfuAppStatus.bState  = dfuERROR;
      dfuAppStatus.bStatus = errSTALLEDPKT;      
    }

  } else if (startState == dfuDNLOAD_SYNC)         {
    /* device received block, waiting for DFU_GETSTATUS request */

    if (pInformation->USBbRequest == DFU_GETSTATUS) {      
      /* todo, add routine to wait for last block write to finish */
	dfuAppStatus.bState  = dfuDNLOAD_IDLE;
	dfuAppStatus.bwPollTimeout0 = 0xFF;
	if (!code_copy_lock) {
	  dfuCopyBufferToExec();
	}
    } else if (pInformation->USBbRequest == DFU_GETSTATE) {
      dfuAppStatus.bState  = dfuDNLOAD_SYNC;
    } else {
      dfuAppStatus.bState  = dfuERROR;
      dfuAppStatus.bStatus = errSTALLEDPKT;      
    }

  } else if (startState == dfuDNBUSY)              {
    /* if were actually done writing, goto sync, else stay busy */
    dfuAppStatus.bState  = dfuDNLOAD_SYNC;

  } else if (startState == dfuDNLOAD_IDLE)         {
    /* device is expecting dfu_dnload requests */
    dfuAppStatus.bwPollTimeout0 = 0x00;
    if (pInformation->USBbRequest == DFU_DNLOAD) {
      if (pInformation->USBwLengths.w > 0) {
	dfuAppStatus.bState  = dfuDNLOAD_SYNC;
	thisBlockLen = pInformation->USBwLengths.bw.bb0*0x100;
	thisBlockLen += pInformation->USBwLengths.bw.bb1;
      } else {
	/* todo, support "disagreement" if device expects more data than this */
	dfuAppStatus.bState  = dfuMANIFEST_SYNC;
      }      
    } else if (pInformation->USBbRequest == DFU_ABORT) {
      dfuAppStatus.bState  = dfuIDLE;
    } else if (pInformation->USBbRequest == DFU_GETSTATUS) {
      dfuAppStatus.bState  = dfuIDLE;
    } else if (pInformation->USBbRequest == DFU_GETSTATE) {
      dfuAppStatus.bState  = dfuIDLE;
    } else {
      dfuAppStatus.bState  = dfuERROR;
      dfuAppStatus.bStatus = errSTALLEDPKT;      
    }

  } else if (startState == dfuMANIFEST_SYNC)       {
    /* device has received last block, waiting DFU_GETSTATUS request */
    
    if (pInformation->USBbRequest == DFU_GETSTATUS) {
      /* for now we have no manifestation, so jump straight to end! */
#if 0
      if (checkUserCode(USER_CODE_RAM)) {
	  dfuAppStatus.bState  = appIDLE;
	  usbConfigDescriptor.Descriptor[16] = 0x01;
	  currentBlock=0;

	  strobePin(GPIOA,5,3,0x40000);
	  jumpToUser(USER_CODE_RAM);
      } else {
	dfuAppStatus.bState  = dfuERROR;
	dfuAppStatus.bStatus = errFIRMWARE;
	strobePin(GPIOA,5,3,0xA0000);
      }
#else
      strobeCode(GPIOA,5,recvBuffer[0]);
      strobeCode(GPIOA,5,recvBuffer[1]);
      strobeCode(GPIOA,5,recvBuffer[2]);
      strobeCode(GPIOA,5,recvBuffer[3]);

      
      u8* usrPtr = (u8*)USER_CODE_RAM;
      int i;
      for (i=0;i<8;i++) {
	*usrPtr++ = recvBuffer[i];
      }
      u32 firstVal = *(u32*) USER_CODE_RAM;

      strobeCode(GPIOA,5, firstVal                >> 24);
      strobeCode(GPIOA,5, (firstVal & 0x00FF0000) >> 16);
      strobeCode(GPIOA,5, (firstVal & 0x0000FF00) >> 8);
      strobeCode(GPIOA,5, firstVal  & 0x000000FF);

      if (firstVal == 0x20005000) {
	strobePin(GPIOA,5,100,0x30000);
      }

      usrPtr = (u8*)USER_CODE_RAM;
      for (i=0;i<4;i++) {
	*usrPtr++ = 2*i+((2*i+1)<<4);
      }
      firstVal = *(u32*) USER_CODE_RAM;

      strobeCode(GPIOA,5, firstVal                >> 24);
      strobeCode(GPIOA,5, (firstVal & 0x00FF0000) >> 16);
      strobeCode(GPIOA,5, (firstVal & 0x0000FF00) >> 8);
      strobeCode(GPIOA,5, firstVal  & 0x000000FF);

      dfuAppStatus.bState = dfuIDLE;
#endif
    } else if (pInformation->USBbRequest == DFU_GETSTATE) {
      dfuAppStatus.bState  = dfuMANIFEST_SYNC;
    } else {
      dfuAppStatus.bState  = dfuERROR;
      dfuAppStatus.bStatus = errSTALLEDPKT;            
    }

  } else if (startState == dfuMANIFEST)            {
    /* device is in manifestation phase */

    /* should never receive request while in manifest! */
    dfuAppStatus.bState  = dfuERROR;
    dfuAppStatus.bStatus = errSTALLEDPKT;
                
  } else if (startState == dfuMANIFEST_WAIT_RESET) {
    /* device has programmed new firmware but needs external
       usb reset or power on reset to run the new code */

    /* consider timing out and self-resetting */
    dfuAppStatus.bState  = dfuMANIFEST_WAIT_RESET;

  } else if (startState == dfuUPLOAD_IDLE)         {
    /* device expecting further dfu_upload requests */

    if (pInformation->USBbRequest == DFU_UPLOAD) {      
      /* todo, add routine to wait for last block write to finish */
      dfuAppStatus.bState  = dfuERROR;
      dfuAppStatus.bStatus = errSTALLEDPKT; 
    } else if (pInformation->USBbRequest == DFU_ABORT) {
      currentBlock=0;
      dfuAppStatus.bState  = dfuIDLE;
    } else if (pInformation->USBbRequest == DFU_GETSTATUS) {
      dfuAppStatus.bState  = dfuUPLOAD_IDLE;
    } else if (pInformation->USBbRequest == DFU_GETSTATE) {
      dfuAppStatus.bState  = dfuUPLOAD_IDLE;
    } else {
      dfuAppStatus.bState  = dfuERROR;
      dfuAppStatus.bStatus = errSTALLEDPKT;      
    }
    

  } else if (startState == dfuERROR)               {
    /* status is in error, awaiting DFU_CLRSTATUS request */

    if (pInformation->USBbRequest == DFU_GETSTATUS) {      
      /* todo, add routine to wait for last block write to finish */
      dfuAppStatus.bState  = dfuERROR;
    } else if (pInformation->USBbRequest == DFU_GETSTATE) {
      dfuAppStatus.bState  = dfuERROR;
    } else if (pInformation->USBbRequest == DFU_CLRSTATUS) {
      /* todo handle any cleanup we need here */
      dfuAppStatus.bState  = dfuIDLE;
      dfuAppStatus.bStatus = OK;      
    } else {
      dfuAppStatus.bState  = dfuERROR;
      dfuAppStatus.bStatus = errSTALLEDPKT;      
    }    

  } else {
    /* some kind of error... */
    dfuAppStatus.bState  = dfuERROR;
    dfuAppStatus.bStatus = errSTALLEDPKT;      
  }

  if (dfuAppStatus.bStatus == OK) {
    return TRUE;
  } else {
    return FALSE;
  }
}

void dfuUpdateByReset(void) {
  u8 startState = dfuAppStatus.bState;
  userSpace = (u32*) USER_CODE_RAM;

  if (startState == appIDLE) {
    dfuAppStatus.bStatus  = OK;
  } else if (startState == appDETACH) {
    dfuAppStatus.bState  = dfuIDLE;
    usbConfigDescriptor.Descriptor[16] = 0x02;
  } else if (startState == dfuIDLE) {
    dfuAppStatus.bState  = dfuIDLE;
    //    usbConfigDescriptor.Descriptor[16] = 0x01;
  } else if (startState == dfuDNLOAD_SYNC) {
    dfuAppStatus.bState  = appIDLE;
    usbConfigDescriptor.Descriptor[16] = 0x01;
  } else if (startState == dfuDNBUSY) {
    dfuAppStatus.bState  = dfuERROR;
    dfuAppStatus.bStatus = errSTALLEDPKT;      
  } else if (startState == dfuDNLOAD_IDLE) {
    dfuAppStatus.bState  = appIDLE;
    usbConfigDescriptor.Descriptor[16] = 0x01;
  } else if (startState == dfuMANIFEST_SYNC) {
    dfuAppStatus.bState  = appIDLE;
    usbConfigDescriptor.Descriptor[16] = 0x01;
  } else if (startState == dfuMANIFEST) {
    dfuAppStatus.bState  = appIDLE;
    usbConfigDescriptor.Descriptor[16] = 0x01;
  } else if (startState == dfuMANIFEST_WAIT_RESET) {
    dfuAppStatus.bState  = appIDLE;
    usbConfigDescriptor.Descriptor[16] = 0x01;
  } else if (startState == dfuUPLOAD_IDLE) {
    dfuAppStatus.bState  = appIDLE;
    usbConfigDescriptor.Descriptor[16] = 0x01;
  } else if (startState == dfuERROR) {
    dfuAppStatus.bState  = appIDLE;
    usbConfigDescriptor.Descriptor[16] = 0x01;
    dfuAppStatus.bStatus  = OK;
  }
}

void dfuUpdateByTimeout(void) {
}

u8* dfuCopyState(u16 length) {
  if (length == 0) {
    pInformation->Ctrl_Info.Usb_wLength=1;
    return NULL;
  } else {
    return (&(dfuAppStatus.bState));
  }
}

u8* dfuCopyStatus(u16 length) {
  if (length == 0) {
    pInformation->Ctrl_Info.Usb_wLength = 6;
    return NULL;
  } else {
    return(&dfuAppStatus);
  }
}


u8* dfuCopyDNLOAD(u16 length) {
  if (length==0) {
    pInformation->Ctrl_Info.Usb_wLength = pInformation->USBwLengths.w - pInformation->Ctrl_Info.Usb_wOffset;
    return NULL;
  } else {
    return ((u8*)recvBuffer + pInformation->Ctrl_Info.Usb_wOffset);
  }
}

u8* dfuCopyUPLOAD(u16 length) {
  u32 frameLen = (u32) userFirmwareLen - ((u32) userSpace - (u32) USER_CODE_RAM);
  if (length == 0) {
    if (frameLen > wTransferSize) {
      pInformation->Ctrl_Info.Usb_wLength = wTransferSize - pInformation->Ctrl_Info.Usb_wOffset;
      lastUserSpace = userSpace;
      userSpace = (u32*)(userSpace + pInformation->Ctrl_Info.Usb_wLength);
    } else {
      pInformation->Ctrl_Info.Usb_wLength = frameLen - pInformation->Ctrl_Info.Usb_wOffset;
      userSpace = (u32*)(userSpace + pInformation->Ctrl_Info.Usb_wLength);
    }
    return NULL;
  } else {
    return ((u8*)(lastUserSpace + pInformation->Ctrl_Info.Usb_wOffset));
  }
}

void dfuCopyBufferToExec() {
  code_copy_lock = TRUE;
  int i;
  u32 thisVal;
  for (i=0;i<thisBlockLen;i++) {
    *userSpace++ = *(u32*)recvBuffer[i];
  }
  userFirmwareLen += thisBlockLen;

  
  localBlockCount++;
  thisBlockLen = 0;
  code_copy_lock = FALSE;
}

bool checkTestFile(void) {
  int i;
  u32* usrPtr = (u32*)USER_CODE_RAM;

  if (*usrPtr == 0x20005000) {
    return TRUE;
  } else {
    return FALSE;
  }
}
/* DFUState dfuGetState(); */
/* void dfuSetState(DFUState); */
