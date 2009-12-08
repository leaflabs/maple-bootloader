#include "maple_dfu.h"

/* DFU globals */
u32 userAppAddr = 0x20000C00; /* default RAM user code location */
DFUStatus dfuAppStatus;       /* includes state */
bool userFlash = FALSE;

u8 recvBuffer[wTransferSize];
u32 userFirmwareLen = 0;
u32 thisBlockLen = 0;


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

void transition_appIDLE(void) {
    u32 usbReq = pInformation->USBbRequest;
    switch (usbReq) {
    case DFU_DETACH:
        dfuAppStatus.bState = appDETACH;
        break;
    case DFU_GETSTATUS:
        dfuAppStatus.bState  = appIDLE;
        break;
    case DFU_GETSTATE:
        dfuAppStatus.bState  = appIDLE;
        break;
    default:
        dfuAppStatus.bState  = appIDLE;
        dfuAppStatus.bStatus = errSTALLEDPKT;
    }
}

void transition_appDETACH(void) {
    u32 usbReq = pInformation->USBbRequest;
    switch (usbReq) {
    case DFU_GETSTATUS:
        dfuAppStatus.bState  = appDETACH;
        break;
    case DFU_GETSTATE:
        dfuAppStatus.bState  = appDETACH;
        break;
    default:
        dfuAppStatus.bState  = appIDLE;
        dfuAppStatus.bStatus = errSTALLEDPKT;
    }
}

void transition_dfuIDLE(void) {
    u32 usbReq = pInformation->USBbRequest;
    switch (usbReq) {
    case DFU_DNLOAD:
        if (pInformation->USBwLengths.w > 0) {
            userFirmwareLen = 0;
            dfuAppStatus.bState  = dfuDNLOAD_SYNC;

            if (pInformation->Current_AlternateSetting == 1) {
                userAppAddr = USER_CODE_FLASH;
                userFlash = TRUE;

                /* make sure the flash is setup properly, unlock it */
                setupFLASH();
                flashUnlock();
            } else {
                userAppAddr = USER_CODE_RAM;
                userFlash = FALSE;
            }
        } else {
            dfuAppStatus.bState  = dfuERROR;
            dfuAppStatus.bStatus = errNOTDONE;
        }
        break;
    case DFU_UPLOAD:
        dfuAppStatus.bState  = dfuUPLOAD_IDLE;
        break;
    case DFU_ABORT:
        dfuAppStatus.bState  = dfuIDLE;
        dfuAppStatus.bStatus = OK;  /* are we really ok? we were just aborted */
        break;
    case DFU_GETSTATUS:
    case DFU_GETSTATE:
        dfuAppStatus.bState  = dfuIDLE;
        break;
    default:
        dfuAppStatus.bState  = dfuERROR;
        dfuAppStatus.bStatus = errSTALLEDPKT;
        break;
    }
}

void transition_dfuDNLOAD_SYNC(void) {
    u32 usbReq = pInformation->USBbRequest;
    switch (usbReq) {
    case DFU_GETSTATUS:
        /* todo, add routine to wait for last block write to finish */
        if (userFlash) {
            switch (code_copy_lock) {
            case WAIT:
                code_copy_lock=BEGINNING;
                dfuAppStatus.bwPollTimeout0 = 0xFF; /* is this enough? */
                // 	  dfuAppStatus.bwPollTimeout1 = 0x0F; /* is this enough? */
                dfuAppStatus.bState=dfuDNLOAD_SYNC;
                break;
            case BEGINNING:
            case MIDDLE:
                dfuAppStatus.bState=dfuDNLOAD_SYNC;
                break;
            case END:
                dfuAppStatus.bwPollTimeout0 = 0x00;
                code_copy_lock=WAIT;
                dfuAppStatus.bState=dfuDNLOAD_IDLE;
                break;
            }
        } else {
            dfuAppStatus.bState = dfuDNLOAD_IDLE;
            dfuCopyBufferToExec();
        }
        break;
    case DFU_GETSTATE:
        dfuAppStatus.bState  = dfuDNLOAD_SYNC;
        break;
    default:
        dfuAppStatus.bState  = dfuERROR;
        dfuAppStatus.bStatus = errSTALLEDPKT;
        break;
    }
}

void transition_dfuDNLOAD_IDLE(void) {
    u32 usbReq = pInformation->USBbRequest;
    switch (usbReq) {
    case DFU_DNLOAD:
        if (pInformation->USBwLengths.w > 0) {
            dfuAppStatus.bState  = dfuDNLOAD_SYNC;
        } else {
            /* todo, support "disagreement" if device expects more data than this */
            dfuAppStatus.bState  = dfuMANIFEST_SYNC;

            /* relock the flash */
            flashLock();
        }
        break;
    case DFU_ABORT:
    case DFU_GETSTATUS:
    case DFU_GETSTATE:
        dfuAppStatus.bState  = dfuIDLE;
        break;
    default:
        dfuAppStatus.bState  = dfuERROR;
        dfuAppStatus.bStatus = errSTALLEDPKT;
    }
}

