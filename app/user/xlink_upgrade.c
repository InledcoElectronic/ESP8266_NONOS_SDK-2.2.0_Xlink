/*
 * xlink_upgrade.c
 *
 *  Created on: 2018年5月14日
 *      Author: liruya
 */

#include "xlink_upgrade.h"

#include "../include/app_config.h"
#include "upgrade.h"

#define XLINK_UPGRADE_LINK		"static.xlink.cn"

#define UPGRADE_MSG_START		"Device firmware upgrade start."
#define UPGRADE_MSG_FAILED		"Device firmware upgrade failed."
#define UPGRADE_MSG_SUCCESS		"Device firmware upgrade success."

#define PHEADBUFFER 			"Connection: keep-alive\r\n\
								Cache-Control: no-cache\r\n\
								User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
								Accept: */*\r\n\
								Accept-Encoding: gzip,deflate\r\n\
								Accept-Language: zh-CN,eb-US;q=0.8\r\n\r\n"

LOCAL os_timer_t delay_timer;
LOCAL struct espconn xlink_upgrade_tcp_client;
LOCAL ip_addr_t xlink_upgrade_ip;

struct upgrade_server_info *xlink_upgrade_server;
xlink_upgrade_info_t xlink_upgrade_info;

void XLINK_FUNCTION xlink_upgrade_state_notify( void *arg )
{
	uint16_t msgid = 0;
	char *msg = (char *) arg;
	struct xlink_sdk_event_t event;
	struct xlink_sdk_event_t *pevent = &event;
	app_printf( "%s   %d", msg, os_strlen( msg ) );
	event.enum_event_type_t = EVENT_TYPE_NOTIFY;
	event.event_struct_t.notify_t.from_id = xlink_get_deviceid();
	event.event_struct_t.notify_t.from_type = 2;
	event.event_struct_t.notify_t.message = msg;
	event.event_struct_t.notify_t.message_length = os_strlen( msg );
	xlink_post_event( &msgid, &pevent );
}

LOCAL void XLINK_FUNCTION xlink_upgrade_response( void *arg )
{
	struct upgrade_server_info *server = arg;
	if ( server->upgrade_flag == 1 )
	{
		app_printf( "Device OTA upgrade success..." );
		os_timer_disarm( &delay_timer );
		os_timer_setfn( &delay_timer, xlink_upgrade_state_notify, UPGRADE_MSG_SUCCESS );
		os_timer_arm( &delay_timer, 1000, 0 );
		system_upgrade_reboot();
	}
	else
	{
		app_printf( "Device OTA upgrade failed..." );
	}
	os_free( server->url );
	server->url = NULL;
	os_free( server );
	server = NULL;
}

LOCAL void XLINK_FUNCTION xlink_upgrade_wait( void *arg )
{
	struct espconn *pespconn = arg;
	os_timer_disarm( &delay_timer );
	if ( pespconn != NULL )
	{
		espconn_disconnect( pespconn );
	}
	else
	{

	}
}

void XLINK_FUNCTION xlink_upgrade_receive_data( void *arg, char *pdata, uint16_t datalen )
{
	struct espconn *pespconn = (struct espconn *) arg;

	if ( xlink_upgrade_server != NULL )
	{
		os_free( xlink_upgrade_server );
		xlink_upgrade_server = NULL;
	}

	os_timer_disarm( &delay_timer );
	xlink_upgrade_server = ( struct upgrade_server_info *) os_zalloc( sizeof(struct upgrade_server_info) );
	if ( xlink_upgrade_server == NULL )
	{
		app_printf( "Malloc upgrade_server_info failed..." );
		xlink_upgrade_state_notify( UPGRADE_MSG_FAILED );
		return;
	}

	xlink_upgrade_server->upgrade_version[5] = '\0';
	xlink_upgrade_server->pespconn = pespconn;
	os_memcpy( xlink_upgrade_server->ip, pespconn->proto.tcp->remote_ip, 4 );
	xlink_upgrade_server->port = pespconn->proto.tcp->remote_port;
	xlink_upgrade_server->check_cb = xlink_upgrade_response;
	xlink_upgrade_server->check_times = 60000;
	if ( xlink_upgrade_server->url == NULL )
	{
		xlink_upgrade_server->url = ( uint8_t * ) os_zalloc( 1024 );
		if ( xlink_upgrade_server->url == NULL )
		{
			app_printf( "Malloc upgrade_server_info->url failed..." );
			return;
		}
	}

	app_printf( "OTA Server Port=%d, Path=%s", xlink_upgrade_server->port, xlink_upgrade_info.path );

	if ( system_upgrade_userbin_check() == USER_BIN1 )
	{
		app_printf( "Download user2 bin" );
	}
	else if ( system_upgrade_userbin_check() == USER_BIN2 )
	{
		app_printf( "Download user1 bin" );
	}

	os_sprintf( xlink_upgrade_server->url, "GET /%s HTTP/1.1\r\nHost: %s\r\n" PHEADBUFFER "", xlink_upgrade_info.path, xlink_upgrade_info.ipaddr );
	app_printf( "Server url is : %s", xlink_upgrade_server->url );
}

void XLINK_FUNCTION xlink_upgrade_send_cb( void *arg )
{
	struct espconn *pespconn = arg;
	os_timer_disarm( &delay_timer );
	os_timer_setfn( &delay_timer, xlink_upgrade_wait, pespconn );
	os_timer_arm( &delay_timer, 5000, 0 );
	app_printf( "xlink_upgrade_send_cb" );
}

void XLINK_FUNCTION xlink_upgrade_disconnect_cb( void *arg )
{
	struct espconn *pespconn = arg;

	app_printf( "xlink_upgrade_disconnect_cb" );

	if ( pespconn != NULL )
	{
		if ( pespconn->proto.tcp != NULL )
		{
			os_free( pespconn->proto.tcp );
			pespconn->proto.tcp = NULL;
		}
		os_free( pespconn );
		pespconn = NULL;
	}
	if ( system_upgrade_start( xlink_upgrade_server ) == false )
	{
		xlink_upgrade_state_notify( UPGRADE_MSG_FAILED );
		app_printf( "upgrade start error..." );
	}
	else
	{
		xlink_upgrade_state_notify( UPGRADE_MSG_START );
		app_printf( "upgrade start success..." );
	}
}

void XLINK_FUNCTION xlink_upgrade_reconnect_cb( void *arg, int8_t error )
{
	struct espconn *pespconn = arg;

	app_printf( "xlink_upgrade_reconnect_cb" );

	if ( pespconn != NULL )
	{
		if ( pespconn->proto.tcp != NULL )
		{
			os_free( pespconn->proto.tcp );
			pespconn->proto.tcp = NULL;
		}
		os_free( pespconn );
		pespconn = NULL;
	}
	if ( xlink_upgrade_server != NULL )
	{
		os_free( xlink_upgrade_server );
		xlink_upgrade_server = NULL;
	}
}

void XLINK_FUNCTION xlink_upgrade_connect_cb( void *arg )
{
	struct espconn *pespconn = arg;
	uint8_t ret = 0;
	uint8_t *ptemp = NULL;

	app_printf( "xlink_upgrade_connect_cb" );

	espconn_regist_disconcb( pespconn, xlink_upgrade_disconnect_cb );
	espconn_regist_recvcb( pespconn, xlink_upgrade_receive_data );
	espconn_regist_sentcb( pespconn, xlink_upgrade_send_cb );

	ptemp = (uint8_t *) os_zalloc( 1024 );
	if ( ptemp == NULL )
	{
		xlink_upgrade_state_notify( UPGRADE_MSG_FAILED );
		app_printf( "Malloc ptemp failed..." );
		return;
	}

	os_sprintf(ptemp, "GET HTTP/1.1\r\nHost: %s\r\n" PHEADBUFFER "",
				 xlink_upgrade_info.ipaddr);

	ret = espconn_send( pespconn, ptemp, os_strlen( ptemp ) );
	if ( ret == 0 )
	{
		app_printf( "request download file success..." );
	}
	else
	{
		xlink_upgrade_state_notify( UPGRADE_MSG_FAILED );
		app_printf( "request download file failed..." );
	}
	os_free( ptemp );
	ptemp = NULL;
}

void XLINK_FUNCTION xlink_upgrade_tcp_client_init( uint8_t *ipaddr )
{
	if ( xlink_upgrade_tcp_client.proto.tcp == NULL )
	{
		xlink_upgrade_tcp_client.proto.tcp = (esp_tcp *) os_zalloc( sizeof( esp_tcp ) );
		if ( xlink_upgrade_tcp_client.proto.tcp == NULL )
		{
			xlink_upgrade_state_notify( UPGRADE_MSG_FAILED );
			app_printf( "Malloc xlink_upgrade_tcp_client.proto.tcp failed..." );
			return;
		}
	}

	int local_port = 0;
	os_memcpy( xlink_upgrade_tcp_client.proto.tcp->remote_ip, ipaddr, 4 );
	app_printf( "xlink_upgrade_tcp_client tcp ip is %d.%d.%d.%d", xlink_upgrade_tcp_client.proto.tcp->remote_ip[0],
																			xlink_upgrade_tcp_client.proto.tcp->remote_ip[1],
																			xlink_upgrade_tcp_client.proto.tcp->remote_ip[2],
																			xlink_upgrade_tcp_client.proto.tcp->remote_ip[3]);

	xlink_upgrade_tcp_client.type = ESPCONN_TCP;
	xlink_upgrade_tcp_client.state = ESPCONN_NONE;
	local_port = espconn_port();
	if( local_port == 8089 || local_port == 5001 )
	{
		local_port += 5;
	}
	xlink_upgrade_tcp_client.proto.tcp->local_port = local_port;
	xlink_upgrade_tcp_client.proto.tcp->remote_port = xlink_upgrade_info.port;
	espconn_regist_connectcb( &xlink_upgrade_tcp_client, xlink_upgrade_connect_cb );
	espconn_regist_reconcb( &xlink_upgrade_tcp_client, xlink_upgrade_reconnect_cb );
	espconn_connect( &xlink_upgrade_tcp_client );
}

void XLINK_FUNCTION xlink_upgrade_dns_found( const char *name, ip_addr_t *ipaddr, void *arg )
{
	struct espconn *pespconn = (struct espconn *) arg;
	uint8_t addr[4];

	if ( ipaddr == NULL )
	{
		xlink_upgrade_state_notify( UPGRADE_MSG_FAILED );
		app_printf( "ipaddr is NULL" );
		return;
	}
	else
	{
		app_printf( "ipaddr is %d.%d.%d.%d", 	*((uint8_t *)&ipaddr->addr),
												*((uint8_t *)&ipaddr->addr+1),
												*((uint8_t *)&ipaddr->addr+2),
												*((uint8_t *)&ipaddr->addr+3) );
	}
	os_memset( addr, 0, sizeof(addr) );

//	if ( xlink_upgrade_ip.addr == 0 && ipaddr->addr != 0 )
//	{
//		xlink_upgrade_ip.addr = ipaddr->addr;
//		os_memcpy( addr, &ipaddr->addr, 4 );
//		xlink_upgrade_tcp_client_init( addr );
//	}
	if ( ipaddr->addr != 0 )
	{
		os_memcpy( addr, &ipaddr->addr, 4 );
		xlink_upgrade_tcp_client_init( addr );
	}
}

void XLINK_FUNCTION xlink_upgrade_start_dns( const char *hostname )
{
	xlink_upgrade_ip.addr = 0;
	espconn_gethostbyname( &xlink_upgrade_tcp_client, hostname, &xlink_upgrade_ip, xlink_upgrade_dns_found );
}

//int XLINK_FUNCTION xlink_upgrade_get_path( uint8_t *url, uint8_t *path )
//{
//	uint8_t *p = os_strstr( url, "http://" );
//	uint8_t offset = 0;
//	if ( p != NULL )
//	{
//		offset = os_strlen( "http://" );
//	}
//	p = os_strchr( url + offset, '/' );
//	if ( p == NULL )
//	{
//		return -1;
//	}
//	os_strcpy( path, p+1 );
//	return 0;
//}
//
//int XLINK_FUNCTION xlink_upgrade_get_ip_port( uint8_t *url, uint8_t *ip, uint8_t *port )
//{
//	uint8_t *p = os_strstr( url, "http://" );
//	int offset = 0;
//	uint8_t domain[128];
//	if ( p != NULL )
//	{
//		offset = os_strlen( "http://" );
//	}
//	p = os_strchr( url + offset, '/' );
//	if ( p == NULL )
//	{
//		return -1;
//	}
//	os_memset( domain, 0, sizeof( domain ) );
//	os_memcpy( domain, url + offset, p - url - offset );
//	p = os_strchr( domain, ':' );
//	if ( p == NULL )
//	{
//		os_strcpy( ip, domain );
//		os_strcpy( port, "80" );
//		return 1;
//	}
//	*p = '\0';
//	os_strcpy( ip, domain );
//	os_strcpy( port, p+1 );
//	return 2;
//}

bool XLINK_FUNCTION xlink_upgrade_get_info( const char *url, xlink_upgrade_info_t *pinfo )
{
	char *p = os_strstr( url, "http://" );
	int offset = 0;
	char domain[128];
	char port[8];
	if ( p != NULL )
	{
		offset = os_strlen( "http://" );
	}
	p = os_strchr( url + offset, '/' );
	if ( p == NULL )
	{
		return false;
	}
	os_memset( pinfo, 0, sizeof( xlink_upgrade_info_t ) );
	os_strcpy( pinfo->path, p+1 );
	os_memset( domain, 0, sizeof( domain ) );
	os_memcpy( domain, url + offset, p - url - offset );
	p = os_strchr( domain, ':' );
	if ( p == NULL )
	{
		os_strcpy( port, "80" );
	}
	else
	{
		*p = '\0';
		os_strcpy( port, p+1 );
	}
	os_strcpy( pinfo->ipaddr, domain );
	pinfo->port = atoi( port );
	return true;
}

//int XLINK_FUNCTION xlink_upgrade_start_download( xlink_download_file_info_t *pfile_info )
//{
//	uint8_t error = 0;
//	uint8_t port[8];
//	uint8_t temp[256];
//
//	memset( port, 0, sizeof(port) );
//	memset( &xlink_upgrade_server, 0, sizeof( xlink_upgrade_server ) );
//
//	memset( temp, 0, sizeof( temp ) );
//	memcpy( temp, pfile_info->url, pfile_info->url_length );
//	error = xlink_upgrade_get_path( temp, xlink_upgrade_info.path );
//	if ( error < 0 )
//	{
//		user_printf( "get path failed..." );
//		return 0;
//	}
//
//	memset( temp, 0, sizeof( temp ) );
//	memcpy( temp, pfile_info->url, pfile_info->url_length );
//	error = xlink_upgrade_get_ip_port( temp, xlink_upgrade_info.ipaddr, port );
//	if ( error < 0 )
//	{
//		user_printf( "get ip port failed..." );
//		return 0;
//	}
//
//	xlink_upgrade_info.port = atoi( port );
//	user_printf( "get ip=%s port=%d", xlink_upgrade_info.ipaddr, xlink_upgrade_info.port );
//	xlink_upgrade_start_dns( &xlink_upgrade_info.ipaddr );
//}

void XLINK_FUNCTION xlink_upgrade_start( const char *url )
{
	uint8_t ipaddr[4];
	if ( xlink_upgrade_get_info( url, &xlink_upgrade_info ) )
	{
		xlink_upgrade_state_notify( UPGRADE_MSG_START );
		if( xlink_check_ip( xlink_upgrade_info.ipaddr, ipaddr ) )
		{
			xlink_upgrade_tcp_client_init( ipaddr );
		}
		else
		{
			xlink_upgrade_start_dns( xlink_upgrade_info.ipaddr );
		}
	}
	else
	{
		xlink_upgrade_state_notify( UPGRADE_MSG_FAILED );
		app_printf( "Invalid upgrade url..." );
	}
}

