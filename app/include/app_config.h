/*
 * user_config.h
 *
 *  Created on: 2018年5月7日
 *      Author: liruya
 */

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#define	USE_TX_DEBUG

#ifdef	USE_TX_DEBUG
#define app_printf( format, ... )	os_printf_plus( "[%s]->[%s] @%4d: " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ )
#else
#define	app_printf( format, ... )
#endif

#endif /* APP_CONFIG_H_ */
