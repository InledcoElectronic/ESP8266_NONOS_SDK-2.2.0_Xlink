/*
 * user_tcp_client.c
 *
 *  Created on: 2017年9月7日
 *      Author: liruya
 */

#include "user_tcp_client.h"
#include "gpio.h"
#include "user_ringbuffer.h"
#include "user_uart.h"
#include "os_type.h"
#include "user_ringbuffer.h"
#include "user_rtc.h"

#define	TCP_SEND_BUFFER_SIZE	2048

#define TCP_SEND_TIMEOUT		800				//unit ms

LOCAL struct espconn user_tcp_client;
LOCAL uint8_t tcp_ringbuffer[TCP_SEND_BUFFER_SIZE];
LOCAL ringbuf_t tcp_rb_send;
LOCAL bool mTcpSending;
LOCAL os_timer_t tcp_send_timer;
LOCAL uint8_t reconn_cnt;
bool isConnectServer;
LOCAL bool flag_dns_discovery;
LOCAL bool isDnsFound;
ip_addr_t remote_ip;
os_timer_t dns_timer;

void user_tcp_send_loop( void );

void ICACHE_FLASH_ATTR user_tcp_connect( void )
{
	espconn_connect( &user_tcp_client );
}

void ICACHE_FLASH_ATTR user_tcp_disconnect( void )
{
	espconn_disconnect( &user_tcp_client );
}

void ICACHE_FLASH_ATTR user_tcp_reconnect( void )
{
	if( user_smartconfig_status() )
	{
		reconn_cnt = 0;
		return;
	}
	reconn_cnt++;
	user_tcp_disconnect();
	if( reconn_cnt > 5 )
	{
		reconn_cnt = 0;
		isDnsFound = false;
		isConnectServer = false;
		flag_dns_discovery = false;
	}
	else
	{
		user_tcp_connect();
	}
}

LOCAL void ICACHE_FLASH_ATTR user_tcp_dns_found( const char *name, ip_addr_t *ip, void *arg )
{
	struct espconn *pespconn = (struct espconn *) arg;
	reconn_cnt = 0;
	isConnectServer = false;
	flag_dns_discovery = false;
	if( ip == NULL )
	{
		return;
	}
	if( ip->addr == 0 )
	{
		return;
	}
//	remote_ip.addr = ip->addr;
	os_timer_disarm( &dns_timer );
	isDnsFound = true;
	os_memcpy( user_tcp_client.proto.tcp->remote_ip, &ip->addr, 4 );
#ifdef	USE_TX_DEBUG
	os_printf( "dns found..."IPSTR"", IP2STR(user_tcp_client.proto.tcp->remote_ip) );
#endif
	espconn_connect( &user_tcp_client );
}

LOCAL void ICACHE_FLASH_ATTR user_tcp_dns_check_cb( void *arg )
{
	struct espconn *pespconn = (struct espconn *) arg;
	espconn_gethostbyname( pespconn, XLINK_DOMAIN, &remote_ip, user_tcp_dns_found );
}

LOCAL void ICACHE_FLASH_ATTR user_tcp_start_dns( struct espconn *pespconn )
{
	remote_ip.addr = 0;

	os_timer_disarm( &dns_timer );
	os_timer_setfn( &dns_timer, user_tcp_dns_check_cb, pespconn );
	os_timer_arm( &dns_timer, 1000, 1 );
#ifdef	USE_TX_DEBUG
	os_printf( "start dns..." );
#endif
}

void ICACHE_FLASH_ATTR user_tcp_func_process( void *arg )
{
	struct ip_info ipinfo;
	if ( user_smartconfig_status() )
	{
		return;
	}
	wifi_get_ip_info( STATION_IF, &ipinfo );
	if ( wifi_get_opmode() == STATION_MODE && wifi_station_get_connect_status() == STATION_GOT_IP && ipinfo.ip.addr != 0 )
	{
		if( flag_dns_discovery == false && isDnsFound == false )
		{
			flag_dns_discovery = true;
			reconn_cnt = 0;
			user_tcp_start_dns( &user_tcp_client );
		}
		if ( isConnectServer )
		{
		}
		xlink_process();
		user_tcp_send_loop();
	}
	else
	{
		flag_dns_discovery = false;
		isDnsFound = false;
		reconn_cnt = 0;
		isConnectServer = false;
		if ( wifi_get_opmode() != STATION_MODE )
		{
			wifi_set_opmode( STATION_MODE );
		}
	}
}

