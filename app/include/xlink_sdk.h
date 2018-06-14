//
// Created by huangjinqiang on 2017/3/23.
//

#ifndef _XLINK_SDK_H_
#define _XLINK_SDK_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "c_types.h"
#include "ip_addr.h"
#include "upgrade.h"
#include "espconn.h"
/*self define write here,not sdk define*/
typedef union {
	unsigned int ip;
	struct {
		unsigned char byte0 :8;
		unsigned char byte1 :8;
		unsigned char byte2 :8;
		unsigned char byte3 :8;
	}bit;
}ip_address_t;
typedef unsigned int xsdk_time_t;

/*self define end*/


//define xlink datatype,modify by different system
#define xlink_int8   char
#define xlink_uint8  unsigned char
#define xlink_int16  short
#define xlink_uint16 unsigned short
#define xlink_int32  int
#define xlink_uint32 unsigned int
#define xlink_int64  long long
#define xlink_uint64 unsigned long long
#define xlink_double double
#define xlink_null   NULL
#define xlink_enum   enum
#define xlink_union  union

#define xlink_sizeof sizeof

#define   xlink_strlen(x)       os_strlen((char*)(x))
#define   xlink_strcmp(x,t)     os_strcmp((char*)(x),(char*)(t))
#define   xlink_strncmp(x,t,z)  os_strncmp((char*)(x),(char*)(t),z)
#define   xlink_memset(x,d,t)   os_memset(x,d,t)
#define   xlink_memcpy(x,d,l)   os_memcpy(x,d,l)
#define   xlink_msleep(n)       xlink_delay(n);
#define   xlink_sprintf         os_sprintf
#define   xlink_snprintf         os_sprintf

#define   xlink_close(x)        xclose(x)
#define   xlink_printf          os_printf
#define   xlink_strchr          os_strchr
#define   xlink_delay_ms  
#define   xlink_memcmp    os_memcmp

#define   snprintf		os_sprintf
#define   sprintf		os_sprintf
#define   printf        os_printf

#define no_libc 0
#define driver_on 1

//define xlink sdk version
#define XLINK_SDK_VERSION 5350
#define XLINK_FUNCTION ICACHE_FLASH_ATTR

//define xlink_sdk
#define XLINK_DEV_NAME_MAX 16
#define XLINK_DEV_MAC_LENGTH_MIN 1
#define XLINK_DEV_MAC_LENGTH_MAX 32
#define XLINK_SYS_PARA_LENGTH (1024*3)
#define XLINK_PACKET_LENGTH_MAX 1000

//xlink addr
typedef struct xlink_addr_t {
    xlink_int32 socket;
    xlink_uint32 ip;
    xlink_uint16 port;
} xlink_addr_t;

//xlink sdk configuration struct
typedef struct xlink_sdk_instance_t {
    //device product id
    xlink_uint8 *dev_pid;
    //device product key
    xlink_uint8 *dev_pkey;
    //device name
    xlink_uint8 dev_name[XLINK_DEV_NAME_MAX + 1];
    //device mac
    xlink_uint8 dev_mac[XLINK_DEV_MAC_LENGTH_MAX];
    //device mac length
    xlink_uint8 dev_mac_length;
    //device firmware version
    xlink_uint16 dev_firmware_version;
    //enable cloud
    xlink_uint8 cloud_enable;
    //enable local
    xlink_uint8 local_enable;
    //user certification
    xlink_uint8 *certificate_id;
    //user certification length
    xlink_int16 certificate_id_length;
    //user certification key
    xlink_uint8 *certificate_key;
    //user certification key
    xlink_int16 certificate_key_length;
    //log level
    xlink_uint8 log_level;
    //log enable
    xlink_uint8 log_enable;
    //cloud rec buffer
    xlink_uint8 *cloud_rec_buffer;
    //cloud rec buffer length;
    xlink_int32 cloud_rec_buffer_length;
    //system para,user don't care
    xlink_uint8 sdk_para[XLINK_SYS_PARA_LENGTH];
	//device sn length         new add
	xlink_uint16 dev_sn_length;
	//device sn
	xlink_uint8 *dev_sn;
} xlink_sdk_instance_t;

//xlink sdk event enum
typedef xlink_enum {
    EVENT_TYPE_STATUS = 0,
    EVENT_TYPE_REQ_DATETIME,
    EVENT_TYPE_REQ_DATETIME_CB,
    EVENT_TYPE_UPGRADE_CB,
    EVENT_TYPE_UPGRADE_COMPLETE,
    EVENT_TYPE_REQUEST_CB,
    EVENT_TYPE_NOTIFY
} xlink_enum_event_type_t;

//xlink device connect station struct
typedef struct xlink_status_t {
    xlink_uint8 status;
} xlink_status_t;

//xlink sdk datetime struct
typedef struct xlink_datetime_t {
    xlink_uint16 year;
    xlink_uint8 month;
    xlink_uint8 day;
    xlink_uint8 week;
    xlink_uint8 hour;
    xlink_uint8 min;
    xlink_uint8 second;
    xlink_int16 zone;
} xlink_datetime_t;

