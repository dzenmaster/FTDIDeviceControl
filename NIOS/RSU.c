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
#include "sw_regs.h"


void RsuInit (alt_u8 CurImage) // 0xF - factory; 0xA - application

{
	alt_u32 res;
	alt_u32 PrevStateReg1;
	alt_u32 PrevStateReg2;

	// Turn off watchdog. Operation available only in factory mode.
	IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogAddr,WD_Disable);
/*	//Read Reconfiguration Source; Current State Contents in Status Register
	res = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigSrcAddr);
	if(res == 0 && CurImage == 0xF){ // we are in factory mode
		dbg_printf("[RSU] Reconfiguration Source -> %x \n\r",res);
		// Previous State Register 1 Contents in Status Register
		PrevStateReg1 = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, PrevStateReg1Addr);
		dbg_printf("[RSU] PreviousState 1  -> %x \n\r",PrevStateReg1);
		// Previous State Register 2 Contents in Status Register
		PrevStateReg2 = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, PrevStateReg2Addr);
		dbg_printf("[RSU] PreviousState 2  -> %x \n\r",PrevStateReg2);

		if(PrevStateReg1 == 0 && PrevStateReg2 == 0) // Power Up, Let's try to jump on application
		{ IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogAddr,WD_Disable);
			// Write boot address
			IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, BootAddrAddr,EPCS_APL_BOOT_ADDR);
			// Read back boot address
			res = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, ReadBackBootAddrAddr1);
			dbg_printf("[RSU] Past Boot ADDR1 -> %x \n\r",res);
			res = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, ReadBackBootAddrAddr2);
			dbg_printf("[RSU] Past Boot ADDR2 -> %x \n\r",res);

			// Turn on watchdog
			IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogAddr,WD_Enable);
			//Write WD value
			IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogValueAddr,WdRegValue);
			Delay(300000); // wait until dbg_printf send mesage
			// Write RECONFIG
			IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigCtrlAddr,0x1);
		}
		else
		{
			if((PrevStateReg1 & 0x1) == 0x1 || (PrevStateReg2 & 0x1) == 0x1)
			{// Jumped back from application
				dbg_printf("[RSU] Configuration reset triggered from logic array\n\r");
				IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, BootAddrAddr,EPCS_APL_BOOT_ADDR);
				// Turn on watchdog
				IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogAddr,WD_Enable);
				//Write WD value
				IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogValueAddr,WdRegValue);
				Delay(300000); // wait until dbg_printf send message
				// Write RECONFIG
				IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigCtrlAddr,0x1);
			}

			if(((PrevStateReg1 & 0x2) >> 1) == 0x1 || ((PrevStateReg2 & 0x2) >> 1) == 0x1)
				dbg_printf("[RSU] User watchdog timer timeout\n\r");

			if(((PrevStateReg1 & 0x4) >> 2) == 0x1 || ((PrevStateReg2 & 0x4) >> 2) == 0x1)
				dbg_printf("[RSU] nstatus asserted by an external device as the result of an error\n\r");

			if(((PrevStateReg1 & 0x8) >> 3) == 0x1 || ((PrevStateReg2 & 0x8) >> 3) == 0x1)
				dbg_printf("[RSU] CRC error during application configuration\n\r");

			if(((PrevStateReg1 & 0x10) >> 4) == 0x1 || ((PrevStateReg2 & 0x10) >> 4) == 0x1)
				dbg_printf("[RSU] External configuration reset (nconfig) assertion\n\r");
		}
	}
	else
	{
		dbg_printf("[RSU] PreviousState 1  -> %x \n\r",PrevStateReg1);
		dbg_printf("[RSU] PreviousState 2  -> %x \n\r",PrevStateReg2);


	}
	res = IORD_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigSrcAddr);
	dbg_printf("[RSU] Current Image -> %x \n\r",res);
*/
}

///

void RsuWdReset(void)
{
	//reset wd
		IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigCtrlAddr,0x2);
		IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigCtrlAddr,0x0);
}

///

void RsuCommands (sw_reg_t *sw_reg_t)
{

	if(sw_reg_t->addr == RSU_SET_BOOT_ADDR_ADDR)
	{
		// Write boot address
		IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, BootAddrAddr,sw_reg_t->data);
		dbg_printf("[RSU] Set boot address -> %x \n\r",sw_reg_t->data);
	}
	else if(sw_reg_t->addr == RSU_SET_RECONFIG_ADDR)
	{
		Delay(100000);
		// Turn on watchdog
		IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogAddr,WD_Enable);
		//Write WD value
		IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, WatchDogValueAddr,WdRegValue);
		// Write RECONFIG
		IOWR_32DIRECT(RSU_CYCLONE5_0_BASE, ReconfigCtrlAddr,0x1);
	}

}

///

void Delay(alt_u32 tiks)
{
	while(tiks!=0)
	{
		tiks--;
	}
}
