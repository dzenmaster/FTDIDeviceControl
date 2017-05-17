
#include "sys/alt_stdio.h"
#include "alt_types.h"
#include "uart_cmd.h"
#include "system.h"
#include "uart_cmd_pkg_process.h"


///

void UartCmd_send_data(alt_u8* data, alt_u8 PkgType,alt_u16 len)
{
	int i;
	UartCmd_Sync_Bytes();
	UartCmd_WRITE_FIFO_TX_DATA(PkgType);
	UartCmd_WRITE_FIFO_TX_DATA(len);
	UartCmd_WRITE_FIFO_TX_DATA(len>>8);

	for(i=0;i<len;i++)
	{
		UartCmd_WRITE_FIFO_TX_DATA(*data);
		data++;
	}

}

///

alt_u16 UartCmd_get_data(alt_u8* data)
{
	alt_u16 pkg_len;
	alt_u16 rcvd_bytes;
	alt_u16 test_data;
	int i;

	rcvd_bytes = UartCmd_FIFO_RX_USEDW_RD();
	pkg_len = UartCmd_get_pkg_len();
		if(rcvd_bytes != pkg_len)
			 alt_putstr("pkg_len and FIFO RX USEDW are not equal!\n");
	 for(i=0; i < pkg_len; i++)
	  {
		 *data  = UartCmd_READ_FIFO_RX_DATA();
		 data++;
	  }
	 return (pkg_len);
}

///

void UartCmd_Sync_Bytes(void)
{
	UartCmd_WRITE_FIFO_TX_DATA(UART_CMD_SYNC_0);
	UartCmd_WRITE_FIFO_TX_DATA(UART_CMD_SYNC_1);
}

///

void UartCmd_Send_Module_type(alt_u8 data)
{
	UartCmd_Sync_Bytes();
	// Package type "Module info"
	UartCmd_WRITE_FIFO_TX_DATA(0x00);
	// Lengths of packet = 2 bytes
	UartCmd_WRITE_FIFO_TX_DATA(0x02);
	UartCmd_WRITE_FIFO_TX_DATA(0x00);
	// Parameter address
	UartCmd_WRITE_FIFO_TX_DATA(UART_CMD_INFO_MODULE_TYPE_ADDR);
	// Parameter data
	UartCmd_WRITE_FIFO_TX_DATA(data);
}

///

void UartCmd_Send_SN(alt_u8 data)
{
	UartCmd_Sync_Bytes();
	// Package type "Module info"
	UartCmd_WRITE_FIFO_TX_DATA(0x00);
	// Lengths of packet = 2 bytes
	UartCmd_WRITE_FIFO_TX_DATA(0x02);
	UartCmd_WRITE_FIFO_TX_DATA(0x00);
	// Parameter address
	UartCmd_WRITE_FIFO_TX_DATA(UART_CMD_INFO_MODULE_SN_ADDR);
	// Parameter data
	UartCmd_WRITE_FIFO_TX_DATA(data);
}

///

void UartCmd_Send_FirmWare_revision(alt_u8 data)
{
	UartCmd_Sync_Bytes();
	// Package type "Module info"
	UartCmd_WRITE_FIFO_TX_DATA(0x00);
	// Lengths of packet = 2 bytes
	UartCmd_WRITE_FIFO_TX_DATA(0x02);
	UartCmd_WRITE_FIFO_TX_DATA(0x00);
	// Parameter address
	UartCmd_WRITE_FIFO_TX_DATA(UART_CMD_INFO_FW_REV_ADDR);
	// Parameter data
	UartCmd_WRITE_FIFO_TX_DATA(data);
}

///

void UartCmd_Send_SoftWare_revision(alt_u8 data)
{
	UartCmd_Sync_Bytes();
	// Package type "Module info"
	UartCmd_WRITE_FIFO_TX_DATA(0x00);
	// Lengths of packet = 2 bytes
	UartCmd_WRITE_FIFO_TX_DATA(0x02);
	UartCmd_WRITE_FIFO_TX_DATA(0x00);
	// Parameter address
	UartCmd_WRITE_FIFO_TX_DATA(UART_CMD_INFO_SW_REV_ADDR);
	// Parameter data
	UartCmd_WRITE_FIFO_TX_DATA(data);
}

/// type = 0x3

void UartCmd_Send_SW_reg_val(alt_u16 addr,alt_u32 reg_val)
{
	UartCmd_Sync_Bytes();
	// Package type "SW register read"
	UartCmd_WRITE_FIFO_TX_DATA(PKG_TYPE_NIOS_RG);
	// Lengths of packet = 7 bytes
	UartCmd_WRITE_FIFO_TX_DATA(0x07);
	UartCmd_WRITE_FIFO_TX_DATA(0x00);
	// flag RD
	UartCmd_WRITE_FIFO_TX_DATA(0x1);
	// address
	UartCmd_WRITE_FIFO_TX_DATA(addr);
	UartCmd_WRITE_FIFO_TX_DATA(addr>>8);
	//data
	UartCmd_WRITE_FIFO_TX_DATA(reg_val);
	UartCmd_WRITE_FIFO_TX_DATA(reg_val>>8);
	UartCmd_WRITE_FIFO_TX_DATA(reg_val>>16);
	UartCmd_WRITE_FIFO_TX_DATA(reg_val>>24);

}

///

void UartCmd_Send_Stream(alt_u8* data,alt_u16 len)
{
	alt_u8  uart_status = 0;
	alt_u16 tx_bytes = 0;

	tx_bytes = len;

	//wait for free space for sync words in FIFO
	while ((UartCmd_FIFO_TX_USEDW_RD()) > 1000);

	UartCmd_Sync_Bytes();

	// Package type "Stream data"
	UartCmd_WRITE_FIFO_TX_DATA(PKG_TYPE_SREAM_DATA);
	// Lengths of packet
	UartCmd_WRITE_FIFO_TX_DATA(len);
	UartCmd_WRITE_FIFO_TX_DATA(len >> 8);

	while (tx_bytes > 0)
	{
		uart_status = UartCmd_GET_STATUS();
		if((uart_status & 0x8)>>3 == 0); // fifo is not full
		{
			UartCmd_WRITE_FIFO_TX_DATA(*data);
			data++;
			tx_bytes--;
		}
	}
}