LOCAL void ICACHE_FLASH_ATTR user_tcp_connect_cb( void *arg )
{
#ifdef	USE_TX_DEBUG
	os_printf( "tcp connected..." );
#endif
	reconn_cnt = 0;
	isConnectServer = true;
	xlink_connect_cloud();
}

LOCAL void ICACHE_FLASH_ATTR user_tcp_disconnect_cb( void *arg )
{
	struct espconn *pespconn = arg;
	if ( pespconn == NULL )
	{
		return;
	}
#ifdef	USE_TX_DEBUG
	os_printf( "tcp disconnected..." );
#endif
	xlink_disconnect_cloud();
	user_tcp_disconnect();
	user_tcp_reconnect();
}

LOCAL void ICACHE_FLASH_ATTR user_tcp_reconnect_cb( void *arg, sint8 errtype )
{
#ifdef	USE_TX_DEBUG
	os_printf( "tcp disconnected with error: %d", errtype );
#endif
	user_tcp_reconnect();
}

LOCAL void ICACHE_FLASH_ATTR user_tcp_recv_cb( void *arg, char *buf, uint16_t len )
{
	if ( buf == NULL || len == 0 )
	{
		return;
	}
	xlink_receive_tcp_data( (const xlink_uint8 **)&buf, len );
}

LOCAL void ICACHE_FLASH_ATTR user_tcp_sent_cb( void *arg )
{
	os_timer_disarm( &tcp_send_timer );
	mTcpSending = false;
}

uint32_t ICACHE_FLASH_ATTR user_tcp_send( uint8_t *data, uint16_t datalen )
{
	return user_rb_put( &tcp_rb_send, data, datalen );
}

LOCAL uint8_t tcp_send_buffer[1460];

LOCAL void ICACHE_FLASH_ATTR user_tcp_send_timeout_cb( void *arg )
{
	os_timer_disarm( &tcp_send_timer );
	mTcpSending = false;
}

void ICACHE_FLASH_ATTR user_tcp_send_loop( void )
{
	uint32_t len;
	if ( mTcpSending )
	{
		return;
	}
	len = user_rb_unread_size( &tcp_rb_send );
	if ( len == 0 )
	{
		return;
	}
	mTcpSending = true;
	memset( tcp_send_buffer, 0, sizeof( tcp_send_buffer ) );
	len = user_rb_get( &tcp_rb_send, tcp_send_buffer, sizeof( tcp_send_buffer ) );
	espconn_send( &user_tcp_client, tcp_send_buffer, len );
	os_timer_disarm( &tcp_send_timer );
	os_timer_setfn( &tcp_send_timer, user_tcp_send_timeout_cb, NULL );
	os_timer_arm( &tcp_send_timer, TCP_SEND_TIMEOUT, 0 );
}

void ICACHE_FLASH_ATTR user_tcp_client_init( void )
{
	if ( user_tcp_client.proto.tcp == NULL )
	{
		user_tcp_client.proto.tcp = (esp_tcp *) os_zalloc( sizeof(esp_tcp) );
	}

	user_tcp_client.type = ESPCONN_TCP;
	user_tcp_client.state = ESPCONN_NONE;
	user_tcp_client.proto.tcp->local_port = TCP_LOCAL_PORT;
	user_tcp_client.proto.tcp->remote_port = TCP_REMOTE_PORT;
	espconn_regist_connectcb( &user_tcp_client, user_tcp_connect_cb );
	espconn_regist_disconcb( &user_tcp_client, user_tcp_disconnect_cb );
	espconn_regist_reconcb( &user_tcp_client, user_tcp_reconnect_cb );
	espconn_regist_recvcb( &user_tcp_client, user_tcp_recv_cb );
	espconn_regist_sentcb( &user_tcp_client, user_tcp_sent_cb );

	user_rb_init( &tcp_rb_send, tcp_ringbuffer, sizeof( tcp_ringbuffer ) );
}
