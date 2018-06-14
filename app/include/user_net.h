/*
 * user_net.h
 *
 *  Created on: 2017年9月7日
 *      Author: liruya
 */

#ifndef _USER_NET_H_
#define _USER_NET_H_

#include "user_udp_server.h"
#include "user_tcp_client.h"

#define XLINK_DOMAIN    	"mqtt.xlink.cn"
//#define XLINK_DOMAIN    	"cm2.xlink.cn"
//#define XLINK_DOMAIN    	"static.xlink.cn"

//#define SSL_ENABLE
#define	UDP_LOCAL_PORT		10000
#define	UDP_REMOTE_PORT		6000
#define	TCP_LOCAL_PORT		8089
#ifdef	SSL_ENABLE
#define	TCP_REMOTE_PORT		1884
#else
#define	TCP_REMOTE_PORT		1883
#endif

//#define	UDP_LOCAL_PORT		5987
//#define	UDP_REMOTE_PORT		6000
//#define	TCP_LOCAL_PORT		5001
//#ifdef	SSL_ENABLE
//#define	TCP_REMOTE_PORT		1884
//#else
//#define	TCP_REMOTE_PORT		23778
//#endif

extern void user_net_init( void );

#endif /* APP_USER_USER_NET_H_ */
