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

#include "libs/USI_UART_config.h"
#include "libs/ws2812.h"

#define PIN_DATA 2 // PB2
#define PIN_POWER 3 // PB3
#define PORT_BANK B // PORTB
#define PORT_LED PB4
#define RGB_COUNT 6


wserr_t init_led(
    __inout wscol_t *ele,
    __in uint16_t ele_idx
    )
{
    wserr_t result = WS_ERR_NONE;

    if(!ele || (ele_idx >= WS_ELE_MAX_COUNT)) {
        result = WS_ERR_INV_ARG;
        goto exit;
    }
    uint16_t temp = ele_idx % 3;
    ele->red = (temp == 0) ? 3:0;
    ele->green = (temp == 1) ? 3:0;
    ele->blue = (temp == 2) ? 3:0;

exit:
    return result;
}

wserr_t
update_led(
    __inout wscol_t *ele,
    __in uint16_t ele_idx,
    __in uint16_t iter
    )
{
    wserr_t result = WS_ERR_NONE;

    if(!ele || (ele_idx >= WS_ELE_MAX_COUNT)) {
        result = WS_ERR_INV_ARG;
        goto exit;
    }
    if ( iter == 0){
        goto exit;
    }
    uint16_t temp = ele_idx % 3;
    ele->red = (temp == 0) ? 3:0;
    ele->green = (temp == 1) ? 3:0;
    ele->blue = (temp == 2) ? 3:0;
exit:
    return result;
}


int main( void )
{
    unsigned char myString[] = "uuuuuu";
    ws2812 cont;
    wserr_t result = WS_ERR_NONE;
    unsigned char counter;
    USI_UART_Flush_Buffers();

    // initialization RGB
    result = ws2812_init(&cont, PORT_BANK, PIN_POWER, PIN_DATA, RGB_COUNT, init_led, 1, update_led);
    if(!WS_ERR_SUCCESS(result)) {
        goto exit;
    }
    USI_UART_Initialise_Receiver();                                         // Initialisation for USI_UART receiver
    sei();                                                   // Enable global interrupts
    for( ; ; )                                                              // Run forever
    {
        if( USI_UART_Data_In_Receive_Buffer() )
        {
            for(counter = 0; counter < 2; counter++)                       // Echo myString[]
            {
                USI_UART_Transmit_Byte(myString[counter]);
            }
            USI_UART_Transmit_Byte( USI_UART_Receive_Byte() );              // Echo the received character      
            result = ws2812_update(&cont, update_led);
        }
        sleep_cpu();
    }

exit:
    for (;;) {
        _delay_ms(300);
        USI_UART_Transmit_Byte('e');
        _delay_ms(300);
    }
    return 1;
}
