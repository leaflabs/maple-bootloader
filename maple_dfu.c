#include "maple_dfu.h"

/* DFU globals */
u32 userAppAddr = 0x20000600; /* default RAM user code location */
DFUStatus dfuAppStatus;       /* includes state */


/* todo: force dfu globals to be singleton to avoid re-inits? */
void dfuInit(void) {
  dfuAppStatus.bStatus = OK;
  dfuAppStatus.bwPollTimeout0 = 0x00;  /* todo: unionize the 3 bytes */
  dfuAppStatus.bwPollTimeout1 = 0x00;  /* todo: unionize the 3 bytes */
  dfuAppStatus.bwPollTimeout2 = 0x00;  /* todo: unionize the 3 bytes */
  dfuAppStatus.bState = appIDLE;
  dfuAppStatus.iString = 0x00          /* all strings must be 0x00 until we make them! */
}

void dfuUpdateState(void) {
  /* were using the global pInformation struct from usb_lib here,
     see comment in maple_dfu.h around DFUEvent struct */
  
  u8 startState = dfuAppStatus.bState;
  /* often leaner to nest if's then embed a switch/case */
  if        (startState == appIDLE)                {
    /* device running outside of DFU Mode */
  } else if (startState == appDETACH)              {
    /* device has received DETACH, awaiting usb reset */
  } else if (startState == dfuIDLE)                {
    /*  device running inside DFU mode */
  } else if (startState == dfuDNLOAD_SYNC)         {
    /* device received block, waiting for DFU_GETSTATUS request */
  } else if (startState == dfuDNBUSY)              {
    /* device is busy programming non-volatile memories, please wait */
  } else if (startState == dfuDNLOAD_IDLE)         {
    /* device is expecting dfu_dnload requests */
  } else if (startState == dfuMANIFEST_SYNC)       {
    /* device has received last block, waiting DFU_GETSTATUS request */
  } else if (startState == dfuMANIFEST)            {
    /* device is in manifestation phase */
  } else if (startState == dfuMANIFEST_WAIT_RESET) {
    /* device has programmed new firmware but needs external
       usb reset or power on reset to run the new code */
  } else if (startState == dfuUPLOAD_IDLE)         {
    /* device expecting further dfu_upload requests */
  } else if (startState == dfuERROR)               {
    /* status is in error, awaiting DFU_CLRSTATUS request */
  } else {
    /* some kind of error... */
  }
}

/* DFUState dfuGetState(); */
/* void dfuSetState(DFUState); */
/* ClassReqCB dfuGetClassHandler(void); */
