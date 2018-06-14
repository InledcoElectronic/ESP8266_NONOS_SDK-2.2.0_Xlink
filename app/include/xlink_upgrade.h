/*
 * xlink_upgrade.h
 *
 *  Created on: 2018年5月14日
 *      Author: liruya
 */

#ifndef XLINK_UPGRADE_H_
#define XLINK_UPGRADE_H_

#include "os_type.h"
#include "xlink_sdk.h"

#define XLINK_UPGRADE_URL_MAX_LENGTH	128

enum ERROR_CDOE{
	SUCCESS = 0,
	NULL_POINTER,
	OVER_BOUNDARY,
	INVALID_CHAR,
	INVALID_FORM
};

typedef struct{
	uint8_t ipaddr[72];
	uint16_t port;
	uint8_t path[128];
} xlink_upgrade_info_t;

typedef void (*xlink_upgrade_status_callback_t) ( const uint32_t file_size, const uint32_t rate, const uint32_t status );
typedef void (*xlink_upgrade_receive_data_callback_t) ( const uint8_t *pdata, uint32_t datalen, const uint32_t file_size, const uint32_t offset );
typedef int (*xlink_upgrade_error_callback_t) ( const uint8_t error );

extern void xlink_upgrade_start( const char *url );
//extern void xlink_upgrade_state_notify( void* arg );

#endif /* XLINK_UPGRADE_H_ */
