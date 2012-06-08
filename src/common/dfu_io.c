


/* This file will be just sucked in dfu.c for the sake of simplicity */

#ifdef CONFIG_ALTSETTING_RAM
/* DFU ====> RAM */
void dfuToRamCopy()
{
	int i;
	u32* userSpace = (u32*)(USER_CODE_RAM+userFirmwareLen);
	/* we dont need to handle when thisBlock len is not divisible by 4,
	   since the linker will align everything to 4B anyway */
	for (i=0;i<thisBlockLen;i=i+4) {
		*userSpace++ = *(u32*)(recvBuffer+i);
	}
	userFirmwareLen += thisBlockLen;
	thisBlockLen = 0;
}

#ifdef CONFIG_UPLOAD

void dfuFromRamCopy()
{
	int i;
	u32* userSpace = (u32*)(USER_CODE_RAM+userFirmwareLen);
	/* we dont need to handle when thisBlock len is not divisible by 4,
	   since the linker will align everything to 4B anyway */
	for (i=0;i<thisBlockLen;i=i+4) {
		*(u32*)(recvBuffer+i) = *userSpace++;
	}
}
#endif


void dfuToRamInit()
{
	dfuCopyFunc = dfuToRamCopy;
	userAppAddr = USER_CODE_RAM;
	strobePin(LED_BANK,LED,5,BLINK_FAST);
#ifdef CONFIG_UPLOAD
	dfuUploadFunc = dfuFromRamCopy;
#endif
}
#endif

/* DFU ====> FLASH */

#ifdef CONFIG_ALTSETTING_FLASH
void dfuToFlashCopy()
{
	int i;
	u32* userSpace = (u32*)(USER_CODE_FLASH+userFirmwareLen);
	flashErasePage((u32)(userSpace));
	for (i=0;i<thisBlockLen;i=i+4) {
		flashWriteWord((u32)userSpace++,*(u32*)(recvBuffer+i));
	}
	userFirmwareLen += thisBlockLen;
	thisBlockLen = 0;

}

void dfuToFlashInit()
{
	userAppAddr = USER_CODE_FLASH;
	dfuCopyFunc = dfuToFlashCopy;
	setupFLASH();
	flashUnlock();
}
#endif

#ifdef CONFIG_ALTSETTING_RUN

void dfuToRunCopy()
{
	jumpToUser(CONFIG_RUN_ADDR);
}

void dfuToRunInit()
{
	/* We cannot jump from here, sice we're in interrupt */
	/* Without extra init it screws up interrupts in the app */
	dfuCopyFunc = dfuToRunCopy;
}
#endif


/* This one copies the fw to nowhere, just for performance testing */
#ifdef CONFIG_ALTSETTING_DUMMY

void dfuToNowhereCopy()
{
	userFirmwareLen += thisBlockLen;
	thisBlockLen = 0;

	/* just copied all the fw to astral */
}

void dfuToNowhereInit()
{
	userAppAddr = USER_CODE_FLASH;
	dfuCopyFunc = dfuToNowhereCopy;
}

#endif

#ifdef CONFIG_ALTSETTING_SPI

void dfuToSPICopy()
{
	/* TODO */
}
void dfuToSPIInit()
{
	/* TODO */
}

#endif

#ifdef CONFIG_ALTSETTING_FPGA

void dfuToFPGAInit()
{
	/* TODO */
}

#endif
/* TODO: FPGA,SPI,etc */
/* DFU ====> FPGA */
