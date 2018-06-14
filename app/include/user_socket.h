/*
 * user_socket.h
 *
 *  Created on: 2018年5月28日
 *      Author: liruya
 */

#ifndef USER_SOCKET_H_
#define USER_SOCKET_H_

#include "xlink_datapoint.h"
#include "xlink.h"
#include "eagle_soc.h"
#include "app_board_socket.h"
#include "app_config.h"

#define SOCKET_CHANNEL_MAX			8
#define SOCKET_CHANNEL_COUNT		1
#if	SOCKET_CHANNEL_COUNT > 8 || SOCKET_CHANNEL_COUNT < 1
#error "SOCKET_CHANNEL_COUNT:1~8"
#endif
#define SOCKET_TIMER_MAX_LENGTH		64
#define SOCKET_TIMER_MAX			15

#define SENSOR_COUNT_MAX			4

#define POWER_ON					1
#define POWER_OFF					0

#define SOCKET1_POWER_INDEX			17
#define SOCKET2_POWER_INDEX			20
#define SOCKET3_POWER_INDEX			23
#define SOCKET4_POWER_INDEX			26
#define SOCKET5_POWER_INDEX			29
#define SOCKET6_POWER_INDEX			32
#define SOCKET7_POWER_INDEX			35
#define SOCKET8_POWER_INDEX			38
#define SOCKET_ACTION_INDEX_LIST	17,20,23,26,29,32
#define	TEMPERATURE_LOW_RANGE		15..25
#define	TEMPERATURE_HIGH_RANGE		25..35

#define LINKAGE_TYPE_LOW_TEMPERATURE	1
#define LINKAGE_TYPE_HIGH_TEMPERATURE	2
#define TEMPERATURE_LOW_TURNON_SOCKET	"{type:" LINKAGE_TYPE_LOW_TEMPERATURE ",\
											express:temp<${" TEMPERATURE_LOW_RANGE "}->turn on socket ${" SOCKET_ACTION_INDEX_LIST "}"
#define TEMPERATURE_HIGH_TURNOFF_SOCKET	"{type:" LINKAGE_TYPE_HIGH_TEMPERATURE ",\
											express:temp>${" TEMPERATURE_HIGH_RANGE "}->turn off socket ${" SOCKET_ACTION_INDEX_LIST "}"

typedef enum _sensor_type {
	SENSOR_UNKOWN,
	SENSOR_TEMPERATURE,
	SENSOR_HUMIDITY,
	SENSOR_WATERLEVEL,
	SENSOR_INTENSITY,
	SENSOR_INVALID
} sensor_type_t;

typedef enum _linkage_type{
	LINKAGE_DISABLED,
	LINKAGE_EQ,					//equal
	LINKAGE_NEQ,				//not equal
	LINKAGE_LT,					//less than
	LINKAGE_LTE,				//less than or equal
	LINKAGE_GT,					//greater than
	LINKAGE_GTE,				//greater than or equal
	LINKAGE_INVALID
}linkage_type_t;

typedef struct {
	unsigned sensor_type : 6;
	unsigned linkage_type : 6;
	unsigned socket_index : 4;
	unsigned threshold : 16;
}linkage_t;

typedef struct{
	bool available;
	uint8_t sensor_type;
	int16_t value;
}sensor_t;

typedef struct{
	unsigned timer : 15;
	unsigned power_state : 1;
	unsigned repeat : 7;
	unsigned enable : 1;
	unsigned flag : 8;
}socket_timer_t;

typedef struct{
	uint32_t count;
	socket_timer_t timers[SOCKET_TIMER_MAX];
}socket_timer_list_t;

typedef struct{
	uint8_t pin_num;
	bool power;
	socket_timer_list_t * const p_timer;
}single_socket_t;

typedef struct{
	uint8_t saved_flag;
	uint16_t zone;

	linkage_t linkage[SENSOR_COUNT_MAX];
	socket_timer_list_t socket_timers[SOCKET_CHANNEL_COUNT];
}socket_config_t;

typedef struct{
	xlink_device_t xlink_device;

	const uint8_t channel_count;

	sensor_t sensor[SENSOR_COUNT_MAX];

	single_socket_t m_socket[SOCKET_CHANNEL_COUNT];
}user_socket_t;

extern user_socket_t user_socket;

#endif /* USER_SOCKET_H_ */

