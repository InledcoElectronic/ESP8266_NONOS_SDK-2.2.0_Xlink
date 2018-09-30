/*
 * user_sensor.h
 *
 *  Created on: 2018年6月15日
 *      Author: liruya
 */

#ifndef USER_SENSOR_H_
#define USER_SENSOR_H_

#include "osapi.h"

#define	LINKAGE_VERSION			1
#define SENSOR_ARGS_MAX			30

#define newLinkageArg()			{	.version=0,\
									.length=0,\
									.arg_array={0}	}

#define newSensor(typ)			{	.available=false,\
									.type=((uint8_t)typ),\
									.value=0,\
									.notify_enable=false,\
									.linkage_enable=false,\
									.linkage_arg=newLinkageArg()	}

typedef enum _sensor_type{
	SENSOR_UNKOWN,
	SENSOR_TEMPERATURE = 2,
	SENSOR_HUMIDITY,
	SENSOR_WATER_LEVEL,
	SENSOR_INTENSITY,
	SENSOR_THERMOSTAT,
	SENSOR_INVALID
}sensor_type_t;

typedef union{
	struct{
		int8_t threshold;
		uint16_t night_start;
		uint16_t night_end;
		int8_t night_threshold;
	};
	uint8_t array[6];
}thermostat_arg_t;

typedef struct{
	uint8_t version;
	uint8_t length;
	union{
		thermostat_arg_t thermostat_arg;
		uint8_t arg_array[SENSOR_ARGS_MAX];
	};
}sensor_arg_t;

typedef struct{
	bool available;
	uint8_t type;
	int16_t value;
	bool notify_enable;
	bool linkage_enable;
	sensor_arg_t linkage_arg;
}user_sensor_t;

#endif /* USER_SENSOR_H_ */