//xlink sdk upgrade struct
typedef struct xlink_upgrade_t {
    xlink_int32 device_id;
    xlink_uint8 flag;
    xlink_uint16 firmware_version;
    xlink_uint32 file_size;
    xlink_uint8 *url;
    xlink_int16 url_length;
    xlink_uint8 *hash;
    xlink_int16 hash_length;
} xlink_upgrade_t;

//xlink sdk upgrade complete struct
typedef struct xlink_upgrade_complete_t {
    xlink_uint8 flag;
    xlink_uint8 status;
    xlink_uint16 last_version;
    xlink_uint16 current_version;
} xlink_upgrade_complete_t;

//xlink sdk request call back struct
typedef struct xlink_request_cb_t {
    xlink_uint16 messageid;
    xlink_uint32 value;
} xlink_request_cb_t;

//xlink notify struct
typedef struct xlink_sdk_notify_t {
    xlink_uint8 from_type;//1:server,2:devcie,3:app
    xlink_int32 from_id;
    xlink_uint8 *message;
    xlink_int16 message_length;
} xlink_sdk_notify_t;

//xlink sdk event struct
typedef struct xlink_sdk_event_t {
    xlink_enum_event_type_t enum_event_type_t;
    xlink_union {
        struct xlink_status_t status;
        struct xlink_datetime_t datetime_t;
        struct xlink_upgrade_t upgrade_t;
        struct xlink_upgrade_complete_t upgrade_complete_t;
        struct xlink_request_cb_t request_cb_t;
        struct xlink_sdk_notify_t notify_t;
    } event_struct_t;
} xlink_sdk_event_t;

/*add by kaven*/
//xlink sdk location struct
typedef struct xlink_location_t {
    xlink_double longitude;
    xlink_double latitude;
    xlink_uint64 timestamp;
    xlink_uint8 timestamp_flag;//if timestamp is not use,timestamp_flag must be 0
    xlink_uint16 address_length;//if address is not use,address_length must be 0
    xlink_uint8 *address;
} xlink_location_t;


//interface funtion
//sdk funtion
XLINK_FUNCTION extern xlink_int32 xlink_sdk_init(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_sdk_uninit(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_sdk_connect_cloud(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_sdk_disconnect_cloud(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNCTION extern void xlink_sdk_process(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_sdk_reset(struct xlink_sdk_instance_t **sdk_instance);
//sdk call back funtiong
XLINK_FUNCTION extern xlink_int32 xlink_write_flash_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength);
XLINK_FUNCTION extern xlink_int32 xlink_read_flash_cb(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength);
XLINK_FUNCTION extern xlink_int32 xlink_send_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr_t, xlink_uint8 flag);
XLINK_FUNCTION extern xlink_int32 xlink_get_datapoint_cb(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer, xlink_int32 datamaxlength);
XLINK_FUNCTION extern xlink_int32 xlink_set_datapoint_cb(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength);
XLINK_FUNCTION extern void xlink_event_cb(struct xlink_sdk_instance_t **sdk_instance, const struct xlink_sdk_event_t **event_t);
XLINK_FUNCTION extern xlink_uint32 xlink_get_ticktime_ms_cb(struct xlink_sdk_instance_t **sdk_instance);
//sdk send data funsion
XLINK_FUNCTION extern xlink_int32 xlink_update_datapoint(struct xlink_sdk_instance_t **sdk_instance, xlink_uint16 *messageid, const xlink_uint8 **data, xlink_int32 datamaxlength, xlink_uint8 flag);
XLINK_FUNCTION extern xlink_int32 xlink_request_event(struct xlink_sdk_instance_t **sdk_instance, xlink_uint16 *messageid, struct xlink_sdk_event_t **event_t);
XLINK_FUNCTION extern xlink_int32 xlink_receive_data(struct xlink_sdk_instance_t **sdk_instance, const xlink_uint8 **data, xlink_int32 datalength, const xlink_addr_t **addr_t, xlink_uint8 flag);
XLINK_FUNCTION extern xlink_int32 xlink_get_device_id(struct xlink_sdk_instance_t **sdk_instance);
XLINK_FUNCTION extern xlink_int32 xlink_get_device_key(struct xlink_sdk_instance_t **sdk_instance, xlink_uint8 **buffer);
XLINK_FUNCTION extern xlink_int32 xlink_report_log(struct xlink_sdk_instance_t** sdk_instance, xlink_uint8 log_level, xlink_uint8** data, xlink_uint32 datalength);
/*add by kaven*/
XLINK_FUNCTION extern xlink_int32 xlink_upload_location_data(struct xlink_sdk_instance_t **sdk_instance, struct xlink_location_t **xlink_location);
/*add by kaven*/

#ifdef  __cplusplus
}
#endif

#endif //_XLINK_SDK_H_
