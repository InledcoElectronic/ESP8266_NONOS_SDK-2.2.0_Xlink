/*
 * app_board_socket.c
 *
 *  Created on: 2018年6月5日
 *      Author: liruya
 */

#include "app_config.h"
#include "app_board_socket.h"
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

void ICACHE_FLASH_ATTR app_board_socket_init()
{
	gpio_init();
#ifdef	GPIO16_OUTPUT
	gpio16_output_conf();
#endif

	GPIO_DIS_OUTPUT( BOOT_IO_NUM );
	PIN_PULLUP_DIS( DETECT_IO_MUX );
	GPIO_DIS_OUTPUT( DETECT_IO_NUM );

	PIN_FUNC_SELECT( BOOT_IO_MUX, BOOT_IO_FUNC );
	PIN_FUNC_SELECT( DETECT_IO_MUX, DETECT_IO_FUNC );
	PIN_FUNC_SELECT( LEDR_IO_MUX, LEDR_IO_FUNC );
	PIN_FUNC_SELECT( LEDG_IO_MUX, LEDG_IO_FUNC );
	PIN_FUNC_SELECT( LEDB_IO_MUX, LEDB_IO_FUNC );
	PIN_FUNC_SELECT( RELAY1_IO_MUX, RELAY1_IO_FUNC );

	app_gpio_output_set( LEDR_IO_NUM, 1 );
	app_gpio_output_set( LEDG_IO_NUM, 1 );
	app_gpio_output_set( LEDB_IO_NUM, 1 );
	app_gpio_output_set( RELAY1_IO_NUM, 0 );
}