void transition_dfuMANIFEST_SYNC(void) {
    u32 usbReq = pInformation->USBbRequest;
    switch (usbReq) {
    case DFU_GETSTATUS:
        dfuAppStatus.bState  = dfuMANIFEST_WAIT_RESET;
        dfuAppStatus.bStatus = OK;
        break;
    case DFU_GETSTATE:
        dfuAppStatus.bState  = dfuMANIFEST_SYNC;
        break;
    default:
        dfuAppStatus.bState  = dfuERROR;
        dfuAppStatus.bStatus = errSTALLEDPKT;
    }
}

bool dfuUpdateByRequest(void) {
    /* were using the global pInformation struct from usb_lib here,
       see comment in maple_dfu.h around DFUEvent struct */
    u8 startState = dfuAppStatus.bState;
    switch (startState) {
    case appIDLE:
        transition_appIDLE();
        break;
    case appDETACH:
        transition_appDETACH();
        break;
    case dfuIDLE:
        transition_dfuIDLE();
        break;
    case dfuDNLOAD_SYNC:
        transition_dfuDNLOAD_SYNC();
        break;
    case dfuDNBUSY:
        dfuAppStatus.bState= dfuDNLOAD_SYNC;
        break;
    case dfuDNLOAD_IDLE:
        transition_dfuDNLOAD_IDLE();
        break;
    case dfuMANIFEST_SYNC:
        transition_dfuMANIFEST_SYNC();
        break;
    case dfuMANIFEST:
        dfuAppStatus.bState  = dfuMANIFEST_WAIT_RESET;
        dfuAppStatus.bStatus = OK;
        break;
    case dfuMANIFEST_WAIT_RESET:
        dfuAppStatus.bState  = dfuMANIFEST_WAIT_RESET;
        break;
    case dfuUPLOAD_IDLE:
        break;
    case dfuERROR:
        break;
    default:
        dfuAppStatus.bState  = dfuERROR;
        dfuAppStatus.bStatus = errSTALLEDPKT;
        break;
    }
#if 0

    if (startState == dfuDNLOAD_SYNC)         {
    } else if (startState == dfuDNBUSY)              {
        /* if were actually done writing, goto sync, else stay busy */

    } else if (startState == dfuDNLOAD_IDLE)         {
        /* device is expecting dfu_dnload requests */

    } else if (startState == dfuMANIFEST_SYNC)       {

    } else if (startState == dfuMANIFEST)            {
        /* device is in manifestation phase */

        /* should never receive request while in manifest! */

    } else if (startState == dfuMANIFEST_WAIT_RESET) {
        /* device has programmed new firmware but needs external
           usb reset or power on reset to run the new code */

        /* consider timing out and self-resetting */

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
#endif

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
        usbConfigDescriptor.Descriptor[0x10] = 0x02;
        usbConfigDescriptor.Descriptor[0x19] = 0x02;
    } else if (startState == appIDLE || startState == dfuIDLE) {
        /* do nothing...might be normal usb bus activity */
    } else {
        /* we reset from the dfu, reset everything and startover,
           which is the correct operation if this is an erroneous 
           event or properly following a MANIFEST */
        dfuAppStatus.bState = appIDLE;
        dfuAppStatus.bStatus = OK;
        usbConfigDescriptor.Descriptor[0x10] = 0x01;
        usbConfigDescriptor.Descriptor[0x19] = 0x01;

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

    if (pInformation->Current_AlternateSetting != 1) {
        userSpace = (u32*)(USER_CODE_RAM+userFirmwareLen);
        for (i=0; i < thisBlockLen/4; i++) {
            *userSpace++ = *(u32*)(recvBuffer +4*i);
        }
    } else {
        userSpace = (u32*)(USER_CODE_FLASH+userFirmwareLen);

        /* engage a flash write */
        /* how many pages do we need to clear? */
        /* peform the write */
        if (userFirmwareLen == 0) {
            flashErasePage((u32)(userSpace));
        }
        /* always nix the next 1KB page! */
        flashErasePage(((u32)userSpace)+0x400);

        for (i = 0; i < thisBlockLen/4; i++) {
            flashWriteWord(userSpace++, *(u32*)(recvBuffer + 4*i));
        }
    }
    userFirmwareLen += thisBlockLen/4;

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
