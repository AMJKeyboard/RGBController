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
#include <avr/pgmspace.h>

#include <stdlib.h>
#include <util/delay.h>

#include "libs/USI_UART_config.h"
#include "libs/ws2812.h"

#define PIN_DATA 2 // PB2
#define PIN_POWER 3 // PB3
#define PORT_BANK B // PORTB
#define PORT_LED PB4

#define RGB_COUNT 16
#define MESSAGEBUF_SIZE 48 // (3 * RGB_COUNT)

// {{{ var

ws2812 cont;

static const uint8_t rainbow[21] PROGMEM = {
    0x8B, 0,    0,      // red
    0xFF, 0xA5, 0,      // orange
    0xFF, 0xFF, 0,      // yellow
    0,    0x80, 0,      // green
    0,    0xFF, 0xFF,   // cyan
    0,    0,    0xFF,   // blue
    0x80, 0,    0x80    // purple
};

static const uint8_t reduce = 15;

static uint8_t spec_value = 10;

static uint8_t messageBuf[MESSAGEBUF_SIZE];

uint16_t RGB_TEMP = 0;
uint8_t RGB_MAXIMUM = 0;
uint8_t RGB_DIRECTION = 0;

volatile int overflow_count;

wserr_t (*UPDATE_EFFECT_FUN)(__inout wscol_t *ele, __in uint16_t ele_idx, __in uint16_t iter);

void commander( void );
void InitTimer1( void );

// }}}


// {{{ rgb update

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
    ele->red = spec_value;
    ele->green = spec_value;
    ele->blue = spec_value;

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
    ele->red = spec_value;
    ele->green = spec_value;
    ele->blue = spec_value;
exit:
    return result;
}


wserr_t
rand_update_led(
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
    uint16_t temp = (ele_idx + rand()) % 7;
    ele->red = pgm_read_byte(&rainbow[temp * 3]) / reduce;
    ele->green = pgm_read_byte(&rainbow[temp * 3 + 1]) / reduce;
    ele->blue = pgm_read_byte(&rainbow[temp * 3 + 2]) / reduce;
exit:
    return result;
}

wserr_t
update_rgb_turnover(
    __inout wscol_t *ele,
    __in uint16_t ele_idx,
    __in uint16_t iter
    )
{
    uint8_t temp, data_idx;
    wserr_t result = WS_ERR_NONE;

    if(!ele || (ele_idx >= WS_ELE_MAX_COUNT)) {
        result = WS_ERR_INV_ARG;
        goto exit;
    }
    data_idx = (ele_idx + (RGB_TEMP % RGB_COUNT)) % RGB_COUNT;
    temp = ((data_idx+1)*3);
    ele->red = messageBuf[temp+1];
    ele->green = messageBuf[temp+2];
    ele->blue = messageBuf[temp+3];
exit:
    return result;
}

wserr_t
update_rgb_null(
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
    // keep null
exit:
    return result;
}

// }}}


// {{{ main
int main( void ) {
    MCUCR |= _BV(BODS);
    wserr_t result = WS_ERR_NONE;
    USI_UART_Flush_Buffers();

    // initialization RGB
    result = ws2812_init(&cont, PORT_BANK, PIN_POWER, PIN_DATA, RGB_COUNT, init_led, 0, NULL);
    if(!WS_ERR_SUCCESS(result)) {
        goto exit;
    }

    InitTimer1();
    USI_UART_Initialise_Receiver();                                         // Initialisation for USI_UART receiver
    sei();                                                   // Enable global interrupts
    USI_UART_Transmit_Byte('B');
    _delay_us(10);
    for( ; ; )                                                              // Run forever
    {
        if( USI_UART_Data_In_Receive_Buffer() )
        {
            commander();
        }
        _delay_us(20);
    }

exit:
    for (;;) {
        _delay_ms(300);
        USI_UART_Transmit_Byte('X');
        _delay_ms(300);
    }
    return 1;
}

// }}}


// {{{ timer1
void InitTimer1() {
    overflow_count = 0;
    GTCCR |= _BV(PSR1);
    TCNT1 = 0;
    TCCR1 = 0;

}

ISR(TIM1_OVF_vect) {
    if (overflow_count >= 3) {
        ws2812_update(&cont, *UPDATE_EFFECT_FUN);
        overflow_count = 0;
    } else {
        overflow_count ++;
    }
    TCNT1 = 0;
}
// }}}


// {{{ commander
void commander() {
    unsigned char cmd = USI_UART_Receive_Byte();
    cli();
    switch(cmd){
        case '0':
            ws2812_off(&cont);
            TIMSK &= ~_BV(TOIE1);
            TCCR1 = 0;
            spec_value = 10;
            break;
        case '1':
            ws2812_on(&cont, update_led);
            break;
        case '2':
            ws2812_update(&cont, rand_update_led);
            break;
        case '3':
            UPDATE_EFFECT_FUN = rand_update_led;
            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case '4':
            TIMSK &= ~_BV(TOIE1);
            TCCR1 = 0;
            break;
        case '5':
            spec_value = 15;
            break;
        case '6':
            spec_value = 25;
            break;
        case '7':
            spec_value = 30;
            break;
        default:
            ws2812_update(&cont, update_led);
            break;
    }
    sei();
    USI_UART_Transmit_Byte(cmd);
    _delay_us(10);
}

// }}}
