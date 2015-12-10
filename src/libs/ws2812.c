/**
 * libws2812
 * Copyright (C) 2014 David Jolly
 * ----------------------
 *
 * libws2812 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libws2812 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>
#include "ws2812.h"

#define WS_MSB_MASK 0x80

#define WS_DELAY_PWR_CHG 100.0 // us
#define WS_DELAY_RESET 50.0 // us

#define WS_BIT_CLR(_FLAG_, _BIT_) ((_FLAG_) &= ~(_BIT_))
#define WS_BIT_CHK(_FLAG_, _BIT_) ((_FLAG_) & (_BIT_))
#define WS_BIT_SET(_FLAG_, _BIT_) ((_FLAG_) |= (_BIT_))

wserr_t  
_ws2812_write(
    __in ws2812 *cont
    )
{
    uint16_t len;
    wserr_t result = WS_ERR_NONE;
    uint8_t bit, *data = NULL, data_ele, old_sreg, mask_high, mask_low;
    uint8_t volatile *out_port = NULL;

    if(!cont || !WS_BIT_CHK(cont->status, WS_STATUS_INIT)
            || !WS_BIT_CHK(cont->status, WS_STATUS_PWR)) {
        result = WS_ERR_INV_ARG;
        goto exit;
    }

    data = (uint8_t *) cont->ele;
    len = cont->count * sizeof(wscol_t);
    if(!data || !len) {
        result = WS_ERR_INV_DATA;
        goto exit;
    }

    mask_high = *cont->port | _BV(cont->pin_data);
    mask_low = *cont->port & ~_BV(cont->pin_data);
    out_port = cont->port;
    old_sreg = SREG;
    cli();

    while(len--) {

        // write each byte to LED chain
        data_ele = *data++;
        bit = WS_CHAN_DEPTH;

        while(bit--) {
            *out_port = mask_high;
            if(!(data_ele & WS_MSB_MASK)) {
                *out_port = mask_low;
                asm volatile("nop");
            }
            asm volatile("nop");
            asm volatile("nop");
            asm volatile("nop");
            asm volatile("nop");
            asm volatile("nop");
            asm volatile("nop");
            asm volatile("nop");
            asm volatile("nop");
            data_ele <<= 1;
            *out_port = mask_low;
        }
    }

    // write reset
    WS_BIT_CLR(*cont->port, _BV(cont->pin_data));
    _delay_us(WS_DELAY_RESET);

    sei();
    SREG = old_sreg;

exit:
    return result;
}

wserr_t 
_ws2812_init(
    __inout ws2812 *cont,
    __in volatile uint8_t *ddr,
    __in volatile uint8_t *port,
    __in uint8_t pin_power,
    __in uint8_t pin_data,
    __in uint16_t count,
    __in_opt wsinit_cb init,
    __in bool power,
    __in_opt wsupdate_cb update
    )
{
    uint16_t ele_iter = 0;
    wserr_t result = WS_ERR_NONE;

    if(!cont || !count || (count > WS_ELE_MAX_COUNT)) {
        result = WS_ERR_INV_ARG;
        goto exit;
    }

    if(WS_BIT_CHK(cont->status, WS_STATUS_INIT)) {
    
        // if context was previously initialized, uninitialize it
        result = ws2812_uninit(cont);
        if(!WS_ERR_SUCCESS(result)) {
            goto exit;
        }
    } else {
        memset(cont, 0, sizeof(ws2812));
    }

    cont->ddr = ddr;
    cont->port = port;
    cont->pin_power = pin_power;
    cont->pin_data = pin_data;
    *cont->ddr |= (_BV(cont->pin_power) | _BV(cont->pin_data));
    *cont->port &= ~(_BV(cont->pin_power) | _BV(cont->pin_data));
    cont->status = WS_STATUS_INIT;
    cont->count = count;

    if(init) {
    
        // if initialize callback is supplied, initialize all LEDs
        for(; ele_iter < cont->count; ++ele_iter) {
        
            result = init(&cont->ele[ele_iter], ele_iter);
            if(!WS_ERR_SUCCESS(result)) {
                goto exit;
            }
        }
    }

    if(power) {

        // if power on is requested, power on all LEDs
        result = ws2812_on(cont, update);
        if(!WS_ERR_SUCCESS(result)) {
            
            result = ws2812_uninit(cont);
            if(!WS_ERR_SUCCESS(result)) {
                goto exit;
            }
        }
    }

exit:
    return result;
}

wserr_t 
ws2812_off(
    __inout ws2812 *cont
    )
{
    wserr_t result = WS_ERR_NONE;

    if(!cont || !WS_BIT_CHK(cont->status, WS_STATUS_INIT)) {
        result = WS_ERR_INV_ARG;
        goto exit;
    }
    
    // confirm that power is not already off
    if(!WS_BIT_CHK(cont->status, WS_STATUS_PWR)) {
        result = WS_ERR_INV_STATE;
        goto exit;
    }
    
    WS_BIT_CLR(*cont->port, _BV(cont->pin_power));
    WS_BIT_CLR(cont->status, WS_STATUS_PWR);
    _delay_us(WS_DELAY_PWR_CHG);
    
exit:
    return result;
}
    
wserr_t 
ws2812_on(
    __inout ws2812 *cont,
    __in_opt wsupdate_cb update
    )
{
    wserr_t result = WS_ERR_NONE;

    if(!cont || !WS_BIT_CHK(cont->status, WS_STATUS_INIT)) {
        result = WS_ERR_INV_ARG;
        goto exit;
    }

    // confirm that power is not already on
    if(WS_BIT_CHK(cont->status, WS_STATUS_PWR)) {
        result = WS_ERR_INV_STATE;
        goto exit;
    }

    WS_BIT_SET(*cont->port, _BV(cont->pin_power));
    WS_BIT_SET(cont->status, WS_STATUS_PWR);
    _delay_us(WS_DELAY_PWR_CHG);

    // if update callback is supplied, update all LEDs
    result = ws2812_update(cont, update);
    if(!WS_ERR_SUCCESS(result)) {
        goto exit;
    }

exit:
    return result;
}

wserr_t 
ws2812_uninit(
    __inout ws2812 *cont
    )
{
    wserr_t result = WS_ERR_NONE;

    if(!cont || !WS_BIT_CHK(cont->status, WS_STATUS_INIT)) {
        result = WS_ERR_INV_ARG;
        goto exit;
    }

    if(WS_BIT_CHK(cont->status, WS_STATUS_PWR)) {
    
        // if power is on, power off all LEDs
        result = ws2812_off(cont);
        if(!WS_ERR_SUCCESS(result)) {
            goto exit;
        }
    }

    *cont->ddr &= ~(_BV(cont->pin_power) | _BV(cont->pin_data));
    *cont->port &= ~(_BV(cont->pin_power) | _BV(cont->pin_data));
    memset(cont, 0, sizeof(ws2812));

exit:
    return result;
}

wserr_t 
ws2812_update(
    __inout ws2812 *cont,
    __in_opt wsupdate_cb update
    )
{
    uint16_t ele_iter;
    wserr_t result = WS_ERR_NONE;

    if(!cont || !WS_BIT_CHK(cont->status, WS_STATUS_INIT)) {
        result = WS_ERR_INV_ARG;
        goto exit;
    }
    
    // confirm that power is not off
    if(!WS_BIT_CHK(cont->status, WS_STATUS_PWR)) {
        result = WS_ERR_INV_STATE;
        goto exit;
    }
    
    if(update) {
        // if update callback is supplied, update all LEDs
        for(ele_iter = 0; ele_iter < cont->count; ++ele_iter) {
            result = update(&cont->ele[ele_iter], ele_iter, cont->iter);
            if(!WS_ERR_SUCCESS(result)) {
                goto exit;
            }
        }
    }

    // write resulting colors to all LEDs
    result = _ws2812_write(cont);
    if(!WS_ERR_SUCCESS(result)) {
        goto exit;
    }

    ++cont->iter;

exit:
    return result;
}
