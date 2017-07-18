#include "sw_regs.h"

extern alt_u8 g_FRMCAPT_STATE;


void FrmCaptCommands (sw_reg_t *FrmCapt_reg);

enum{
	FRMCAPT_NULL = 0x0,
	FRMCAPT_RUN  = 0x1,
};
