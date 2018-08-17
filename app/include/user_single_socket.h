/*
 * user_socket.h
 *
 *  Created on: 2018年5月28日
 *      Author: liruya
 */

#ifndef USER_SINGLE_SOCKET_H_
#define USER_SINGLE_SOCKET_H_

#include "xlink_datapoint.h"
#include "xlink.h"
#include "eagle_soc.h"
#include "app_board_socket.h"
#include "app_config.h"
#include "user_sensor.h"

#define SOCKET_TIMER_MAX_LENGTH		64
#define SOCKET_TIMER_MAX			15

#define SENSOR_COUNT_MAX			2

#define POWER_ON					1
#define POWER_OFF					0

typedef struct{
	unsigned timer : 15;
	unsigned action : 1;
	unsigned repeat : 7;
	unsigned enable : 1;
	unsigned flag : 8;
}socket_timer_t;

typedef struct{
	uint32_t count;
	socket_timer_t timers[SOCKET_TIMER_MAX];
}socket_timer_list_t;

typedef struct{
	uint8_t saved_flag;
	uint16_t zone;

	socket_timer_list_t socket_timer;
}socket_config_t;

typedef struct{
	xlink_device_t xlink_device;

	user_sensor_t sensor[SENSOR_COUNT_MAX];

	uint8_t pin;
	bool power;
	socket_timer_list_t * const p_timer;
}user_single_socket_t;

extern user_single_socket_t user_single_socket;

extern void user_single_socket_decode_sensor( uint8_t *pbuf, uint8_t len );

#endif /* USER_SINGLE_SOCKET_H_ */

