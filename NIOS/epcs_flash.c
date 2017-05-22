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
			case EPCS_APL_FROM_ADDR		: 	  UartCmd_Send_SW_reg_val(EPCS_APL_FROM_ADDR,EPCS_APL_BOOT_ADDR);
											  break;

			default : 	return(EPCS_ERR);
		}
	}
	else
	{
		switch (epcs_area->addr)
		{
			case EPCS_ID_ADDR 				: return(EPCS_ERR);//send_response_packet(NIOS_CMD_SW_RG_WRITE_ERR); // only read register
											  break;

			case EPCS_START_ADDRESS_ADDR	:  if(epcs_area->data < EPCS_APL_BOOT_ADDR){
													 dbg_printf("[EPCS] Wrong start address-> %x \n\r", epcs_area->data);
													 dbg_printf("[EPCS] Need to set ADDR from -> %x \n\r", EPCS_APL_BOOT_ADDR);
													return(EPCS_ERR);
												}
											  else {
												  epcs_start_address_rg = epcs_area->data;
												  dbg_printf("[EPCS] Set start address -> %x \n\r", epcs_start_address_rg);
											  }
											  break;

			case EPCS_TOTAL_LENTH_ADDR		: epcs_total_lenth_rg = epcs_area->data; dbg_printf("[EPCS] Set length -> %x \n\r", epcs_total_lenth_rg);
											  break;

			case EPCS_COMMANDS_ADDR    		: epcs_cmd_rg = epcs_area->data; dbg_printf("[EPCS] Set command -> %x \n\r",epcs_cmd_rg );
											  epcs_run_cmd();
											  break;

			case EPCS_APL_FROM_ADDR		    : return(EPCS_ERR);




			default : 	return(EPCS_ERR);
		}

	}

	return(EPCS_OK);
}

///

void epcs_read_flash_id(void)
{

	epcs_id_rg = epcs_read_electronic_signature(EPCS_FLASH_CONTROLLER_0_BASE+
											 	EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET);

	UartCmd_Send_SW_reg_val(EPCS_ID_ADDR,epcs_id_rg);
	dbg_printf("[EPCS] Read Flash ID -> %x\n\r",epcs_id_rg);
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
			dbg_printf("[EPCS] SECTOR ERASE \n\r");
			break;

		case EPCS_CMD_UPDATE_FIRMWARE	: g_EPCS_STATE = EPCS_STATE_WRITE_FW; break;

		case EPCS_CMD_READ_FIRMWARE		: g_EPCS_STATE = EPCS_STATE_READ_FW;  break;

	}

}

///

alt_u8 epcs_write_fw(alt_u8*  src_data,alt_u16 len)
{
	alt_u32 bytes_written;
	alt_u16 n_bytes;
	alt_u16 write_cnt;
	alt_u8  write_buf[1024];
	static alt_u32 total_bytes;

	int i =0;

	write_cnt = 0;
	n_bytes = len;

	bytes_written = 0;

	if(n_bytes == 0 || epcs_start_address_rg < EPCS_APL_BOOT_ADDR)
	{
		dbg_printf("[EPCS] Error \n\r");
		return(EPCS_ERR);
	}
	else
	{
		for(i=0;i<len;i++) // reverse byte requires for Altera
			{
				write_buf[i] = *src_data;
				write_buf[i] = BitReverseTable256[write_buf[i]];
				src_data++;

			}

			while(n_bytes >= 256) // maximum write length
			{
					epcs_write_buffer(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
									  epcs_start_address_rg,
									  &write_buf[bytes_written],
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
													  &write_buf[bytes_written],
													  n_bytes,
									                  0x0);
				bytes_written += n_bytes;
				epcs_start_address_rg += n_bytes;
				n_bytes = 0;
			}
			total_bytes+=bytes_written;

			if(total_bytes == epcs_total_lenth_rg)
			{
				epcs_cmd_rg = EPCS_CMD_NULL;
				g_EPCS_STATE = EPCS_STATE_IDLE;
				dbg_printf("[EPCS] Bytes written -> %x \n\r", total_bytes);
				dbg_printf("[EPCS] Wire done \n\r");
				total_bytes = 0;
			}
			else
			{
				dbg_printf("[EPCS] Bytes written -> %x \n\r", total_bytes);
			}


			return(EPCS_OK);
	}

}

///

alt_u32 epcs_read_fw(alt_u8*  src_data)
{
	alt_u8  rd_buf[1024];
	int i =0;
	alt_u16 last_len;
	alt_u8  *rd_buf_xp;

	rd_buf_xp = &rd_buf[0];

	if(epcs_total_lenth_rg >= 1024) // maximum packet data len
	{
		epcs_read_buffer(EPCS_FLASH_CONTROLLER_0_BASE+EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET,
				epcs_start_address_rg,
				&rd_buf[0],
				1024,
		        0x0);
		for(i=0;i<1024;i++) // bit swap
		{
			rd_buf[i]= BitReverseTable256[rd_buf[i]];
			*src_data = rd_buf[i];
			src_data++;
			rd_buf_xp++;
		}

		dbg_printf("[EPCS] Read data: start address -> %x; Len - > %x \n\r", epcs_start_address_rg,1024);
		epcs_start_address_rg +=1024;
		epcs_total_lenth_rg -= 1024;

		if(epcs_total_lenth_rg == 0)
		{
			epcs_cmd_rg = EPCS_CMD_NULL;
			g_EPCS_STATE = EPCS_STATE_IDLE;
			dbg_printf("[EPCS] Read Done \n\r");
		}
		return(1024);
	}
	else if(epcs_total_lenth_rg > 0)
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
				dbg_printf("[EPCS] Read data: start address -> %x Len - > \n\r", epcs_start_address_rg,epcs_total_lenth_rg);
				last_len = epcs_total_lenth_rg;
				epcs_start_address_rg+=last_len;
				epcs_total_lenth_rg = 0;
				epcs_cmd_rg = EPCS_CMD_NULL;
				g_EPCS_STATE = EPCS_STATE_IDLE;
				dbg_printf("[EPCS] Read Done \n\r");
				return(last_len);
	}
	else 	return(0);


}
