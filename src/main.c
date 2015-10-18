/*****************************************************************************
*
* Copyright (C) 2003 Atmel Corporation
*
* File          : main.c
* Compiler      : IAR EWAAVR 2.28a
* Created       : 18.07.2002 by JLL
* Modified      : 11-07-2003 by LTA
*
* Support mail  : avr@atmel.com
*
* AppNote       : AVR307 - Half duplex UART using the USI Interface
*
* Description   : Example showing how you could use the USI_UART drivers
*
*
****************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "USI_UART_config.h"

int main( void )
{
    unsigned char myString[] = "uuuuuu";
  
    
    unsigned char counter;
    
    USI_UART_Flush_Buffers();
    USI_UART_Initialise_Receiver();                                         // Initialisation for USI_UART receiver
    sei();                                                   // Enable global interrupts
    MCUCR = (1<<SE)|(0<<SM1)|(0<<SM0);                                      // Enable Sleepmode: Idle
    for( ; ; )                                                              // Run forever
    {
        if( USI_UART_Data_In_Receive_Buffer() )
        {
            for(counter = 0; counter < 2; counter++)                       // Echo myString[]
            {
                USI_UART_Transmit_Byte(myString[counter]);
            }
            USI_UART_Transmit_Byte( USI_UART_Receive_Byte() );              // Echo the received character      
        }
        sleep_cpu();
    }
}
