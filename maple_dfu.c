#include "maple_dfu.h"

/* DFU globals */
u32 userAppAddr = 0x20000C00; /* default RAM user code location */
DFUStatus dfuAppStatus;       /* includes state */
bool userFlash = FALSE;

u8 recvBuffer[wTransferSize];
u32 userFirmwareLen = 0;
u16 thisBlockLen = 0;


PLOT code_copy_lock;

/* todo: force dfu globals to be singleton to avoid re-inits? */
void dfuInit(void) {
  dfuAppStatus.bStatus = OK;
  dfuAppStatus.bwPollTimeout0 = 0x00;  /* todo: unionize the 3 bytes */
  dfuAppStatus.bwPollTimeout1 = 0x00;  /* todo: unionize the 3 bytes */
  dfuAppStatus.bwPollTimeout2 = 0x00;  /* todo: unionize the 3 bytes */
  dfuAppStatus.bState = appIDLE;
  dfuAppStatus.iString = 0x00;          /* all strings must be 0x00 until we make them! */
  userFirmwareLen = 0;
  thisBlockLen = 0;;
  userAppAddr = 0x20000C00; /* default RAM user code location */
  userFlash = FALSE;
  code_copy_lock = WAIT;
}

bool dfuUpdateByRequest(void) {
  /* were using the global pInformation struct from usb_lib here,
     see comment in maple_dfu.h around DFUEvent struct */

  u8 startState = dfuAppStatus.bState;
  dfuAppStatus.bStatus = OK;      
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

	if (pInformation->Current_AlternateSetting == 1) {
	  userAppAddr = USER_CODE_FLASH;
	  userFlash = TRUE;
	  
	  /* make sure the flash is setup properly, unlock it */
	  setupFLASH();
	  flashUnlock();
	  //	  flashErasePage((u32)USER_CODE_FLASH);

	} else {
	  userAppAddr = USER_CODE_RAM;
	  userFlash = FALSE;
	}
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
      if (userFlash) {
	if (code_copy_lock==WAIT) {
	  code_copy_lock=BEGINNING;
 	  dfuAppStatus.bwPollTimeout0 = 0xFF; /* is this enough? */
 	  dfuAppStatus.bwPollTimeout1 = 0x01; /* is this enough? */
	  dfuAppStatus.bState=dfuDNBUSY;

	} else if (code_copy_lock==BEGINNING) {
	  dfuAppStatus.bState=dfuDNLOAD_SYNC;	  

	} else if (code_copy_lock==MIDDLE) {
	  dfuAppStatus.bState=dfuDNLOAD_SYNC;

	} else if (code_copy_lock==END) {
 	  dfuAppStatus.bwPollTimeout0 = 0x00;
	  code_copy_lock=WAIT;
	  dfuAppStatus.bState=dfuDNLOAD_IDLE;
	}

      } else {
	dfuAppStatus.bState = dfuDNLOAD_IDLE;
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
    if (code_copy_lock == END) {
      dfuAppStatus.bwPollTimeout0 = 0x00;
      code_copy_lock=WAIT;
      dfuAppStatus.bState = dfuDNLOAD_IDLE;
    } else {
      dfuAppStatus.bState= dfuDNBUSY;
    }

  } else if (startState == dfuDNLOAD_IDLE)         {
    /* device is expecting dfu_dnload requests */
    if (pInformation->USBbRequest == DFU_DNLOAD) {
      if (pInformation->USBwLengths.w > 0) {
	dfuAppStatus.bState  = dfuDNLOAD_SYNC;
      } else {
	/* todo, support "disagreement" if device expects more data than this */
	dfuAppStatus.bState  = dfuMANIFEST_SYNC;
	
	/* relock the flash */
	flashLock();
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
/*       if (userFlash) { */
/* 	if (checkUserCode(USER_CODE_FLASH)) { */
/* 	  dfuAppStatus.bState  = dfuMANIFEST_WAIT_RESET; */
/* 	} else { */
/* 	  dfuAppStatus.bState  = dfuERROR; */
/* 	  dfuAppStatus.bStatus = OK; */
/* 	} */
/*       } else { */
/* 	if (checkUserCode(USER_CODE_RAM)) { */
/* 	  dfuAppStatus.bState  = dfuMANIFEST_WAIT_RESET; */
/* 	} else { */
/* 	  dfuAppStatus.bState  = dfuERROR; */
/* 	  dfuAppStatus.bStatus = OK; */
/* 	} */
/*       } */

      dfuAppStatus.bState  = dfuMANIFEST_WAIT_RESET;
      dfuAppStatus.bStatus = OK;
    } else if (pInformation->USBbRequest == DFU_GETSTATE) {
      dfuAppStatus.bState  = dfuMANIFEST_SYNC;
    } else {
      dfuAppStatus.bState  = dfuERROR;
      dfuAppStatus.bStatus = errSTALLEDPKT;            
    }

  } else if (startState == dfuMANIFEST)            {
    /* device is in manifestation phase */

    /* should never receive request while in manifest! */
    dfuAppStatus.bState  = dfuMANIFEST_WAIT_RESET;
    dfuAppStatus.bStatus = OK;

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
  userFirmwareLen = 0;

  if (startState == appDETACH) {
    dfuAppStatus.bState = dfuIDLE;
    dfuAppStatus.bStatus = OK;


#if COMM_ENB
#else
    usbConfigDescriptorDFU.Descriptor[0x10] = 0x02;
    usbConfigDescriptorDFU.Descriptor[0x19] = 0x02; 
#endif

  } else if (startState == appIDLE || startState == dfuIDLE) {
    /* do nothing...might be normal usb bus activity */
  } else {
    /* we reset from the dfu, reset everything and startover,
       which is the correct operation if this is an erroneous 
       event or properly following a MANIFEST */
    dfuAppStatus.bState = appIDLE;
    dfuAppStatus.bStatus = OK;

#if COMM_ENB
#else
    usbConfigDescriptorDFU.Descriptor[0x10] = 0x01;
    usbConfigDescriptorDFU.Descriptor[0x19] = 0x01; 
#endif

    systemHardReset();
  }

/*   if (startState == appIDLE) { */
/*     dfuAppStatus.bStatus  = OK; */
/*   } else if (startState == appDETACH) { */
/*     dfuAppStatus.bState  = dfuIDLE; */
/*     usbConfigDescriptor.Descriptor[0x10] = 0x02; */
/*     usbConfigDescriptor.Descriptor[0x19] = 0x02; */
/*   } else if (startState == dfuIDLE) { */
/*     dfuAppStatus.bState  = dfuIDLE; */
/*   } else if (startState == dfuDNLOAD_SYNC) { */
/*     dfuAppStatus.bState  = appIDLE; */
/*     usbConfigDescriptor.Descriptor[0x10] = 0x01; */
/*     usbConfigDescriptor.Descriptor[0x19] = 0x01; */
/*   } else if (startState == dfuDNBUSY) { */
/*     dfuAppStatus.bState  = dfuERROR; */
/*     dfuAppStatus.bStatus = errSTALLEDPKT;       */
/*   } else if (startState == dfuDNLOAD_IDLE) { */
/*     dfuAppStatus.bState  = appIDLE; */
/*     usbConfigDescriptor.Descriptor[0x10] = 0x01; */
/*     usbConfigDescriptor.Descriptor[0x19] = 0x01; */
/*   } else if (startState == dfuMANIFEST_SYNC) { */
/*     dfuAppStatus.bState  = appIDLE; */
/*     usbConfigDescriptor.Descriptor[0x10] = 0x01; */
/*     usbConfigDescriptor.Descriptor[0x19] = 0x01; */

/*     /\* hard reset the chip *\/ */
/*     systemHardReset(); */

/*   } else if (startState == dfuMANIFEST) { */
/*     dfuAppStatus.bState  = appIDLE; */
/*     usbConfigDescriptor.Descriptor[0x10] = 0x01; */
/*     usbConfigDescriptor.Descriptor[0x19] = 0x01; */

/*     /\* hard reset the chip *\/ */
/*     systemHardReset(); */

/*   } else if (startState == dfuMANIFEST_WAIT_RESET) { */
    
/*     dfuAppStatus.bState  = appIDLE; */
/*     usbConfigDescriptor.Descriptor[0x10] = 0x01; */
/*     usbConfigDescriptor.Descriptor[0x19] = 0x01; */

/*     /\* hard reset the chip *\/ */
/*     systemHardReset(); */

/*   } else if (startState == dfuUPLOAD_IDLE) { */
/*     dfuAppStatus.bState  = appIDLE; */
/*     usbConfigDescriptor.Descriptor[0x10] = 0x01; */
/*     usbConfigDescriptor.Descriptor[0x19] = 0x01; */
/*   } else if (startState == dfuERROR) { */
/*     dfuAppStatus.bState  = appIDLE; */
/*     usbConfigDescriptor.Descriptor[0x10] = 0x01; */
/*     usbConfigDescriptor.Descriptor[0x19] = 0x01; */
/*     dfuAppStatus.bStatus  = OK; */
/*   } */
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
    thisBlockLen = pInformation->USBwLengths.w;
    return NULL;
  } else {
    return ((u8*)recvBuffer + pInformation->Ctrl_Info.Usb_wOffset);
  }
}

u8* dfuCopyUPLOAD(u16 length) {
/*   u32 frameLen = (u32) userFirmwareLen - ((u32) userSpace - (u32) USER_CODE_RAM); */
/*   if (length == 0) { */
/*     if (frameLen > wTransferSize) { */
/*       pInformation->Ctrl_Info.Usb_wLength = wTransferSize - pInformation->Ctrl_Info.Usb_wOffset; */
/*       lastUserSpace = userSpace; */
/*       userSpace = (u32*)(userSpace + pInformation->Ctrl_Info.Usb_wLength); */
/*     } else { */
/*       pInformation->Ctrl_Info.Usb_wLength = frameLen - pInformation->Ctrl_Info.Usb_wOffset; */
/*       userSpace = (u32*)(userSpace + pInformation->Ctrl_Info.Usb_wLength); */
/*     } */
/*     return NULL; */
/*   } else { */
/*     return ((u8*)(lastUserSpace + pInformation->Ctrl_Info.Usb_wOffset)); */
/*   } */
  return NULL;
}

void dfuCopyBufferToExec() {
  int i;
  u32* userSpace;

  
  //  if (pInformation->Current_AlternateSetting != 1) {
  if (!userFlash) {
    userSpace = (u32*)(USER_CODE_RAM+userFirmwareLen);
    /* we dont need to handle when thisBlock len is not divisible by 4,
     since the linker will align everything to 4B anyway */
    for (i=0;i<thisBlockLen;i=i+4) {
      *userSpace++ = *(u32*)(recvBuffer+i);
    }
  } else {
    userSpace = (u32*)(USER_CODE_FLASH+userFirmwareLen);

    /* moved this out to usb reset, so that we have more time */
    /* engage a flash write */
    if (userFirmwareLen == 0) {
      //      flashErasePages((u32)(userSpace),2);
    } else {
      /* always nix the next 1KB page! */
      //      flashErasePage((u32)(userSpace+0x100));
    }
    flashErasePage((u32)(userSpace));

    for (i=0;i<thisBlockLen;i=i+4) {
      flashWriteWord(userSpace++,*(u32*)(recvBuffer+i));
    }

  }
  userFirmwareLen += thisBlockLen;

  thisBlockLen = 0;
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

u8 dfuGetState(void) {
  return dfuAppStatus.bState;
}

void dfuSetState(u8 newState) {
  dfuAppStatus.bState = newState;
}
