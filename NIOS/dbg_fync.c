/*
 * dbg_fync.c
 *
 *  Created on: 19 мая 2017 г.
 *      Author: artemv
 */

#include "sys/alt_stdio.h"
#include <stdarg.h>
#include "uart_cmd.h"
//#define debug;
#define terminal;



void dbg_putstr(const char* str)
{
#ifdef debug
	alt_putstr(str);
#endif
}


///
#ifdef debug
void dbg_printf(const char *fmt, ...)
{

	va_list args;
	va_start(args, fmt);
    const char *w;
    char c;

    /* Process format string. */
    w = fmt;
    while ((c = *w++) != 0)
    {
        /* If not a format escape character, just print  */
        /* character.  Otherwise, process format string. */
        if (c != '%')
        {
            alt_putchar(c);
        }
        else
        {
            /* Get format character.  If none     */
            /* available, processing is complete. */
            if ((c = *w++) != 0)
            {
                if (c == '%')
                {
                    /* Process "%" escape sequence. */
                    alt_putchar(c);
                }
                else if (c == 'c')
                {
                    int v = va_arg(args, int);
                    alt_putchar(v);
                }
                else if (c == 'x')
                {
                    /* Process hexadecimal number format. */
                    unsigned long v = va_arg(args, unsigned long);
                    unsigned long digit;
                    int digit_shift;

                    /* If the number value is zero, just print and continue. */
                    if (v == 0)
                    {
                        alt_putchar('0');
                        continue;
                    }

                    /* Find first non-zero digit. */
                    digit_shift = 28;
                    while (!(v & (0xF << digit_shift)))
                        digit_shift -= 4;

                    /* Print digits. */
                    for (; digit_shift >= 0; digit_shift -= 4)
                    {
                        digit = (v & (0xF << digit_shift)) >> digit_shift;
                        if (digit <= 9)
                            c = '0' + digit;
                        else
                            c = 'a' + digit - 10;
                        alt_putchar(c);
                    }
                }
                else if (c == 's')
                {
                    /* Process string format. */
                    char *s = va_arg(args, char *);

                    while(*s)
                      alt_putchar(*s++);
                }
            }
            else
            {
                break;
            }
        }
    }

}
#endif

#ifdef terminal

void dbg_printf(const char *fmt, ...)
{

	va_list args;
	va_start(args, fmt);
    const char *w;
    char c;
    char buf[1024];
    alt_u16 i;
    alt_u16 tx_cnt;
    tx_cnt =0;
    i = 0;

    /* Process format string. */
    w = fmt;
    while ((c = *w++) != 0)
    {
        /* If not a format escape character, just print  */
        /* character.  Otherwise, process format string. */
        if (c != '%')
        {
           // alt_putchar(c);
        	buf[i] = c;
        	i++;
        }
        else
        {
            /* Get format character.  If none     */
            /* available, processing is complete. */
            if ((c = *w++) != 0)
            {
                if (c == '%')
                {
                    /* Process "%" escape sequence. */
                    //alt_putchar(c);
                	buf[i] =c;
                	i++;

                }
                else if (c == 'c')
                {
                    int v = va_arg(args, int);
                    //alt_putchar(v);
                    buf[i] =v;
                    i++;
                }
                else if (c == 'x')
                {
                    /* Process hexadecimal number format. */
                    unsigned long v = va_arg(args, unsigned long);
                    unsigned long digit;
                    int digit_shift;

                    /* If the number value is zero, just print and continue. */
                    if (v == 0)
                    {
                        //alt_putchar('0');
                    	buf[i] ='0';
                    	i++;
                        continue;
                    }

                    /* Find first non-zero digit. */
                    digit_shift = 28;
                    while (!(v & (0xF << digit_shift)))
                        digit_shift -= 4;

                    /* Print digits. */
                    for (; digit_shift >= 0; digit_shift -= 4)
                    {
                        digit = (v & (0xF << digit_shift)) >> digit_shift;
                        if (digit <= 9)
                            c = '0' + digit;
                        else
                            c = 'a' + digit - 10;
                        //alt_putchar(c);
                        buf[i] =c;
                        i++;
                    }
                }
                else if (c == 's')
                {
                    /* Process string format. */
                    char *s = va_arg(args, char *);

                    while(*s)
                      //alt_putchar(*s++);
                    	buf[i] = *s++;
                    i++;
                }
            }
            else
            {
                break;
            }
        }
    }

    UartCmd_Sync_Bytes();

	// Package type "ASCII Print"
	UartCmd_WRITE_FIFO_TX_DATA(UartPkgType_ASCII);
	// Lengths of packet = 2 bytes
	UartCmd_WRITE_FIFO_TX_DATA(i);
	UartCmd_WRITE_FIFO_TX_DATA(i>>8);

	while(tx_cnt != i)
	{
		UartCmd_WRITE_FIFO_TX_DATA(buf[tx_cnt]);
		tx_cnt++;
	}

}
#endif
///



