/* 
 * "Small Hello World" example. 
 * 
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example 
 * designs. It requires a STDOUT  device in your system's hardware. 
 *
 * The purpose of this example is to demonstrate the smallest possible Hello 
 * World application, using the Nios II HAL library.  The memory footprint
 * of this hosted application is ~332 bytes by default using the standard 
 * reference design.  For a more fully featured Hello World application
 * example, see the example titled "Hello World".
 *
 * The memory footprint of this example has been reduced by making the
 * following changes to the normal "Hello World" example.
 * Check in the Nios II Software Developers Manual for a more complete 
 * description.
 * 
 * In the SW Application project (small_hello_world):
 *
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 * In System Library project (small_hello_world_syslib):
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 *    - Define the preprocessor option ALT_NO_INSTRUCTION_EMULATION 
 *      This removes software exception handling, which means that you cannot 
 *      run code compiled for Nios II cpu with a hardware multiplier on a core 
 *      without a the multiply unit. Check the Nios II Software Developers 
 *      Manual for more details.
 *
 *  - In the System Library page:
 *    - Set Periodic system timer and Timestamp timer to none
 *      This prevents the automatic inclusion of the timer driver.
 *
 *    - Set Max file descriptors to 4
 *      This reduces the size of the file handle pool.
 *
 *    - Check Main function does not exit
 *    - Uncheck Clean exit (flush buffers)
 *      This removes the unneeded call to exit when main returns, since it
 *      won't.
 *
 *    - Check Don't use C++
 *      This builds without the C++ support code.
 *
 *    - Check Small C library
 *      This uses a reduced functionality C library, which lacks  
 *      support for buffering, file IO, floating point and getch(), etc. 
 *      Check the Nios II Software Developers Manual for a complete list.
 *
 *    - Check Reduced device drivers
 *      This uses reduced functionality drivers if they're available. For the
 *      standard design this means you get polled UART and JTAG UART drivers,
 *      no support for the LCD driver and you lose the ability to program 
 *      CFI compliant flash devices.
 *
 *    - Check Access device drivers directly
 *      This bypasses the device file system to access device drivers directly.
 *      This eliminates the space required for the device file system services.
 *      It also provides a HAL version of libc services that access the drivers
 *      directly, further reducing space. Only a limited number of libc
 *      functions are available in this configuration.
 *
 *    - Use ALT versions of stdio routines:
 *
 *           Function                  Description
 *        ===============  =====================================
 *        alt_printf       Only supports %s, %x, and %c ( < 1 Kbyte)
 *        alt_putstr       Smaller overhead than puts with direct drivers
 *                         Note this function doesn't add a newline.
 *        alt_putchar      Smaller overhead than putchar with direct drivers
 *        alt_getchar      Smaller overhead than getchar with direct drivers
 *
 */

#include "sys/alt_stdio.h"
#include "system.h"
#include "io.h"
#include "alt_types.h"
#include "uart_cmd.h"
#include "uart_cmd_pkg_process.h"
#include "epcs_flash.h"



#define		AVS_READ_SATATUS_ADDR	  		0x0 << 2
#define		AVS_READ_USEDW_RX_FIFO_ADDR	  	0x1 << 2
#define		AVS_READ_USEDW_TX_FIFO_ADDR	  	0x2 << 2
#define		AVS_READ_UART_DATA_ADDR	  		0x3 << 2
#define		AVS_WRITE_UART_DATA_ADDR	  	0x4 << 2
#define		AVS_PACKAGE_TYPE_LEN_RD_ADDR	0x5 << 2

volatile  alt_u16 pkg_len;
volatile  alt_u8  pkg_type;
volatile  alt_u8  rd_data[1024];
volatile  alt_u8  wr_data[1024];

alt_u32  uart_status = 0;
alt_u8 cnt = 0;
int i;

alt_u8 state = 0;

int main()
{ 
  alt_putstr("Hello from Nios II!\n");
//  epcs_read_buffer(EPCS_FLASH_CONTROLLER_0_BASE+ EPCS_FLASH_CONTROLLER_0_REGISTER_OFFSET, 0x0,&rd_data, 1024,0x0);
  /* Event loop never exits. */
  while (1)
  {
	do
	 {
		 uart_status = UartCmd_GET_STATUS();
		 if((uart_status & 0x2)>>1)
		 {
			 alt_putstr("Timeout has been occurred!\n");
			 send_response_packet(NIOS_CMD_TIMEOUT_OCCURRED);
		 }

		 if((uart_status & 0x4)>>2)
		 {
			 alt_putstr("Wrong Sync Word!\n");
			 send_response_packet(NIOS_CMD_WRONG_SYNC);
		 }
	 }
	 while ((uart_status & 0x1) == 0);

 	 pkg_type = UartCmd_get_pkg_type();

	 switch (pkg_type)
	 {
	 	 case 0x0 : process_info_packet();		 	 break;
	 	 case 0x1 :	process_CurState_packet(0x0); 	 break;
	 	 case 0x3 : process_SW_registers();	 break;
	 	 case 0x4 : if(g_STATE_EPCS_UPD_FW == 1)
	 		 	 	 	 process_epcs_flash(0x1);
	 		 	 	break;
	 	default:
	 	{
	 		alt_putstr("Unknown Packet Type\n");
	 		send_response_packet(NIOS_CMD_UNKN_PKG_TYPE);
	 	}
	 }




  }



  return 0;
}
