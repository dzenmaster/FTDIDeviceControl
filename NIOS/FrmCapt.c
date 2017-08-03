
#include "sw_regs.h"
#include "alt_types.h"
#include "FrmCapt.h"

alt_u32 FRM_CAPT_MODE_RG;
alt_u32 FRM_CAPT_CNTFRM_RG;

alt_u8 g_FRMCAPT_STATE;

void FrmCaptCommands (sw_reg_t *FrmCapt_reg)
{
	if (FrmCapt_reg ->f_RD_WRn == 0x1) //read
	{

	}
	else
	{
		switch(FrmCapt_reg ->addr)
		{
			case FRMCAPT_AREA_MODE_ADDR		: FRM_CAPT_MODE_RG = FrmCapt_reg -> data;
											  dbg_printf("[FRM_CAPT] Mode -> %x \n\r", FRM_CAPT_MODE_RG);
											//  g_FRMCAPT_STATE = FRMCAPT_RUN;
											  break;
			case FRMCAPT_AREA_FRMCNT_ADDR	: FRM_CAPT_CNTFRM_RG = FrmCapt_reg -> data;
											  dbg_printf("[FRM_CAPT] Number of frames -> %x \n\r", FRM_CAPT_CNTFRM_RG);
											  g_FRMCAPT_STATE = FRMCAPT_RUN;
											  break;
		}
	}
}

