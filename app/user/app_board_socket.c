/*
 * app_board_socket.c
 *
 *  Created on: 2018年6月5日
 *      Author: liruya
 */

#include "app_config.h"
#include "app_board_socket.h"
#include "gpio.h"
#include "user_uart.h"

void ICACHE_FLASH_ATTR app_board_socket_init()
{
	gpio_init();

	GPIO_DIS_OUTPUT( BOOT_IO_NUM );
	PIN_PULLUP_DIS( DETECT_IO_MUX );
	GPIO_DIS_OUTPUT( DETECT_IO_NUM );

	PIN_FUNC_SELECT( BOOT_IO_MUX, BOOT_IO_FUNC );
	PIN_FUNC_SELECT( DETECT_IO_MUX, DETECT_IO_FUNC );
	PIN_FUNC_SELECT( LEDR_IO_MUX, LEDR_IO_FUNC );
	PIN_FUNC_SELECT( LEDG_IO_MUX, LEDG_IO_FUNC );
	PIN_FUNC_SELECT( LEDB_IO_MUX, LEDB_IO_FUNC );
	PIN_FUNC_SELECT( RELAY1_IO_MUX, RELAY1_IO_FUNC );

	ledr_off();
	ledg_off();
	ledb_off();
	relay_off();
}


