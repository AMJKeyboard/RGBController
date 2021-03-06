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
#define COLORS_BIT_SIZE 48


// {{{ var

enum Rgb_Status {
    SUNINIT=0,
    SINITED,
    SPOWERON,
    SPOWEROFF
} rgb_status;

enum Cmd_Flag {
    FUNUSED=0,
    FINITLED,
    FLEDCOLOR,
    FLEDMODE,
    FLEDLIGHT,
    FFWINFO,
    FEXTCMD
};

enum Ext_Cmd {
    EUNUSED=0,
    ELEDON,
    ELEDOFF,
    ELEDREFRESH,
    ETIMER_START,
    ETIMER_STOP,
    EINTHIGHT,
    EINTLOW,
    ELAST
};

enum Mode_List {
    MUNUSED=0,
    MBRIG1,
    MBRIG2,
    MBRIG3,
    MBREA1,
    MBREA2,
    MBREA3,
    MRUN1,
    MRUN2,
    MRUN3,
    MTURN1,
    MTURN2,
    MTURN3,
    MRAND1,
    MRAND2,
    MRAND3,
    MLAST
} mode_spec;

enum Color_List {
    CUNUSED,
    CSWHITE,
    CSRED,
    CSORANGE,
    CSYELLOW,
    CSGREEN,
    CSCYAN,
    CSBLUE,
    CSPURPLE,
    CSRANDOM,
    CMRANDOM,
    CMORDERED,
    CMFULLRAND,
    CLAST
} color_spec;

ws2812 cont;

struct Rgb_Color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_color;


static const uint8_t rainbow[21] PROGMEM = {
    0x8B, 0,    0,      // red
    0xFF, 0xA5, 0,      // orange
    0xFF, 0xFF, 0,      // yellow
    0,    0x80, 0,      // green
    0,    0xFF, 0xFF,   // cyan
    0,    0,    0xFF,   // blue
    0x80, 0,    0x80    // purple
};

static uint8_t reduce = 4;

const uint8_t DIVISOR_MAX = 10;

static uint8_t divisor = 5;
static uint8_t spec_value = 5;

static uint8_t colors_buf[COLORS_BIT_SIZE];

uint16_t color_sort_start = 0;

uint8_t OVERFLOW_MAX = 3;
volatile int overflow_count = 0;
uint8_t MODE_VALUE_CHNAGE_MAX = UINT8_MAX;
uint8_t MODE_VALUE_DIRECTION = 0; // 0:down 1:up
volatile int mode_value_change_count = 0;
volatile int cur_mode = MBRIG1;

uint8_t TCNT1_INIT_VALUE = 0;


wserr_t (*UPDATE_RGB_EFFECT_FUN)(__inout wscol_t *ele, __in uint16_t ele_idx, __in uint16_t iter);
void (*UPDATE_MODE_VALUE_CHANGE_FUN)(void);

void Commander( void );
void InitTimer1( void );

