/*
 * xlink.c
 *
 *  Created on: 2017年9月11日
 *      Author: liruya
 */

#include "xlink_sdk.h"
#include "xlink.h"

#include "app_config.h"
#include "xlink_config.h"
#include "xlink_datapoint.h"

#define RECV_BUFFER_SIZE	2048
#define XLINK_CERTIFY_ID 	"591929aca26e6a07dec2edd4"
#define XLINK_CERTIFY_KEY 	"b7409d07-0612-440e-95dc-3e9cb9478ab2"

LOCAL uint8_t recv_buffer[RECV_BUFFER_SIZE];
LOCAL struct xlink_sdk_instance_t g_xlink_sdk_instance;
LOCAL struct xlink_sdk_instance_t *p_xlink_sdk_instance;
LOCAL uint16_t msgid;

void ICACHE_FLASH_ATTR xlink_init( xlink_device_t *pdev )
{
	uint16_t version;
	xlink_read_version( &version );
	if ( version > pdev->firmware_version )
	{
		xlink_write_version( pdev->firmware_version );
	}
	os_memset( &g_xlink_sdk_instance, 0, sizeof( g_xlink_sdk_instance ) );
	p_xlink_sdk_instance = &g_xlink_sdk_instance;
	g_xlink_sdk_instance.dev_pid = (uint8_t *) pdev->product_id;
	g_xlink_sdk_instance.dev_pkey = (uint8_t *) pdev->product_key;
	os_memcpy( g_xlink_sdk_instance.dev_name, "xlink_dev", sizeof("xlink_dev") );
	g_xlink_sdk_instance.dev_mac_length = 6;
	wifi_get_macaddr( STATION_IF, (uint8_t *) g_xlink_sdk_instance.dev_mac );
	g_xlink_sdk_instance.cloud_rec_buffer = recv_buffer;
	g_xlink_sdk_instance.cloud_rec_buffer_length = sizeof( recv_buffer );
	g_xlink_sdk_instance.dev_firmware_version = pdev->firmware_version;
	g_xlink_sdk_instance.certificate_id = XLINK_CERTIFY_ID;
	g_xlink_sdk_instance.certificate_id_length = os_strlen( XLINK_CERTIFY_ID );
	g_xlink_sdk_instance.certificate_key = XLINK_CERTIFY_KEY;
	g_xlink_sdk_instance.certificate_key_length = os_strlen( XLINK_CERTIFY_KEY );
	g_xlink_sdk_instance.cloud_enable = 1;
	g_xlink_sdk_instance.local_enable = 1;
	g_xlink_sdk_instance.log_enable = 0;
	g_xlink_sdk_instance.log_level = 0;

	xlink_sdk_init( &p_xlink_sdk_instance );

	pdev->init();
	xlink_setOnDatapointChangedCallback( pdev->datapoint_changed_cb );
}

bool XLINK_FUNCTION xlink_check_ip( const char *ip, uint8_t *ipaddr )
{
	uint8_t dot = 0;
	bool first_num_got = false;
	char first_num;
	int16_t val = -1;
	char temp;

	while( *ip != '\0' )
	{
		temp = *ip++;
		if ( temp >= '0' && temp <= '9' )
		{
			if ( first_num_got == false )
			{
				first_num_got = true;
				first_num = temp;
			}
			else if ( first_num == '0' )
			{
				return false;
			}
			if ( val < 0 )
			{
				val = 0;
			}
			val = val * 10 + (temp -'0');
			if ( val > 255 )
			{
				return false;
			}
			ipaddr[dot] = val;
		}
		else if ( temp == '.' )
		{
			/* .前面必须为 数字且在 0 ~ 255 之间  */
			if ( val < 0 && val > 255 )
			{
				return false;
			}
			dot++;
			if ( dot > 3 )
			{
				return false;
			}
			val = -1;
			first_num_got = false;
		}
		else
		{
			return false;
		}
	}
	return dot == 3 && val >= 0 && val <= 255;
}

int XLINK_FUNCTION xlink_get_deviceid()
{
	return xlink_get_device_id( &p_xlink_sdk_instance );
}

void XLINK_FUNCTION xlink_reset()
{
	xlink_sdk_reset( &p_xlink_sdk_instance );
}

void XLINK_FUNCTION xlink_connect_cloud()
{
	xlink_sdk_connect_cloud( &p_xlink_sdk_instance );
}

