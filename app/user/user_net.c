/*
 * user_net.c
 *
 *  Created on: 2017年9月7日
 *      Author: liruya
 */

#include "user_net.h"
#include "espconn.h"

LOCAL os_timer_t xlink_loop_timer;

//void ICACHE_FLASH_ATTR user_net_send_pipe( uint8_t *pbuf, uint16_t len )
//{
//	user_tcp_send_pipe( pbuf, len );
//	user_udp_send_pipe( pbuf, len );
//}

void ICACHE_FLASH_ATTR user_net_init( void )
{
	user_udp_server_init();
	user_tcp_client_init();

//	if( XlinkSystemInit( PRODUCT_ID, PRODUCT_KEY, &user_config ) )
//	{
//
//	}
//	if( XlinkGetACK() == 0 )
//	{
//		XlinkSetACK( 88888888 );
//	}
//	XlinkSystemSetDeviceName( "Test" );
//	XlinkPorcess_UDP_Enable();
//	if( user_config.setServerStatus != NULL )
//	{
//		user_config.setServerStatus( 0, 0 );
//	}
//	XlinkSystemSetWifiStatus( 1 );
	os_timer_disarm( &xlink_loop_timer );
	os_timer_setfn( &xlink_loop_timer, user_tcp_func_process, NULL );
	os_timer_arm( &xlink_loop_timer, 100, 1 );
}

