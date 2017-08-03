/*
 * RSU.h
 *
 *  Created on: 20 мая 2017 г.
 *      Author: artemv
 */

#ifndef RSU_H_
#define RSU_H_
#include "alt_types.h"
#include "sw_regs.h"



// RSU addresses

#define ReconfigSrcAddr  (0x0  << 2)
#define ReconfigCtrlAddr (0x20 << 2)
#define BootAddrAddr	 (0x4  << 2)
#define AnF_Addr		 (0x5  << 2) // for Cyclone 5 only
#define WatchDogAddr     (0x3  << 2)

// Only for Cyclone 4

#define CurStateAddr    0x0
#define Prev1StateAddr  (0x1 << 3) << 2
#define Prev2StateAddr  (0x2 << 3) << 2

// Read back boot address
#define	ReadBackBootAddrAddr1 0x1C << 2
#define	ReadBackBootAddrAddr2 0x0B << 2
// Previous State Registers Contents in Status Register
#define	PrevStateReg1Addr 	  0x0F << 2
#define	PrevStateReg2Addr     0x17 << 2
#define	WatchDogValueAddr     0x02 << 2

#define ReconfigTrigCondition  (0x7 << 2)


// registers values

#define AnF_Bit 		0x1
#define WD_Disable 		0x0
#define WD_Enable 		0x1

#define WdRegValue 		0x1FF

void RsuInit (alt_u8 CurImage);
void RsuWdReset(void);
void RsuCommands (sw_reg_t *sw_reg_t);

#endif /* RSU_H_ */
