/*
 * user_main.c
 *
 *  Created on: 2017年9月2日
 *      Author: liruya
 */

#include "espconn.h"
#include "gpio.h"

#include "app_config.h"
#include "osapi.h"
#include "smartconfig.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "user_net.h"
#include "user_key.h"
#include "driver/gpio16.h"

/* device include */
//#include "user_led.h"
//#include "app_board_led.h"
#include "user_socket.h"
#include "app_board_socket.h"


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR user_rf_cal_sector_set( void )
{
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch ( size_map )
	{
		case FLASH_SIZE_4M_MAP_256_256:
			rf_cal_sec = 128 - 5;
			break;

		case FLASH_SIZE_8M_MAP_512_512:
			rf_cal_sec = 256 - 5;
			break;

		case FLASH_SIZE_16M_MAP_512_512:
		case FLASH_SIZE_16M_MAP_1024_1024:
			rf_cal_sec = 512 - 5;
			break;

		case FLASH_SIZE_32M_MAP_512_512:
		case FLASH_SIZE_32M_MAP_1024_1024:
			rf_cal_sec = 1024 - 5;
			break;

		case FLASH_SIZE_64M_MAP_1024_1024:
			rf_cal_sec = 2048 - 5;
			break;
		case FLASH_SIZE_128M_MAP_1024_1024:
			rf_cal_sec = 4096 - 5;
			break;
		default :
			rf_cal_sec = 0;
			break;
	}

	return rf_cal_sec;
}

void ICACHE_FLASH_ATTR user_rf_pre_init( void )
{
}

void ICACHE_FLASH_ATTR app_init( void )
{
	os_delay_us(60000);

//	app_board_led_init();
//	xlink_init( &user_led );
	app_board_socket_init();
	xlink_init( &user_socket );
	user_rtc_init();
//	user_udp_transfer_init( user_led_decode );
	user_net_init();
}

void ICACHE_FLASH_ATTR user_init( void )
{
	system_init_done_cb( app_init );
}