uint8_t ExtCommand(uint8_t ext);
uint8_t SelectColor(uint8_t color);
uint8_t SelectMode(uint8_t mode);
void SetSingleColor(uint8_t color);
void mode_value_change_null(void);
void mode_value_change_bea(void);
void colors_buf_fill(void);
void colors_buf_fill_spec(void);
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
spec_update_led(
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
    ele->red = rgb_color.red / (reduce * divisor);
    ele->green = rgb_color.green / (reduce * divisor);
    ele->blue = rgb_color.blue / (reduce * divisor);
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
    data_idx = (ele_idx + (color_sort_start % RGB_COUNT)) % RGB_COUNT;
    temp = data_idx * 3;
    ele->red = colors_buf[temp] / reduce;
    ele->green = colors_buf[temp+1] / reduce;
    ele->blue = colors_buf[temp+2] / reduce;
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

    CLKPR = _BV(CLKPCE);
    CLKPR = 0;

    DDRB |= _BV(PORT_LED);
    PORTB &= ~_BV(PORT_LED);

    rgb_status = SUNINIT;
    wserr_t result = WS_ERR_NONE;
    USI_UART_Flush_Buffers();


    // initialization RGB
    result = ws2812_init(&cont, PORT_BANK, PIN_POWER, PIN_DATA, RGB_COUNT, init_led, 0, NULL);
    if(!WS_ERR_SUCCESS(result)) {
        goto exit;
    }
    rgb_status = SINITED;

    divisor = 5;
    UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_null;

    color_spec = CSWHITE;
    SetSingleColor(color_spec);

    InitTimer1();
    USI_UART_Initialise_Receiver();                                         // Initialisation for USI_UART receiver
    sei();                                                   // Enable global interrupts

    USI_UART_Transmit_Byte(0xAA);
    PORTB |= _BV(PORT_LED);

    _delay_us(5);
    for( ; ; )                                                              // Run forever
    {
        if( USI_UART_Data_In_Receive_Buffer() )
        {
            Commander();
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
    if (overflow_count >= OVERFLOW_MAX) {
        ws2812_update(&cont, *UPDATE_RGB_EFFECT_FUN);
        overflow_count = 0;
    } else {
        overflow_count ++;
    }
    if (mode_value_change_count >= MODE_VALUE_CHNAGE_MAX){
        (*UPDATE_MODE_VALUE_CHANGE_FUN)();
        mode_value_change_count = 0;
    } else {
        mode_value_change_count ++;
    }
    TCNT1 = TCNT1_INIT_VALUE;
}
// }}}


// {{{ select color

uint8_t SelectColor(uint8_t color){
    uint8_t ret = 0;
    if (color <= CUNUSED || color >= CLAST) {
        return ret;
    }
    ret = 1;
    if ( color >= CSWHITE && color <= CSRANDOM) {
        color_spec = color;
        SetSingleColor(color);
        return ret;

    }
    // Not Implemented
    switch(color) {
        case CSRANDOM:
            break;
        case CMRANDOM:
            break;
        case CMORDERED:
            break;
        case CMFULLRAND:
            break;
    }
    return ret;
}

void SetSingleColor(uint8_t color) {
    switch (color) {
        case CSWHITE:
            rgb_color.red = UINT8_MAX;
            rgb_color.green = UINT8_MAX;
            rgb_color.blue = UINT8_MAX;
            break;
        case CSRED:
            rgb_color.red = pgm_read_byte(&rainbow[0]);
            rgb_color.green = pgm_read_byte(&rainbow[1]);
            rgb_color.blue = pgm_read_byte(&rainbow[2]);
            break;
        case CSORANGE:
            rgb_color.red = pgm_read_byte(&rainbow[3]);
            rgb_color.green = pgm_read_byte(&rainbow[4]);
            rgb_color.blue = pgm_read_byte(&rainbow[5]);
            break;
        case CSYELLOW:
            rgb_color.red = pgm_read_byte(&rainbow[6]);
            rgb_color.green = pgm_read_byte(&rainbow[7]);
            rgb_color.blue = pgm_read_byte(&rainbow[8]);
            break;
        case CSGREEN:
            rgb_color.red = pgm_read_byte(&rainbow[9]);
            rgb_color.green = pgm_read_byte(&rainbow[10]);
            rgb_color.blue = pgm_read_byte(&rainbow[11]);
            break;
        case CSCYAN:
            rgb_color.red = pgm_read_byte(&rainbow[12]);
            rgb_color.green = pgm_read_byte(&rainbow[13]);
            rgb_color.blue = pgm_read_byte(&rainbow[14]);
            break;

        case CSBLUE:
            rgb_color.red = pgm_read_byte(&rainbow[15]);
            rgb_color.green = pgm_read_byte(&rainbow[16]);
            rgb_color.blue = pgm_read_byte(&rainbow[17]);
            break;
        case CSPURPLE:
            rgb_color.red = pgm_read_byte(&rainbow[18]);
            rgb_color.green = pgm_read_byte(&rainbow[19]);
            rgb_color.blue = pgm_read_byte(&rainbow[20]);
            break;
        case CSRANDOM:
            rgb_color.red = rand() % 255;
            rgb_color.green = rand() % 255;
            rgb_color.blue = rand() % 255;
            break;
    }

}

// }}}


// {{{ select mode

void mode_value_change_null(void) {
    // null function
}

void mode_value_change_turn(void) {
    color_sort_start ++;
    if (color_sort_start == 32 ){
        color_sort_start = 0;
    }
}

void mode_value_change_bea(void) {
    if (MODE_VALUE_DIRECTION) {
        divisor++;
    }
    else {
        divisor--;
    }
    if (divisor <= 1 ) {
        MODE_VALUE_DIRECTION = 1;
    }
    else if (divisor >= DIVISOR_MAX) {
        MODE_VALUE_DIRECTION = 0;
    }
}

uint8_t SelectMode(uint8_t mode){
    uint8_t ret = 0;

    if ( mode <= MUNUSED || mode >= MLAST) {
        return ret;
    }

    ret = 1;

    // diable old mode
    TIMSK &= ~_BV(TOIE1);
    TCCR1 = 0;
    TCNT1_INIT_VALUE = 0;
    mode_spec = mode;

    switch (mode) {
        case MBRIG1:
            divisor = 5;
            UPDATE_RGB_EFFECT_FUN = spec_update_led;
            ws2812_update(&cont, *UPDATE_RGB_EFFECT_FUN);
            break;
        case MBRIG2:
            divisor = 2;
            UPDATE_RGB_EFFECT_FUN = spec_update_led;
            ws2812_update(&cont, *UPDATE_RGB_EFFECT_FUN);
            break;
        case MBRIG3:
            divisor = 1;
            UPDATE_RGB_EFFECT_FUN = spec_update_led;
            ws2812_update(&cont, *UPDATE_RGB_EFFECT_FUN);
            break;

        case MBREA1:
            OVERFLOW_MAX = 0;
            MODE_VALUE_CHNAGE_MAX = 0;
            TCNT1_INIT_VALUE = 200;

            UPDATE_RGB_EFFECT_FUN = spec_update_led;
            UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_bea;

            divisor = DIVISOR_MAX;
            MODE_VALUE_DIRECTION = 0;

            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case MBREA2:
            OVERFLOW_MAX = 0;
            MODE_VALUE_CHNAGE_MAX = 0;
            TCNT1_INIT_VALUE = 100;

            UPDATE_RGB_EFFECT_FUN = spec_update_led;
            UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_bea;

            divisor = DIVISOR_MAX;
            MODE_VALUE_DIRECTION = 0;

            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case MBREA3:
            OVERFLOW_MAX = 0;
            MODE_VALUE_CHNAGE_MAX = 0;
            TCNT1_INIT_VALUE = 0;

            UPDATE_RGB_EFFECT_FUN = spec_update_led;
            UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_bea;

            divisor = DIVISOR_MAX;
            MODE_VALUE_DIRECTION = 0;

            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case MRUN1:
            colors_buf_fill_spec();

            OVERFLOW_MAX = 0;
            MODE_VALUE_CHNAGE_MAX = 1;
            TCNT1_INIT_VALUE = 192;

            UPDATE_RGB_EFFECT_FUN = update_rgb_turnover;
            UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_turn;
            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case MRUN2:
            colors_buf_fill_spec();

            OVERFLOW_MAX = 0;
            MODE_VALUE_CHNAGE_MAX = 1;
            TCNT1_INIT_VALUE = 128;

            UPDATE_RGB_EFFECT_FUN = update_rgb_turnover;
            UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_turn;
            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case MRUN3:
            colors_buf_fill_spec();

            OVERFLOW_MAX = 1;
            MODE_VALUE_CHNAGE_MAX = 1;
            TCNT1_INIT_VALUE = 0;

            UPDATE_RGB_EFFECT_FUN = update_rgb_turnover;
            UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_turn;
            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;

        case MTURN1:
            colors_buf_fill();
            OVERFLOW_MAX = 1;
            MODE_VALUE_CHNAGE_MAX = 1;
            TCNT1_INIT_VALUE = 0;

            UPDATE_RGB_EFFECT_FUN = update_rgb_turnover;
            UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_turn;

            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));

            break;
        case MTURN2:
            colors_buf_fill();
            OVERFLOW_MAX = 2;
            MODE_VALUE_CHNAGE_MAX = 2;
            TCNT1_INIT_VALUE = 0;

            UPDATE_RGB_EFFECT_FUN = update_rgb_turnover;
            UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_turn;

            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case MTURN3:
            colors_buf_fill();
            OVERFLOW_MAX = 4;
            MODE_VALUE_CHNAGE_MAX = 4;
            TCNT1_INIT_VALUE = 0;

            UPDATE_RGB_EFFECT_FUN = update_rgb_turnover;
            UPDATE_MODE_VALUE_CHANGE_FUN = mode_value_change_turn;

            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case MRAND1:
            OVERFLOW_MAX = 0;
            TCNT1_INIT_VALUE = 100;
            UPDATE_RGB_EFFECT_FUN = rand_update_led;
            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case MRAND2:
            OVERFLOW_MAX = 1;
            UPDATE_RGB_EFFECT_FUN = rand_update_led;
            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        case MRAND3:
            OVERFLOW_MAX = 3;
            UPDATE_RGB_EFFECT_FUN = rand_update_led;
            TIMSK |= _BV(TOIE1);
            TCCR1 |= (_BV(CS11) | _BV(CS12) | _BV(CS13));
            break;
        default:
            ret = 0;
    }

    return ret;

}

