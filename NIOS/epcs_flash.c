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
alt_u8 g_EPCS_STATE;


alt_u8 epcs_commands(epcs_reg_t *epcs_area)
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
			case EPCS_START_ADDRESS_ADDR	: epcs_start_address_rg = epcs_area->data; alt_putstr("EPCS_SET_START_ADDR\n");
											  break;
			case EPCS_TOTAL_LENTH_ADDR		: epcs_total_lenth_rg = epcs_area->data;alt_putstr("EPCS_SET_LENGTH\n");
											  break;
			case EPCS_COMMANDS_ADDR    		: epcs_cmd_rg = epcs_area->data; alt_putstr("EPCS_SET_CMD\n");
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
	alt_putstr("EPCS_READ_ID\n");
}

///

alt_u8 epcs_run_cmd(void)
{
	switch (epcs_cmd_rg)
	{
		case EPCS_CMD_NULL				: break;

		case EPCS_CMD_WRITE				: break;

		case EPCS_CMD_READ				: break;

		case EPCS_CMD_ERASE_SECTOR		:
			epcs_sector_erase(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
							  epcs_start_address_rg,
							  0x0);
			epcs_cmd_rg = EPCS_CMD_NULL;
			 alt_putstr("EPCS_SECTOR_ERASE\n");
			break;

		case EPCS_CMD_UPDATE_FIRMWARE	: g_EPCS_STATE = EPCS_STATE_WRITE_FW; break;

		case EPCS_CMD_READ_FIRMWARE		: g_EPCS_STATE = EPCS_STATE_READ_FW;  break;

	}

}

///

void epcs_write_fw(alt_u8*  src_data,alt_u16 len)
{
	static alt_u32 bytes_written;
	alt_u16 n_bytes;
	alt_u16 write_cnt;
	alt_u8  write_buf[1024];
	int i =0;

	n_bytes = len;
	//write_buf = 0;

	for(i=0;i<len;i++) // bit swap
	{
		write_buf[i] = BitReverseTable256[*(src_data+i)];

	}

	while(n_bytes >= 256) // maximum write length
	{
			epcs_write_buffer(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
							  epcs_start_address_rg,
							  &write_buf + write_cnt,
			                  256,
			                  0x0);
			epcs_start_address_rg += 256;
			write_cnt += 256;
			n_bytes -=256;
			bytes_written += 256;
	}
	if(n_bytes != 0) // packet less the 256 bytes
	{
		epcs_write_buffer(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
											  epcs_start_address_rg,
											  &write_buf,
											  n_bytes,
							                  0x0);
		bytes_written += n_bytes;
		epcs_start_address_rg += n_bytes;
		n_bytes = 0;
	}

	if(bytes_written == epcs_total_lenth_rg)
	{
		epcs_cmd_rg = EPCS_CMD_NULL;
		g_EPCS_STATE = EPCS_STATE_IDLE;
		bytes_written = 0;
	}

	alt_putstr("EPCS_WRITE_DATA\n");
}

///

alt_u32 epcs_read_fw(alt_u8*  src_data)
{
	alt_u8  rd_buf[1024];
	int i =0;
	alt_u16 len;



	if(epcs_total_lenth_rg >= 1024) // maximum packe data len
	{
		epcs_read_buffer(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
				epcs_start_address_rg,
				&rd_buf,
				1024,
		        0x0);
		for(i=0;i<1024;i++) // bit swap
		{
			*src_data = BitReverseTable256[rd_buf[i]];
			src_data++;
		}
		epcs_start_address_rg +=1024;
		epcs_total_lenth_rg -= 1024;
		alt_putstr("EPCS_READ_DATA\n");
		return(1024);
	}
	else if(len > 0)
	{
		epcs_read_buffer(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
				epcs_start_address_rg,
				&rd_buf,
				epcs_total_lenth_rg,
				0x0);

				for(i=0;i<epcs_total_lenth_rg;i++) // bit swap
				{
					*src_data = BitReverseTable256[rd_buf[i]];
					src_data++;
				}
				len = epcs_total_lenth_rg;
				epcs_start_address_rg+=len;
				epcs_total_lenth_rg = 0;
				epcs_cmd_rg = EPCS_CMD_NULL;
				g_EPCS_STATE = EPCS_STATE_IDLE;
				alt_putstr("EPCS_READ_DATA\n");
				return(len);
	}

}
