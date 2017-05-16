#include "system.h"
#include "io.h"


#define		AVS_READ_SATATUS_ADDR	  		0x0 << 2
#define		AVS_READ_USEDW_RX_FIFO_ADDR	  	0x1 << 2
#define		AVS_READ_USEDW_TX_FIFO_ADDR	  	0x2 << 2
#define		AVS_READ_UART_DATA_ADDR	  		0x3 << 2
#define		AVS_WRITE_UART_DATA_ADDR	  	0x4 << 2
#define		AVS_PACKAGE_TYPE_LEN_RD_ADDR	0x5 << 2


#define		UartCmd_get_pkg_len()			(IORD_32DIRECT(AVS_UART_CMD_0_BASE, AVS_PACKAGE_TYPE_LEN_RD_ADDR) & 0xFFF)
#define		UartCmd_get_pkg_type()			(IORD_32DIRECT(AVS_UART_CMD_0_BASE, AVS_PACKAGE_TYPE_LEN_RD_ADDR) >> 12)
#define		UartCmd_FIFO_RX_USEDW_RD()	 	 IORD_32DIRECT(AVS_UART_CMD_0_BASE, AVS_READ_USEDW_RX_FIFO_ADDR)
#define		UartCmd_FIFO_TX_USEDW_RD()	 	 IORD_32DIRECT(AVS_UART_CMD_0_BASE, AVS_READ_USEDW_TX_FIFO_ADDR)
#define		UartCmd_READ_FIFO_RX_DATA()	 	 IORD_32DIRECT(AVS_UART_CMD_0_BASE, AVS_READ_UART_DATA_ADDR)
#define		UartCmd_WRITE_FIFO_TX_DATA(data) IOWR_32DIRECT(AVS_UART_CMD_0_BASE, AVS_WRITE_UART_DATA_ADDR,data)
#define		UartCmd_GET_STATUS()			 IORD_32DIRECT(AVS_UART_CMD_0_BASE, AVS_READ_SATATUS_ADDR)


///

#define		UART_CMD_SYNC_0 	0xA5
#define		UART_CMD_SYNC_1 	0x5A

///

#define		UART_CMD_INFO_MODULE_TYPE_ADDR 	0x0
#define		UART_CMD_INFO_MODULE_SN_ADDR   	0x1
#define		UART_CMD_INFO_FW_REV_ADDR   	0x2
#define		UART_CMD_INFO_SW_REV_ADDR   	0x3


alt_u16 UartCmd_get_data(alt_u8*data);
void UartCmd_Sync_Bytes(void);
void UartCmd_Send_Module_type(alt_u8 data);
void UartCmd_Send_SN(alt_u8 data);
void UartCmd_Send_FirmWare_revision(alt_u8 data);
void UartCmd_Send_SoftWare_revision(alt_u8 data);
void UartCmd_Send_SW_reg_val(alt_u16 addr,alt_u32 reg_val);
