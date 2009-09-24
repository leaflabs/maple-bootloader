#ifndef __MAPLE_DFU_H
#define __MAPLE_DFU_H

#include "maple_lib.h"

/* exposed enums */
typedef enum _DFUState {
} DFUState;

typedef enum _DFUEvent {
} DFUEvent;

typedef u8*(*ClassReqCB)(u16);

/* DFU special numbers */


/* exposed functions */
void dfuInit(void);  /* singleton dfu initializer */
void dfuUpdateState(DFUEvent);
DFUState dfuGetState();
void dfuSetState(DFUState);
ClassReqCB dfuGetClassHandler(void);

#endif
