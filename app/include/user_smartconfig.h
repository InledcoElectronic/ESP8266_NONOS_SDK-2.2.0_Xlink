/*
 * user_smartconfig.h
 *
 *  Created on: 2017年9月8日
 *      Author: liruya
 */

#ifndef _USER_SMARTCONFIG_H_
#define _USER_SMARTCONFIG_H_

#include "smartconfig.h"
#include "osapi.h"
#include "user_interface.h"
#include "c_types.h"

#define SMARTCONFIG_USE_LED
#ifdef	SMARTCONFIG_USE_LED
#define SC_LED_NUM				16
#if	SC_LED_NUM == 16
#include "driver/gpio16.h"
#define sc_led_on()				gpio16_output_set(0)
#define sc_led_off()			gpio16_output_set(1)
#define sc_led_set(value)		gpio16_output_set(value)
#define sc_led_status()			gpio16_output_get()
#else
#define sc_led_on()				GPIO_OUTPUT_SET(SC_LED_NUM, 0)
#define sc_led_off()			GPIO_OUTPUT_SET(SC_LED_NUM, 1)
#define sc_led_set(value)		GPIO_OUTPUT_SET(SC_LED_NUM, value)
#define sc_led_status()			GPIO_INPUT_GET(SC_LED_NUM)
#endif
#endif

extern void user_smartconfig_set_timeout( uint32_t timeout );
extern bool user_smartconfig_status( void );
extern void user_smartconfig_start( void );

#endif /* _USER_SMARTCONFIG_H_ */
