/*
 * app_board.c
 *
 *  Created on: 2018年6月5日
 *      Author: liruya
 */

#include "app_board_led.h"
#include "gpio.h"
#ifdef	GPIO16_OUTPUT
#include "driver/gpio16.h"
#endif

LOCAL void ICACHE_FLASH_ATTR app_gpio_output_set( uint8_t pin, uint8_t value )
{
	if ( pin < 16 )
	{
		GPIO_OUTPUT_SET( pin, value );
	}
#ifdef	GPIO16_OUTPUT
	else if ( pin == 16 )
	{
		gpio16_output_set( value );
		return;
	}
#endif
}

void ICACHE_FLASH_ATTR app_board_led_init()
{
	gpio_init();
#ifdef	GPIO16_OUTPUT
	gpio16_output_conf();
#endif

	GPIO_DIS_OUTPUT( BOOT_IO_NUM );
	GPIO_DIS_OUTPUT( TOUCH_IO_FUNC );

	PIN_FUNC_SELECT( BOOT_IO_MUX, BOOT_IO_FUNC );
	PIN_FUNC_SELECT( PWM1_IO_MUX, PWM1_IO_FUNC );
	PIN_FUNC_SELECT( PWM2_IO_MUX, PWM2_IO_FUNC );
	PIN_FUNC_SELECT( PWM3_IO_MUX, PWM3_IO_FUNC );
	PIN_FUNC_SELECT( PWM4_IO_MUX, PWM4_IO_FUNC );
	PIN_FUNC_SELECT( PWM5_IO_MUX, PWM5_IO_FUNC );
	PIN_FUNC_SELECT( TOUCH_IO_MUX, TOUCH_IO_FUNC );
	PIN_FUNC_SELECT( LEDR_IO_MUX, LEDR_IO_FUNC );
//	PIN_FUNC_SELECT( LEDG_IO_MUX, LEDG_IO_FUNC );	//ledg
#ifndef	USE_TX_DEBUG
	PIN_FUNC_SELECT( LEDB_IO_MUX, LEDB_IO_FUNC );
#endif

	app_gpio_output_set( PWM1_IO_NUM, 0 );
	app_gpio_output_set( PWM2_IO_NUM, 0 );
	app_gpio_output_set( PWM3_IO_NUM, 0 );
	app_gpio_output_set( PWM4_IO_NUM, 0 );
	app_gpio_output_set( PWM5_IO_NUM, 0 );
	app_gpio_output_set( LEDR_IO_NUM, 1 );
	app_gpio_output_set( LEDG_IO_NUM, 1 );
#ifndef	USE_TX_DEBUG
	app_gpio_output_set( LEDB_IO_NUM, 1 );
#endif
}

