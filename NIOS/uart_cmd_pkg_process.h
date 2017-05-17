
#include "sys/alt_stdio.h"
#include "alt_types.h"
#include "system.h"

///

/// Pakage types

enum {
	PKG_TYPE_INFO		= 0x0,
	PKG_TYPE_STATE_REQ	= 0x1,
	PKG_TYPE_ASCII	    = 0x2,
	PKG_TYPE_NIOS_RG	= 0x3,
	PKG_TYPE_SREAM_DATA = 0x4
};

/// Current module state or response command state

enum {
	NIOS_CMD_OK 		      = 0x0,
	NIOS_CMD_WRONG_SYNC       = 0x1,
	NIOS_CMD_LEN_USEDW_ERR    = 0x2,
	NIOS_CMD_TIMEOUT_OCCURRED = 0x3,
	NIOS_CMD_BUSY			  = 0x4,
	NIOS_CMD_WRONG_COMMAND	  = 0x5,
	NIOS_CMD_UNKN_PKG_TYPE	  = 0x6,
	NIOS_CMD_SW_RG_WRITE_ERR  = 0x7
};

///

#pragma pack(push, 1) //packet struct
typedef struct
{
	alt_u8  f_RD_WRn;
	alt_u16 addr;
	alt_u32 data;
}sw_reg_t;
#pragma pack(pop)

typedef sw_reg_t  *sw_reg_xp;

///



void process_info_packet(void);

void process_CurState_packet(alt_u8 state);
void send_response_packet(alt_u8 state);
alt_u8 process_SW_registers(void);
void process_epcs_flash(alt_u8 state);


