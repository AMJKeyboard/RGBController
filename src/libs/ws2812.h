/** * libws2812
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

#ifndef WS2812_H_
#define WS2812_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define __in
#define __in_opt
#define __inout

/*
 * Port/Pin definition macros
 */
#define DEF_DDR(_BNK_) DDR ## _BNK_
#define DEF_PORT(_BNK_) PORT ## _BNK_
#define DEF_PIN(_BNK_, _PIN_) P ## _BNK_ ## _PIN_

/*
 * Error codes
 */
typedef enum _wserr_t {
	WS_ERR_NONE = 0,
	WS_ERR_INV_ARG,
	WS_ERR_INV_DATA,
	WS_ERR_INV_STATE,
} wserr_t;

#define WS_ERR_MAX WS_ERR_STATE

/*
 * Error code check macro
 * @param _ERR_ error code
 * @return true if no error code, false otherwise
 */
#define WS_ERR_SUCCESS(_ERR_) ((_ERR_) == WS_ERR_NONE)

/*
 * Color channel types
 */
typedef enum _wschan_t {

	/*
	 * NOTE: WS2812 color channels are ordered GRB
	 */
	WS_CHAN_GREEN = 0,
	WS_CHAN_RED,
	WS_CHAN_BLUE,
} wschan_t;

#define WS_CHAN_MAX WS_CHAN_BLUE

/*
 * Color structure
 * @member green green color channel
 * @member red red color channel
 * @member blue blue color channel
 */
typedef struct __attribute__ ((packed)) _wscol_t {
	uint8_t green, red, blue;
} wscol_t;

#define WS_CHAN_DEPTH (sizeof(uint8_t) * 8)
#define WS_COL_DEPTH (WS_CHAN_DEPTH * sizeof(wscol_t))

/*
 * Maximum supported chained LEDs
 * Change this if your need exceeds maximum
 */
#define WS_ELE_MAX_COUNT 64

/*
 * Status flag macros
 */
#define WS_STATUS_INIT 1
#define WS_STATUS_PWR 2

/*
 * Context structure
 * @member status status flag
 * @member count LED count
 * @member iter update iteration
 * @member ele LED array
 */
typedef struct __attribute__ ((packed)) _ws2812 {
	uint8_t status;
	volatile uint8_t *ddr;
	volatile uint8_t *port;
	uint8_t pin_power;
	uint8_t pin_data;
	uint16_t count;
	uint16_t iter;
	wscol_t ele[WS_ELE_MAX_COUNT];
} ws2812;

/*
 * Initialization callback routine
 * Called during initialization for all LEDs
 * @param ele current LED
 * @param ele_idx current LED index
 * @return WS_ERR_NONE on success
 */
typedef wserr_t(*wsinit_cb)(
	__inout wscol_t *ele,
	__in uint16_t ele_idx
	);

/*
 * Update callback routine
 * Called during update for all LEDs
 * @param ele current LED
 * @param ele_idx current LED index
 * @param iter current update iteration
 * @return WS_ERR_NONE on success
 */
typedef wserr_t(*wsupdate_cb)(
	__inout wscol_t *ele,
	__in uint16_t ele_idx,
	__in uint16_t iter
	);

/*
 * Initialization macro
 * Called prior to any other library calls
 * @param _CONT_ context structure
 * @papram _BNK_ user specified DDR/PORT/PIN bank
 * @param _PIN_PWR_ user specified power pin
 * @param _PIN_DATA_ user specified data pin
 * @param _COUNT_ LED count
 * @param _INIT_ optional initialization callback
 * @param _PWR_ initial power settings
 * @param _UPDATE_ optional update callback
 * @return WS_ERR_NONE on success
 */
#define ws2812_init(_CONT_, _BNK_, _PIN_PWR_, _PIN_DATA_, _COUNT_, _INIT_, _POWER_, _UPDATE_)\
	_ws2812_init(_CONT_, &DEF_DDR(_BNK_), &DEF_PORT(_BNK_), DEF_PIN(_BNK_, _PIN_PWR_),\
		DEF_PIN(_BNK_, _PIN_DATA_), _COUNT_, _INIT_, _POWER_, _UPDATE_)

/*
 * Initialization routine
 * Called prior to any other library calls
 * @param cont context structure
 * @papram ddr user specified DDR
 * @param port user specified PORT
 * @param pin_power user specified power pin
 * @param pin_data user specified data pin
 * @param count LED count
 * @param init optional initialization callback
 * @param power initial power settings
 * @param update optional update callback
 * @return WS_ERR_NONE on success
 */
wserr_t _ws2812_init(
	__inout ws2812 *cont,
	__in volatile uint8_t *ddr,
	__in volatile uint8_t *port,
	__in uint8_t pin_power,
	__in uint8_t pin_data,
	__in uint16_t count,
	__in_opt wsinit_cb init,
	__in bool power,
	__in_opt wsupdate_cb update
	);
	
/*
 * Power off routine
 * Must only be called after power on
 * @param cont context structure
 * @return WS_ERR_NONE on success
 */
wserr_t ws2812_off(
	__inout ws2812 *cont
	);

/*
 * Power on routine
 * Called to turn all LEDs on
 * @param cont context structure
 * @return WS_ERR_NONE on success
 */	
wserr_t ws2812_on(
	__inout ws2812 *cont,
	__in_opt wsupdate_cb update
	);
	
/*
 * Uninitialization routine
 * Called after all other library calls
 * @param cont context structure
 * @return WS_ERR_NONE on success
 */
wserr_t ws2812_uninit(
	__inout ws2812 *cont
	);
	
/*
 * Update routine
 * Called to update all LED colors
 * @param cont context structure
 * @param update optional update callback
 * @return WS_ERR_NONE on success
 */
wserr_t ws2812_update(
	__inout ws2812 *cont,
	__in_opt wsupdate_cb update
	);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // WS2812_H_
