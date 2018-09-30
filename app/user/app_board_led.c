/*
 * app_board.c
 *
 *  Created on: 2018年6月5日
 *      Author: liruya
 */

#include "app_board_led.h"
#include "gpio.h"
#include "driver/gpio16.h"
#include "user_smartconfig.h"

LOCAL void ICACHE_FLASH_ATTR sc_led_set(bool value)
{
	GPIO_OUTPUT_SET(LEDG_IO_NUM, value);
	// gpio16_output_set(value);
}

LOCAL bool ICACHE_FLASH_ATTR sc_led_get()
{
	return ((GPIO_REG_READ(GPIO_OUT_ADDRESS)>>(LEDG_IO_NUM))&BIT0);
	// return gpio16_output_get();
}

void ICACHE_FLASH_ATTR app_board_led_init()
{
	gpio_init();
	gpio16_output_conf();

	GPIO_DIS_OUTPUT( TOUCH_IO_FUNC );

	PIN_FUNC_SELECT( PWM1_IO_MUX, PWM1_IO_FUNC );
	PIN_FUNC_SELECT( PWM2_IO_MUX, PWM2_IO_FUNC );
	PIN_FUNC_SELECT( PWM3_IO_MUX, PWM3_IO_FUNC );
	PIN_FUNC_SELECT( PWM4_IO_MUX, PWM4_IO_FUNC );
	PIN_FUNC_SELECT( PWM5_IO_MUX, PWM5_IO_FUNC );
	PIN_FUNC_SELECT( TOUCH_IO_MUX, TOUCH_IO_FUNC );
	PIN_FUNC_SELECT( LEDR_IO_MUX, LEDR_IO_FUNC );
	PIN_FUNC_SELECT( LEDG_IO_MUX, LEDG_IO_FUNC );
	// PIN_FUNC_SELECT( LEDB_IO_MUX, LEDB_IO_FUNC );	//IO16

	GPIO_OUTPUT_SET( PWM1_IO_NUM, 0 );
	GPIO_OUTPUT_SET( PWM2_IO_NUM, 0 );
	GPIO_OUTPUT_SET( PWM3_IO_NUM, 0 );
	GPIO_OUTPUT_SET( PWM4_IO_NUM, 0 );
	GPIO_OUTPUT_SET( PWM5_IO_NUM, 0 );
	ledr_off();
	ledg_off();
	ledb_off();

	user_smartconfig_init_led(sc_led_set, sc_led_get);
}