void colors_buf_fill(void){
    uint8_t max_src_len = sizeof(rainbow);
    uint8_t colors_bit_size = sizeof(colors_buf);
    uint8_t src_addr = 0;
    for (int i=0; i < colors_bit_size; i++) {
        src_addr = i % max_src_len;
        colors_buf[i] = pgm_read_byte(&rainbow[src_addr]);
    }
}

void colors_buf_fill_spec(void){
    uint8_t colors_bit_size = sizeof(colors_buf);
    for (int i=0; i < colors_bit_size; i++) {
        colors_buf[i] = 0;
    }
    colors_buf[0] = rgb_color.red;
    colors_buf[1] = rgb_color.green;
    colors_buf[2] = rgb_color.blue;

    colors_buf[24] = rgb_color.red;
    colors_buf[25] = rgb_color.green;
    colors_buf[26] = rgb_color.blue;
}

// }}}


// {{{ commander
void Commander() {
    uint8_t cmd, data, ret = 0;
    uint8_t rev = USI_UART_Receive_Byte();
    cmd = rev & 0b111;
    USI_UART_Transmit_Byte(cmd);
    data = rev >> 4; // FIXME 5bit !
    switch(cmd){
        case FINITLED:
            // Not Implemented
            break;
        case FLEDCOLOR:
            ret = SelectColor(data);
            if (cur_mode < MRUN1){
                UPDATE_RGB_EFFECT_FUN = spec_update_led;
                ws2812_update(&cont, *UPDATE_RGB_EFFECT_FUN);
            }
            break;
        case FLEDMODE:
            if (rgb_status == SPOWERON){
                ret = SelectMode(data);
            }
            break;
        case FLEDLIGHT:
            if (data > 0 && data <= 0xF){
                reduce = data;
                ret = 1;
            }
            break;
        case FFWINFO:
            // Not Implemented
            break;
        case FEXTCMD:
            ret = ExtCommand(data);
            break;
    }
    if (ret) {
        _delay_us(2);
       //USI_UART_Transmit_Byte(0xFF);
    }
}


