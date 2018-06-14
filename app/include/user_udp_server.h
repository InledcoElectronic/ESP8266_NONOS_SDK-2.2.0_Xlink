/*
 * user_udp_server.h
 *
 *  Created on: 2017年9月7日
 *      Author: liruya
 */

#ifndef _USER_UDP_SERVER_H_
#define _USER_UDP_SERVER_H_

#include "user_net.h"
#include "espconn.h"
#include "xlink_sdk.h"

//extern void user_udp_transfer_init( espconn_recv_callback callback );
extern void user_udp_server_init( void );
extern void user_udp_recv_cb( void *arg, char *pdata, uint16_t len );
extern uint32_t user_udp_send( xlink_addr_t *addr, uint8_t *pdata, uint16_t len );

#endif /* _USER_UDP_SERVER_H_ */
