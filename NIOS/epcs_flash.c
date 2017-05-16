/*
 * epcs_flash.c
 *
 *  Created on: 16 мая 2017 г.
 *      Author: artemv
 */

#include "system.h"
#include "epcs_flash.h"
#include "alt_types.h"
#include "alt_types.h"
#include "epcs_commands.h"


static alt_u32 epcs_id_rg = 0;
static alt_u32 epcs_start_address_rg= 0;
static alt_u32 epcs_total_lenth_rg= 0;
static alt_u32 epcs_cmd_rg= 0x00;
alt_u8 g_STATE_EPCS_UPD_FW;

void epcs_commands(epcs_reg_t *epcs_area)
{

	if(epcs_area->f_RD_WRn == 0x1) // read register
	{
		switch (epcs_area->addr)
		{
			case EPCS_ID_ADDR 				: epcs_read_flash_id();
											  break;
			case EPCS_START_ADDRESS_ADDR	: UartCmd_Send_SW_reg_val(EPCS_START_ADDRESS_ADDR,epcs_start_address_rg);
											  break;
			case EPCS_TOTAL_LENTH_ADDR		: UartCmd_Send_SW_reg_val(EPCS_TOTAL_LENTH_ADDR,epcs_total_lenth_rg);
											  break;
			case EPCS_COMMANDS_ADDR    		: UartCmd_Send_SW_reg_val(EPCS_COMMANDS_ADDR,epcs_cmd_rg);
											  break;
		}
	}
	else
	{
		switch (epcs_area->addr)
		{
			case EPCS_ID_ADDR 				: return(ERR_WR);//send_response_packet(NIOS_CMD_SW_RG_WRITE_ERR); // only read register
											  break;
			case EPCS_START_ADDRESS_ADDR	: epcs_start_address_rg = epcs_area->data;
											  break;
			case EPCS_TOTAL_LENTH_ADDR		: epcs_total_lenth_rg = epcs_area->data;
											  break;
			case EPCS_COMMANDS_ADDR    		: epcs_cmd_rg = epcs_area->data;
											  epcs_run_cmd();
											  break;
		}

	}

	return(OK);
}

///

void epcs_read_flash_id(void)
{

	epcs_id_rg = epcs_read_electronic_signature(EPCS_FLASH_CONTROLLER_0_BASE+
											 	EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET);

	UartCmd_Send_SW_reg_val(EPCS_ID_ADDR,epcs_id_rg);
}

///

alt_u8 epcs_run_cmd(void)
{
	switch (epcs_cmd_rg)
	{
		case EPCS_CMD_NULL				: break;

		case EPCS_CMD_WRITE				: break;

		case EPCS_CMD_READ				: break;

		case EPCS_CMD_ERASE_SECTOR		: break;
			epcs_sector_erase(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
							  epcs_start_address_rg,
							  0x0);
			epcs_cmd_rg = EPCS_CMD_NULL;
			break;

		case EPCS_CMD_UPDATE_FIRMWARE	: g_STATE_EPCS_UPD_FW = 1;
										  break;
	}

}

///

void epcs_write_fw(alt_u8*  src_data,alt_u16 len)
{
	static alt_u32 bytes_written;
	alt_u16 n_bytes;
	alt_u8  *write_buf;
	int i =0;

	n_bytes = len;
	write_buf = 0;

	for(i=0;i<len;i++) // bit swap
	{
		*(write_buf+i) = BitReverseTable256[*(src_data+i)];
	}

	while(n_bytes >= 256) // maximum write length
	{
			epcs_write_buffer(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
							  epcs_start_address_rg,
							  write_buf,
			                  256,
			                  0x0);
			epcs_start_address_rg += 256;
			write_buf += 256;
			n_bytes -=256;
			bytes_written += 256;
	}
	if(n_bytes != 0) // packet less the 256 bytes
	{
		epcs_write_buffer(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
											  epcs_start_address_rg,
											  write_buf,
											  n_bytes,
							                  0x0);
		n_bytes = 0;
		bytes_written += n_bytes;
		epcs_start_address_rg += n_bytes;
	}

	if(bytes_written == epcs_total_lenth_rg)
	{
		epcs_cmd_rg = EPCS_CMD_NULL;
		g_STATE_EPCS_UPD_FW = 0x0;
		bytes_written = 0;
	}


}