void XLINK_FUNCTION xlink_disconnect_cloud()
{
	xlink_sdk_disconnect_cloud( &p_xlink_sdk_instance );
}

void XLINK_FUNCTION xlink_post_event( xlink_uint16 *msgid, struct xlink_sdk_event_t **event )
{
	xlink_request_event( &p_xlink_sdk_instance, msgid, event );
}

xlink_int32 XLINK_FUNCTION xlink_receive_tcp_data( const xlink_uint8 **data, xlink_int32 datalength )
{
	return xlink_receive_data( &p_xlink_sdk_instance, data, datalength, NULL, 1 );
}

xlink_int32 XLINK_FUNCTION xlink_receive_udp_data( const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr )
{
	return xlink_receive_data( &p_xlink_sdk_instance, data, datalength, addr, 0 );
}

void XLINK_FUNCTION xlink_process()
{
	xlink_sdk_process( &p_xlink_sdk_instance );
}

void XLINK_FUNCTION xlink_report_version( uint16_t previous_version, uint16_t current_version )
{
	uint16_t msgid = 0;
	int ret = 0;
	struct xlink_sdk_event_t event;
	struct xlink_sdk_event_t *pevent;
	event.enum_event_type_t = EVENT_TYPE_UPGRADE_COMPLETE;
	event.event_struct_t.upgrade_complete_t.current_version = current_version;
	event.event_struct_t.upgrade_complete_t.last_version = previous_version;
	event.event_struct_t.upgrade_complete_t.status = 1;
	event.event_struct_t.upgrade_complete_t.flag = 0x80;
	pevent = &event;
	ret = xlink_request_event( &p_xlink_sdk_instance, &msgid, &pevent );
	if ( ret == 0 )
	{
		xlink_write_version( p_xlink_sdk_instance->dev_firmware_version );
	}
}

void XLINK_FUNCTION xlink_event_cb( struct xlink_sdk_instance_t **sdk_instance, const struct xlink_sdk_event_t **event_t )
{
	struct xlink_sdk_event_t *pevent = ( struct xlink_sdk_event_t * ) *event_t;
	uint16_t prev_version;
	int16_t disv;
	switch ( pevent->enum_event_type_t )
	{
		case EVENT_TYPE_STATUS:
			if ( pevent->event_struct_t.status.status == 0 )
			{
				app_printf( "xlink cloud disconnected..." );
				xlink_disconnect_cloud();
			}
			else
			{
				xlink_read_version( &prev_version );
				if ( prev_version < p_xlink_sdk_instance->dev_firmware_version )
				{
					xlink_report_version( prev_version, p_xlink_sdk_instance->dev_firmware_version );
				}
				xlink_datapoint_update_all();
				user_rtc_get_time();
				app_printf( "prev_version:%d   xlink cloud connected...", prev_version );
			}
			break;
		case EVENT_TYPE_REQUEST_CB:

			break;
		case EVENT_TYPE_REQ_DATETIME_CB:
			app_printf( "sync cloud time: %d-%d-%d %d:%d:%d week-%d zone-%d", 	pevent->event_struct_t.datetime_t.year,
																				pevent->event_struct_t.datetime_t.month,
																				pevent->event_struct_t.datetime_t.day,
																				pevent->event_struct_t.datetime_t.hour,
																				pevent->event_struct_t.datetime_t.min,
																				pevent->event_struct_t.datetime_t.second,
																				pevent->event_struct_t.datetime_t.week,
																				pevent->event_struct_t.datetime_t.zone );
			user_rtc_sync_cloud_cb( pevent );
			break;
		case EVENT_TYPE_UPGRADE_CB:
			app_printf( "firmware version: %d url:%s", pevent->event_struct_t.upgrade_t.firmware_version, pevent->event_struct_t.upgrade_t.url );
			if ( pevent->event_struct_t.upgrade_t.url_length <= 0 || pevent->event_struct_t.upgrade_t.url == NULL )
			{
				return;
			}
			if ( pevent->event_struct_t.upgrade_t.firmware_version > p_xlink_sdk_instance->dev_firmware_version )
			{
				int16_t disv = pevent->event_struct_t.upgrade_t.firmware_version - p_xlink_sdk_instance->dev_firmware_version;
				if ( disv%2 == 0 )
				{
					app_printf( "upgrade task invalid target version..." );
					return;
				}
				uint8_t url[128];
				uint16_t url_length = pevent->event_struct_t.upgrade_t.url_length;
				if(url_length < 128)
				{
					os_memset( url, 0, 128 );
					os_memcpy( url, pevent->event_struct_t.upgrade_t.url, url_length );
					xlink_upgrade_start( url );
				}
				else
				{
					app_printf( "url link too long..." );
				}
			}
			else
			{
				xlink_read_version( &prev_version );
				xlink_report_version( prev_version, p_xlink_sdk_instance->dev_firmware_version );
			}
			break;
		case EVENT_TYPE_UPGRADE_COMPLETE:
			app_printf( "upgrade complete..." );
			break;
		case EVENT_TYPE_NOTIFY:
			app_printf( "notify type: %d  %s", pevent->event_struct_t.notify_t.from_type, pevent->event_struct_t.notify_t.message );
			break;
		default:
			break;
	}
}

