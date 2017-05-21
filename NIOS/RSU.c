/*
 * RSU.c
 *
 *  Created on: 20 мая 2017 г.
 *      Author: artemv
 */

#include "RSU.h"
#include "epcs_flash.h"
#include "system.h"
#include "io.h"
#include "dbg_fync.h"




void RsuInit (alt_u8 CurImage) // 0xF - factory; 0xA - application

{
	alt_u32 res;
	alt_u32 PrevStateReg1;
	alt_u32 PrevStateReg2;

	// Turn off watchdog
	// It is working only in factory mode!!
	  IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogAddr,WD_Disable);
	//Read Reconfiguration Source; Current State Contents in Status Register
	res = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigSrcAddr);
	if(res == 0 && CurImage == 0xF){ // we are in factory mode
		dbg_printf("[RSU] Reconfiguration Source -> %x \n",res);
		// Previous State Register 1 Contents in Status Register
		PrevStateReg1 = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, PrevStateReg1Addr);
		dbg_printf("[RSU] PreviousState 1  -> %x \n",PrevStateReg1);
		// Previous State Register 2 Contents in Status Register
		PrevStateReg2 = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, PrevStateReg2Addr);
		dbg_printf("[RSU] PreviousState 2  -> %x \n",PrevStateReg2);

		if(PrevStateReg1 == 0 && PrevStateReg2 == 0) // Power Up, Let's try to jump to application
		{ IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogAddr,WD_Disable);
			// Write boot address
			IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, BootAddrAddr,EPCS_APL_BOOT_ADDR);
			// Read back boot address
			res = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, ReadBackBootAddrAddr1);
			dbg_printf("[RSU] Past Boot ADDR1 -> %x \n",res);
			res = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, ReadBackBootAddrAddr2);
			dbg_printf("[RSU] Past Boot ADDR2 -> %x \n",res);

			// Turn on watchdog
			IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogAddr,WD_Enable);
			//Write WD value
			IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogValueAddr,WdRegValue);
			// Write RECONFIG
			IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigCtrlAddr,0x1);
		}
		else
		{
			if((PrevStateReg1 & 0x1) == 0x1 || (PrevStateReg2 & 0x1) == 0x1)
				dbg_putstr("[RSU] Configuration reset triggered from logic array\n");

			if(((PrevStateReg1 & 0x2) >> 1) == 0x1 || ((PrevStateReg2 & 0x2) >> 1) == 0x1)
				dbg_putstr("[RSU] User watchdog timer timeout\n");

			if(((PrevStateReg1 & 0x4) >> 2) == 0x1 || ((PrevStateReg2 & 0x4) >> 2) == 0x1)
				dbg_putstr("[RSU] nstatus asserted by an external device as the result of an error\n");

			if(((PrevStateReg1 & 0x8) >> 3) == 0x1 || ((PrevStateReg2 & 0x8) >> 3) == 0x1)
				dbg_putstr("[RSU] CRC error during application configuration\n");

			if(((PrevStateReg1 & 0x10) >> 4) == 0x1 || ((PrevStateReg2 & 0x10) >> 4) == 0x1)
				dbg_putstr("[RSU] External configuration reset (nconfig) assertion\n");
		}
	}
	else
	{
		dbg_printf("[RSU] PreviousState 1  -> %x \n",PrevStateReg1);
		dbg_printf("[RSU] PreviousState 2  -> %x \n",PrevStateReg2);


	}
	res = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigSrcAddr);
	dbg_printf("[RSU] Current Image -> %x \n",res);

}

///

void RsuWdReset(void)
{
	//reset wd
		IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigCtrlAddr,0x2);
		IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigCtrlAddr,0x0);
}
