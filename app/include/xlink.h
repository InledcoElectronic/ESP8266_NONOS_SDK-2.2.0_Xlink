/*
 * XLINK.h
 *
 *  Created on: 2017��9��2��
 *      Author: liruya
 */

#ifndef __XLINK_H__
#define __XLINK_H__

#include "xlink_sdk.h"
#include "osapi.h"
#include "app_config.h"

#define XLINK_UPGRADE_URL_MAX_LENGTH		128

typedef struct{
	const char product_id[33];
	const char product_key[33];
	const uint16_t firmware_version;

	void (* const init)();
	void (* datapoint_changed_cb)();
}xlink_device_t;

#define newXlinkDevice(id,key,ver,devinit,dp_changed_cb)	{.product_id=(id),\
															 .product_key=(key),\
															 .firmware_version=(ver),\
															 .init=(devinit),\
															 .datapoint_changed_cb=(dp_changed_cb)}

extern void xlink_connect_cloud();
extern void xlink_disconnect_cloud();
extern void xlink_init();
extern void xlink_reset();
extern int xlink_get_deviceid();
//extern void xlink_post_event(xlink_uint16 *msgid, struct xlink_sdk_event_t **event);
//extern xlink_int32 xlink_receive_tcp_data( const xlink_uint8 **data, xlink_int32 datalength );
//extern xlink_int32 xlink_receive_udp_data( const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr );
extern void xlink_process();
//extern void xlink_update_datapoint_with_alarm( uint16_t *messageid, const uint8_t **data, uint32_t datamaxlength );
//extern void xlink_update_datapoint_no_alarm( uint16_t *messageid, const uint8_t **data, uint32_t datamaxlength );

#endif /* __XLINK_H__ */
