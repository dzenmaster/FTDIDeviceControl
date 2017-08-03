/*
 * sw_regs.h
 *
 *  Created on: 24 мая 2017 г.
 *      Author: artemv
 */
#include "alt_types.h"

#ifndef SW_REGS_H_
#define SW_REGS_H_


// EPCS RG addresses

#define EPCS_AREA_START_ADDR      (0x0)
//
#define EPCS_ID_ADDR 			  (0x0)
#define EPCS_START_ADDRESS_ADDR   (0x1)
#define EPCS_TOTAL_LENTH_ADDR 	  (0x2)
#define EPCS_COMMANDS_ADDR        (0x3)
#define EPCS_APL_FROM_ADDR        (0x4)
//
#define EPCS_AREA_END_ADDR		  (0x4)

// RSU RG addresses

#define RSU_AREA_START_ADDR       (0x10)
//
#define RSU_SET_BOOT_ADDR_ADDR	  (0x10)
#define RSU_SET_RECONFIG_ADDR	  (0x11)
//
#define RSU_AREA_END_ADDR		  (0x11)

// Frame capture regs

#define FRMCAPT_AREA_START_ADDR   (0x15)
//
#define FRMCAPT_AREA_MODE_ADDR	  (0x15)
#define FRMCAPT_AREA_FRMCNT_ADDR  (0x16)
//
#define FRMCAPT_AREA_END_ADDR	  (0x16)


#pragma pack(push, 1) //packet struct
typedef struct
{
	alt_u8  f_RD_WRn;
	alt_u16 addr;
	alt_u32 data;
}sw_reg_t;
#pragma pack(pop)




#endif /* SW_REGS_H_ */
