/*
 * user_smartconfig.c
 *
 *  Created on: 2017年9月8日
 *      Author: liruya
 */

#include "user_smartconfig.h"
#include "app_config.h"

LOCAL bool smartconfig_status;
LOCAL uint32_t sc_timeout = 60000;
LOCAL os_timer_t sc_timer;

#ifdef SMARTCONFIG_USE_LED
LOCAL os_timer_t sc_led_timer;
LOCAL bool sc_led_state;
LOCAL uint8_t sc_led_flash_cnt;

void user_smartconfig_success_led_cb( void *arg );
#endif

LOCAL void ICACHE_FLASH_ATTR user_smartconfig_done( sc_status status, void *pdata )
{
	switch ( status )
	{
		case SC_STATUS_WAIT:
			app_printf( "SC_STATUS_WAIT\n" );
			break;
		case SC_STATUS_FIND_CHANNEL:
			app_printf( "SC_STATUS_FIND_CHANNEL\n" );
			break;
		case SC_STATUS_GETTING_SSID_PSWD:
			app_printf( "SC_STATUS_GETTING_SSID_PSWD\n" );
			{
				sc_type *type = (sc_type *) pdata;
				if ( *type == SC_TYPE_ESPTOUCH )
				{
					app_printf( "SC_TYPE:SC_TYPE_ESPTOUCH\n" );
				}
				else
				{
					app_printf( "SC_TYPE:SC_TYPE_AIRKISS\n" );
				}
			}
			break;
		case SC_STATUS_LINK:
			app_printf( "SC_STATUS_LINK\n" );
			{
				wifi_station_disconnect();
				struct station_config *sta_conf = (struct station_config *) pdata;
				wifi_station_set_config( sta_conf );
				wifi_station_connect();
			}
			break;
		case SC_STATUS_LINK_OVER:
			app_printf( "SC_STATUS_LINK_OVER\n" );
			if ( pdata != NULL )
			{
				//SC_TYPE_ESPTOUCH
				uint8 phone_ip[4] = { 0 };
				os_memcpy( phone_ip, (uint8*) pdata, 4 );
				app_printf( "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1],
						phone_ip[2], phone_ip[3] );
			}
			else
			{
				//SC_TYPE_AIRKISS - support airkiss v2.0
			}
			os_timer_disarm( &sc_timer );
#ifdef	SMARTCONFIG_USE_LED
			sc_led_flash_cnt = 0;
			os_timer_disarm( &sc_led_timer );
			os_timer_setfn( &sc_led_timer, user_smartconfig_success_led_cb, NULL );
			os_timer_arm( &sc_led_timer, 200, 1 );
#endif
			smartconfig_stop();
			smartconfig_status = false;
			break;
	}
}

void ICACHE_FLASH_ATTR user_smartconfig_timeout_cb( void *arg )
{
	os_timer_disarm( &sc_timer );
#ifdef SMARTCONFIG_USE_LED
	sc_led_set( sc_led_state );
	os_timer_disarm( &sc_led_timer );
#endif
	smartconfig_stop();
	smartconfig_status = false;
	wifi_station_connect();
}

#ifdef SMARTCONFIG_USE_LED
void ICACHE_FLASH_ATTR user_smartconfig_process_led_cb( void *arg )
{
	sc_led_set( !sc_led_status() );
}

void ICACHE_FLASH_ATTR user_smartconfig_success_led_cb( void *arg )
{
	sc_led_flash_cnt++;
	if ( sc_led_flash_cnt > 10 )
	{
		sc_led_flash_cnt = 0;
		os_timer_disarm( &sc_led_timer );
		sc_led_set( sc_led_state );
		return;
	}
	sc_led_set( !sc_led_status() );
}
#endif

void ICACHE_FLASH_ATTR user_smartconfig_start( void )
{
	smartconfig_status = true;
	wifi_station_disconnect();
	smartconfig_stop();
	wifi_set_opmode( STATION_MODE );
	smartconfig_set_type( SC_TYPE_ESPTOUCH );
	os_timer_disarm( &sc_timer );
	os_timer_setfn( &sc_timer, user_smartconfig_timeout_cb, NULL );
	os_timer_arm( &sc_timer, sc_timeout, 0 );
#ifdef SMARTCONFIG_USE_LED
	sc_led_state = sc_led_status();
	os_timer_disarm( &sc_led_timer );
	os_timer_setfn( &sc_led_timer, user_smartconfig_process_led_cb, NULL );
	os_timer_arm( &sc_led_timer, 800, 1 );
#endif
	smartconfig_start( user_smartconfig_done );
}

void ICACHE_FLASH_ATTR user_smartconfig_set_timeout( uint32_t timeout )
{
	sc_timeout = timeout;
}

bool ICACHE_FLASH_ATTR user_smartconfig_status( void )
{
	return smartconfig_status;
}