uint8_t ExtCommand(uint8_t ext) {
    uint8_t ret = 0;
    if (ext <= EUNUSED || ext >= ELAST) {
        return ret;
    }
    ret = 0;
    switch (ext) {
        case ELEDON:
            if ( rgb_status == SINITED || rgb_status == SPOWEROFF ) {
                ret = 1;
                rgb_status = SPOWERON;
                ws2812_on(&cont, spec_update_led);
            }
            break;
        case ELEDOFF:
            if ( rgb_status == SPOWERON) {
                ret = 1;
                rgb_status = SPOWEROFF;
                ws2812_off(&cont);
                TIMSK &= ~_BV(TOIE1);
                TCCR1 = 0;
            }
            break;
        case ELEDREFRESH:
            if ( rgb_status == SPOWERON) {
                ws2812_update(&cont, spec_update_led);
                ret = 1;
            }
            break;
        case ETIMER_START:
            TIMSK |= _BV(TOIE1);
            break;

        case ETIMER_STOP:
            TIMSK &= ~_BV(TOIE1);
            break;

        case EINTHIGHT:
            PORTB |= _BV(PORT_LED);
            ret = 1;
            break;

        case EINTLOW:
            PORTB &= ~_BV(PORT_LED);
            ret = 1;
            break;
        default:
            ret = 0;
    }
    return ret;
}

// }}}
