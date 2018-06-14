/*
 * user_user_udp_server.c
 *
 *  Created on: 2017年9月7日
 *      Author: liruya
 */

#include "user_udp_server.h"
#include "../include/user_uart.h"
#include "../include/xlink.h"

#define UDP_TRANSFER_LOCAL_PORT			8266
#define UDP_TRANSFER_REMOTE_PORT		9587

LOCAL struct espconn user_udp_server;
LOCAL struct espconn user_udp_transfer;

//LOCAL user_udp_recv_pipe_cb_t user_udp_recv_pipe_cb;

void ICACHE_FLASH_ATTR user_udp_server_init( void )
{
	user_udp_server.type = ESPCONN_UDP;
	user_udp_server.state = ESPCONN_NONE;
	user_udp_server.proto.udp = (esp_udp *) os_zalloc( sizeof( esp_udp ) );
	user_udp_server.reverse = NULL;
	user_udp_server.proto.udp->local_port = UDP_LOCAL_PORT;
	user_udp_server.proto.udp->remote_port = UDP_REMOTE_PORT;
	user_udp_server.proto.udp->remote_ip[0] = 0x00;
	user_udp_server.proto.udp->remote_ip[1] = 0x00;
	user_udp_server.proto.udp->remote_ip[2] = 0x00;
	user_udp_server.proto.udp->remote_ip[3] = 0x00;
	espconn_regist_recvcb( &user_udp_server, user_udp_recv_cb );
	espconn_create( &user_udp_server );
}

void ICACHE_FLASH_ATTR user_udp_recv_cb( void *arg, char *pdata, unsigned short len )
{
	if( pdata == NULL || len == 0 )
	{
		return;
	}
	xlink_addr_t addr;
	xlink_addr_t *paddr = &addr;
	ip_address_t ip_temp;
	ip_temp.ip = 0;
	remot_info *premote = NULL;
	if( espconn_get_connection_info( &user_udp_server, &premote, 0 ) != ESPCONN_OK )
	{
		return;
	}
	os_memcpy( user_udp_server.proto.udp->remote_ip, premote->remote_ip, 4 );
	user_udp_server.proto.udp->remote_port = premote->remote_port;

	ip_temp.bit.byte0 = user_udp_server.proto.udp->remote_ip[0];
	ip_temp.bit.byte1 = user_udp_server.proto.udp->remote_ip[1];
	ip_temp.bit.byte2 = user_udp_server.proto.udp->remote_ip[2];
	ip_temp.bit.byte3 = user_udp_server.proto.udp->remote_ip[3];
	xlink_memset( &addr, 0, sizeof( xlink_addr_t ) );
	addr.ip = ip_temp.ip;
	addr.port = user_udp_server.proto.udp->remote_port;
	xlink_receive_udp_data( (const xlink_uint8 **)&pdata, len, (const xlink_addr_t **)&paddr );
}

uint32_t ICACHE_FLASH_ATTR user_udp_send( xlink_addr_t *addr, uint8_t *pdata, uint16_t len )
{
	ip_address_t ip_addr;
	ip_addr.ip = addr->ip;
	user_udp_server.proto.udp->remote_port = addr->port;
	user_udp_server.proto.udp->remote_ip[0] = ip_addr.bit.byte0;
	user_udp_server.proto.udp->remote_ip[1] = ip_addr.bit.byte1;
	user_udp_server.proto.udp->remote_ip[2] = ip_addr.bit.byte2;
	user_udp_server.proto.udp->remote_ip[3] = ip_addr.bit.byte3;
	espconn_send( &user_udp_server, pdata, len );
	return len;
}

uint32_t ICACHE_FLASH_ATTR user_udp_sent( uint8_t *pdata, uint16_t len )
{
	espconn_send( &user_udp_server, pdata, len );
	return len;
}

/**
 * local udp transfer instead of xlink transfer
 */
//void ICACHE_FLASH_ATTR user_udp_transfer_init( espconn_recv_callback callback )
//{
//	user_udp_transfer.type = ESPCONN_UDP;
//	user_udp_transfer.state = ESPCONN_NONE;
//	user_udp_transfer.proto.udp = (esp_udp *) os_zalloc( sizeof( esp_udp ) );
//	user_udp_transfer.reverse = NULL;
//	user_udp_transfer.proto.udp->local_port = UDP_TRANSFER_LOCAL_PORT;
//	user_udp_transfer.proto.udp->remote_port = UDP_TRANSFER_REMOTE_PORT;
//	user_udp_transfer.proto.udp->remote_ip[0] = 0x00;
//	user_udp_transfer.proto.udp->remote_ip[1] = 0x00;
//	user_udp_transfer.proto.udp->remote_ip[2] = 0x00;
//	user_udp_transfer.proto.udp->remote_ip[3] = 0x00;
//	espconn_regist_recvcb( &user_udp_transfer, callback );
//	espconn_create( &user_udp_transfer );
//}

