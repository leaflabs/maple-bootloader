#include "maple_usb.h"

/* declare all those nasty globals. todo, wrap them up into
   a singleton object struct which we can pass around or point
   to as a single global
*/
vu32 bDeviceState = UNCONNECTED;
DEVICE Device_Table = 
  {
    NUM_ENDPTS,
    1
  }

DEVICE_PROP Device_Property = 
  {
    usbInit,
    usbReset,
    usbStatusIn,
    usbStatusOut,
    usbDataSetup,
    usbNoDataSetup,
    usbGetInterfaceSetting,
    usbGetDeviceDescriptor,
    usbGetConfigDescriptor,
    usbGetStringDescriptor,
    0,
    bMaxPacketSize
  };

USER_STANDARD_REQUESTS User_Standard_Requests = 
  {
    usbGetConfiguration,
    usbSetConfiguration,
    usbGetInterface,
    usbSetInterface,
    usbGetStatus,
    usbClearFeature,
    usbSetEndpointFeature,
    usbSetDeviceFeature.
    usbSetDeviceAddress
  };


struct
{
  volatile RESUME_STATE eState;
  volatile u8 bESOFcnt;
} ResumeS;



/* Function Definitions */
void usbAppInit(void) {
  /* hook in to usb_core, depends on all those damn
     non encapsulated externs! */
  USB_Init();
}

void usbSuspend(void) {
  u16 wCNTR;
  wCNTR = _GetCNTR();
  wCNTR |= CNTR_FSUSP | CNTR_LPMODE;
  _SetCNTR(wCNTR);

  /* run any power reduction handlers */
  bDeviceState = SUSPENDED;
}

void usbResumeInit(void) {
  u16 wCNTR;

  /* restart any clocks that had been stopped */

  wCNTR = _GetCNTR();
  wCNTR &= (~CNTR_LPMODE);
  _SetCNTR(wCNTR);

  /* undo power reduction handlers here */

  _SetCNTR(IMR_MSK);
}

void usbResume(RESUME_STATE eResumeSetVal) {
  u16 wCNTR;

  if (eResumeSetVal != RESUME_ESOF)
    ResumeS.eState = eResumeSetVal;

  switch (ResumeS.eState)
    {
    case RESUME_EXTERNAL:
      Resume_Init();
      ResumeS.eState = RESUME_OFF;
      break;
    case RESUME_INTERNAL:
      Resume_Init();
      ResumeS.eState = RESUME_START;
      break;
    case RESUME_LATER:
      ResumeS.bESOFcnt = 2;
      ResumeS.eState = RESUME_WAIT;
      break;
    case RESUME_WAIT:
      ResumeS.bESOFcnt--;
      if (ResumeS.bESOFcnt == 0)
        ResumeS.eState = RESUME_START;
      break;
    case RESUME_START:
      wCNTR = _GetCNTR();
      wCNTR |= CNTR_RESUME;
      _SetCNTR(wCNTR);
      ResumeS.eState = RESUME_ON;
      ResumeS.bESOFcnt = 10;
      break;
    case RESUME_ON:
      ResumeS.bESOFcnt--;
      if (ResumeS.bESOFcnt == 0)
	{
	  wCNTR = _GetCNTR();
	  wCNTR &= (~CNTR_RESUME);
	  _SetCNTR(wCNTR);
	  ResumeS.eState = RESUME_OFF;
	}
      break;
    case RESUME_OFF:
    case RESUME_ESOF:
    default:
      ResumeS.eState = RESUME_OFF;
      break;
    }
}

RESULT usbPowerOn(void) {
  u16 wRegVal;

  wRegVal = CNTR_FRES;
  _SetCNTR(wRegVal);

  wInterrupt_Mask = 0;
  _SetCNTR(wInterrupt_Mask);
  _SetISTR(0);
  wInterrupt_Mask = CNTR_RESETM | CNTR_SUSPM | CNTR_WKUPM; /* the bare minimum */
  _SetCNTR(wInterrupt_Mask);
  
  return USB_SUCCESS;
}

RESULT usbPowerOff(void) {
  _SetCNTR(CNTR_FRES);
  _SetISTR(0);
  _SetCNTR(CNTR_FRES + CNTR_PDWN);

  /* note that all weve done here is powerdown the 
     usb peripheral. we have no disabled the clocks,
     pulled the usb_disc pin back up, or reset the
     application state machines */

  return USB_SUCCESS;
}