xlink_int32 XLINK_FUNCTION xlink_send_cb( struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength,
        const xlink_addr_t **addr_t, xlink_uint8 flag )
{
	xlink_uint8 *pdata = (xlink_uint8 *) *data;
	if ( flag )
	{
		user_tcp_send( pdata, datalength );
	}
	else
	{
		xlink_addr_t *paddr = ( xlink_addr_t * ) *addr_t;
		user_udp_send( paddr, pdata, datalength );
	}
	return datalength;
}

xlink_uint32 XLINK_FUNCTION xlink_get_ticktime_ms_cb( struct xlink_sdk_instance_t **sdk_instance )
{
    return (system_get_time()/1000);
}

xlink_int32 XLINK_FUNCTION xlink_set_datapoint_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength)
{
	xlink_uint8 *pdata = ( xlink_uint8 * ) *data;
	xlink_array_to_datapoints( pdata, datalength );
	return datalength;
}

xlink_int32 XLINK_FUNCTION xlink_get_datapoint_cb(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength)
{
	xlink_uint8 *data = ( xlink_uint8 * ) *buffer;
	return xlink_datapoints_to_array( data );
}

xlink_int32 XLINK_FUNCTION xlink_write_flash_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength)
{
	uint8_t *pdata = ( uint8_t * ) *data;
	xlink_write_config( pdata, datalength );
	return datalength;
}

xlink_int32 XLINK_FUNCTION xlink_read_flash_cb(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength)
{
	uint8_t *pdata = ( uint8_t * ) *buffer;
	xlink_read_config( pdata, datamaxlength );
	return datamaxlength;
}

xlink_int32 XLINK_FUNCTION xlink_update_datapoint_with_alarm( const xlink_uint8 **data, xlink_int32 datamaxlength )
{
	msgid++;
	return xlink_update_datapoint( &p_xlink_sdk_instance, &msgid, data, datamaxlength, 1 );
}

xlink_int32 XLINK_FUNCTION xlink_update_datapoint_no_alarm( const xlink_uint8 **data, xlink_int32 datamaxlength )
{
	msgid++;
	return xlink_update_datapoint( &p_xlink_sdk_instance, &msgid, data, datamaxlength, 0 );
}

xlink_int32 XLINK_FUNCTION xlink_get_rssi_cb(struct xlink_sdk_instance_t **sdk_instance, xlink_uint16 *result, xlink_int16 *rssi, xlink_uint16 *AP_STA)
{
	xlink_int32 ret = -1;
	uint8_t wifimode, wifirssi;
	wifimode = wifi_get_opmode();
	if(wifimode == 0x01)
	{
		wifimode = 0;
	}
	else
	{
		wifimode = 1;
	}
	wifirssi = wifi_station_get_rssi();
	if(wifirssi == 32)
	{
		return -1;
	}
	*result = ret;
	*rssi = wifirssi;
	*AP_STA = wifimode;
	return ret;
}

xlink_int32 XLINK_FUNCTION xlink_get_custom_test_data_cb(struct xlink_sdk_instance_t **sdk_instance,  xlink_uint16 *result,xlink_uint8 **data, xlink_int32 datamaxlength)
{

}

xlink_int32 XLINK_FUNCTION xlink_probe_datapoint_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **dp_idx, xlink_uint8 dp_idx_length, xlink_uint8 **buffer, xlink_int32 datamaxlength)
{
	xlink_uint8 *dp_index = (xlink_uint8 *) *dp_idx;
	xlink_uint8 *data = (xlink_uint8 *) *buffer;
	return xlink_probe_datapoints_to_array(dp_index, dp_idx_length, data);
}



