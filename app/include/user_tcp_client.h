/*
 * user_tcp_client.h
 *
 *  Created on: 2017年9月7日
 *      Author: liruya
 */

#ifndef _USER_TCP_CLIENT_H_
#define _USER_TCP_CLIENT_H_

#include "app_config.h"
#include "user_net.h"
#include "espconn.h"
#include "xlink_sdk.h"

extern bool isConnectServer;

extern uint32_t user_tcp_send( uint8_t *data, uint16_t datalen );
extern void user_tcp_func_process( void *arg );
extern void user_tcp_client_init( void );
extern void user_tcp_disconnect( void );

#endif /* _USER_TCP_CLIENT_H_ */
