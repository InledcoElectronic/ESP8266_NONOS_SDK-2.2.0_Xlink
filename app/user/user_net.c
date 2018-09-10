/*
 * user_net.c
 *
 *  Created on: 2017年9月7日
 *      Author: liruya
 */

#include "user_net.h"
#include "espconn.h"

LOCAL os_timer_t xlink_loop_timer;

void ICACHE_FLASH_ATTR user_net_init( void )
{
	user_udp_server_init();
	user_tcp_client_init();

	os_timer_disarm( &xlink_loop_timer );
	os_timer_setfn( &xlink_loop_timer, user_tcp_func_process, NULL );
	os_timer_arm( &xlink_loop_timer, 100, 1 );
}