void usbInit(void) {
  pInformation->Current_Configuration = 0;
  usbPowerOn();

  _SetISTR(0);
  wInterrupt_Mask = IMR_MSK;
  _SetCNTR(wInterrupt_Mask);
  usbEnbISTR();

  bDeviceState = UNCONNECTED;
}

void usbReset(void) {
  pInformation->Current_Configuration = 0;
  pInformation->Current_Feature = usbConfigDescriptor.Descriptor[7];

  _SetBTABLE(BTABLE_ADDRESS);

  /* setup the ctrl endpoint */
  _SetEPType(ENDP0, EP_CONTROL);
  _SetEPTxStatus(ENDP0, EP_TX_NAK);

  _SetEPRxAddr(ENDP0, ENDP0_RXADDR);
  SetEPRxCount(ENDP0, pProperty->MaxPacketSize);

  _SetEPTxAddr(ENDP0, ENDP0_TXADDR);
  SetEPTxCount(ENDP0, pProperty->MaxPacketSize);
  
  Clear_Status_Out(ENDP0);
  SetEPRxValid(ENDP0);

  bDeviceState = ATTACHED;
  SetDeviceAddress(0); /* different than usbSetDeviceAddr! comes from usb_core */
}

void usbStatusIn(void) {
  /* nada here, cb for status in requests */
  /* in order for this to be called the host must have acked
     our transmission, so this doesnt do much (increment the dfu state machine?)*/
}

void usbStatusOut(void) {
  /* handle DFU status Out requests, otherwise ignore */
}

RESULT usbDataSetup(u8 request) {
  u8 *(*CopyRoutine)(u16);
  CopyRoutine = NULL;

  if ((pInformation->USBbmRequestType & (REQUEST_TYPE | RECIPIENT)) == (CLASS_REQUEST | INTERFACE_RECIPIENT))
  {
    /* switch over the various CLASS Interface requests
       setting the copyroutine as appropriate
    */

/*     if (request == DFU_REQUEST_ITEM) { */
/*     } else if (...) { */
/*     } */
  }

  if (CopyRoutine != NULL) {
    pInformation->Ctrl_Info.CopyData = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
    (*CopyRoutine)(0);

    return USB_SUCCESS;
  }

  return USB_UNSUPPORT;
}

RESULT usbNoDataSetup(u8 requestNo) {
  if ((pInformation->USBbmRequestType & (REQUEST_TYPE | RECIPIENT)) == (CLASS_REQUEST | INTERFACE_RECIPIENT))
  {
    /* switch over the various CLASS Interface requests 
     return USB_SUCCESS when appropriate */

  }

  return USB_UNSUPPORT;
}

u8* usbGetInterfaceSetting(u8 interface, u8 altSetting) {
  /* we only support alt setting 0 for now */
  if (altSetting < 1) {
    return USB_SUCCESS;
  };

  return USB_UNSUPPORT;
}

u8* usbGetDeviceDescriptor(u16 len) {
  /* descriptor defined in maple_usb_desc */
  /* this function (from usb_core) is exactly the same format as
     the copyRoutine functions from dataSetup requests
  */
  return Standard_GetDescriptorData(len, &usbDeviceDescriptor);
}

u8* usbGetConfigDescriptor(u16 len) {
  /* descriptor defined in maple_usb_desc */
  /* this function (from usb_core) is exactly the same format as
     the copyRoutine functions from dataSetup requests
  */
  return Standard_GetDescriptorData(len, &usbConfigDescriptor);
}

u8* usbGetStringDescriptor(u16 len) {
  /* descriptor defined in maple_usb_desc */
  /* this function (from usb_core) is exactly the same format as
     the copyRoutine functions from dataSetup requests
  */

  /* for now we actually have no string descriptors! */
  u8 strIndex = pInformation->USBwValue0;
  return NULL;
  //  return Standard_GetDescriptorData(len, &usbStringDescriptor[strIndex]);
}

/* void usbGetConfiguration(void); */

void usbSetConfiguration(void) {
  if (pInformation->Current_Configuration != 0)
  {
    bDeviceState = CONFIGURED;
  }
}

/* void usbGetInterface(void); */
/* void usbSetInterface(void); */
/* void usbGetStatus(void); */
/* void usbClearFeature(void); */
/* void usbSetEndpointFeature(void); */
/* void usbSetDeviceFeature(void); */

void usbSetDeviceAddress(void) {
  bDeviceState = ADDRESSED;
}

/* void usbSetupISR(void); */
/* void usbEnbISTR(void) */
/* void usbISTR(void); */
