
#include "sys/alt_stdio.h"
#include "alt_types.h"
#include "system.h"
#include "uart_cmd.h"
#include "uart_cmd_pkg_process.h"
#include "epcs_flash.h"
#include "RSU.h"
#include "sw_regs.h"

///

void process_info_packet(void)
{
	alt_u8 info_addr;
	UartCmd_get_data(&info_addr);
	switch (info_addr)
	{
		case 0 : UartCmd_Send_Module_type(0x1); 		break;
		case 1 : UartCmd_Send_SN(0x2);		    		break;
		case 2 : UartCmd_Send_FirmWare_revision(0x3); 	break;
		case 3 : UartCmd_Send_SoftWare_revision(0x4); 	break;
		default :  dbg_printf("[Info] Unknown parameter\n\r");
	}
}

///

void process_CurState_packet(alt_u8 state)
{
	alt_u16 rx_pkg_len=0;
	alt_u8  rx_data=0;
	alt_u8  tx_data=0;
	alt_u8  TxPkgType =0;
	alt_u8	tx_len;

	rx_pkg_len = UartCmd_get_pkg_len();
	UartCmd_get_data(&rx_data);
	if (rx_pkg_len == 0x1 && rx_data == 0x1)
	{
		tx_len  = 0x1;
		tx_data = state;
		TxPkgType = PKG_TYPE_STATE_REQ;
		UartCmd_send_data(&tx_data, TxPkgType,tx_len);
	}
	else
	{
		tx_len  = 0x1;
		tx_data = NIOS_CMD_WRONG_COMMAND;
		TxPkgType = PKG_TYPE_STATE_REQ;
		UartCmd_send_data(&tx_data, TxPkgType,tx_len);
	}
}

///

void send_response_packet(alt_u8 state)
{
	alt_u8  tx_data=0;
	alt_u8  TxPkgType =0;
	alt_u8	tx_len;

	tx_len  = 0x1;
	tx_data = state;
	TxPkgType = PKG_TYPE_STATE_REQ;
	UartCmd_send_data(&tx_data, TxPkgType,tx_len);

}

///

alt_u8 process_SW_registers(void)
{
	sw_reg_t sw_reg;
	alt_u8 res;
	UartCmd_get_data(&sw_reg);

	if(sw_reg.addr >= EPCS_AREA_START_ADDR && sw_reg.addr <= EPCS_AREA_END_ADDR)
	{
		res = epcs_commands(&sw_reg);
		if(res != EPCS_OK)
			send_response_packet(NIOS_CMD_SW_RG_WRITE_ERR);
		else if(sw_reg.f_RD_WRn == 0) // response requires only for write
			send_response_packet(NIOS_CMD_OK);
	}
	else if(sw_reg.addr >= RSU_AREA_START_ADDR && sw_reg.addr <= RSU_AREA_END_ADDR)
	{
		 send_response_packet(NIOS_CMD_OK); // need to send OK before reconfiguration
		 RsuCommands (&sw_reg);
	}

	else if(sw_reg.addr >= FRMCAPT_AREA_START_ADDR && sw_reg.addr <= FRMCAPT_AREA_END_ADDR)
	{
		 FrmCaptCommands (&sw_reg);
		 send_response_packet(NIOS_CMD_OK);
	}
	else
	{
		dbg_printf("[SW_REG] Unknown address\n\r ");
	}
}

///

void process_epcs_flash(alt_u8 sate)
{
		alt_u8  buf[1024];
		alt_u32 len;
		alt_u8  res;

		len = 0;

		if(sate == EPCS_STATE_WRITE_FW)
		{
			len = UartCmd_get_data(&buf);
			res = epcs_write_fw(&buf,len);
			if(res == EPCS_OK)
				send_response_packet(NIOS_CMD_OK);
			else
				send_response_packet(NIOS_CMD_EPCS_WRITE_ERR);
		}
		else if(sate == EPCS_STATE_READ_FW)
		{
			do
			{
				RsuWdReset();
				len=epcs_read_fw(&buf);
				if(len != 0)
					UartCmd_Send_Stream(buf,len);
				else
				{
					send_response_packet(NIOS_CMD_EPCS_READ_ERR);
					g_EPCS_STATE = EPCS_STATE_IDLE;
					dbg_printf("[EPCS] Read ERR\n\r");
				}

			}
			while(len == 1024);

			if(len != 0)
				send_response_packet(NIOS_CMD_OK);
		}

}
