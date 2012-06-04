#include "dfu.h"
#include "usb.h"

extern volatile u8 recvBuffer[wTransferSize];
extern volatile u32 userFirmwareLen;
extern volatile u16 thisBlockLen;

void dfuCopyToRam()
{
	int i;
	u32* userSpace = (u32*)(USER_CODE_RAM+userFirmwareLen);
	/* we dont need to handle when thisBlock len is not divisible by 4,
	   since the linker will align everything to 4B anyway */
	for (i=0;i<thisBlockLen;i=i+4) {
		*userSpace++ = *(u32*)(recvBuffer+i);
	}
}

void dfuCopyToFlash()
{
	int i;
	u32* userSpace = (u32*)(USER_CODE_FLASH+userFirmwareLen);
	flashErasePage((u32)(userSpace));
	for (i=0;i<thisBlockLen;i=i+4) {
		flashWriteWord((u32)userSpace++,*(u32*)(recvBuffer+i));
	}

}

void dfuCopyToSPIFlash()
{

}

void dfuCopyToFPGA()
{

}
