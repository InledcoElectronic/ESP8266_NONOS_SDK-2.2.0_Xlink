/*
 * user_led.h
 *
 *  Created on: 2018年4月2日
 *      Author: liruya
 */

#ifndef USER_LED_H_
#define USER_LED_H_

#include "app_config.h"
#include "osapi.h"
#include "pwm.h"
#include "xlink.h"

#define LED_CHANNEL_COUNT		5
#define CHN1_NAME				"WarmWhite"
#define CHN2_NAME				"Purple"
#define CHN3_NAME				"Blue"
#define CHN4_NAME				"Red"
#define CHN5_NAME				"ColdWhite"
#define NIGHT_CHANNEL			2

#define CUSTOM_COUNT			4
#if	NIGHT_CHANNEL >= LED_CHANNEL_COUNT
#error "NIGHT_CHANNEL must be smaller than CHANNEL_COUNT."
#endif

#define POINT_COUNT_MIN			4
#define POINT_COUNT_MAX			10

#define	MODE_MANUAL				0
#define MODE_AUTO				1
#define	MODE_PRO				2

typedef union{
	struct{
		uint16_t sunrise_start;
		uint8_t sunrise_ramp;
		uint8_t day_bright[LED_CHANNEL_COUNT];
		uint16_t sunset_start;
		uint8_t sunset_ramp;
		uint8_t night_bright[LED_CHANNEL_COUNT];
		bool turnoff_enabled;
		uint16_t turnoff_time;
	};
	uint8_t array[2*LED_CHANNEL_COUNT+9];
}auto_para_t;

typedef struct {
	/* Off All Blue WiFi */
	unsigned saved_flag : 8;
	unsigned last_mode : 2;
	unsigned state : 2;
	unsigned all_bright : 10;
	unsigned blue_bright : 10;

	/* WiFi Para */
	uint8_t location_flag;
	float longitude;										//经度
	float latitude;											//纬度
	uint16_t zone;											//时区
	uint8_t mode;											//模式 Manual / Auto / Pro

	/* Manual Mode */
	uint8_t power;											//手动模式 开/关
	uint16_t bright[LED_CHANNEL_COUNT];							//通道亮度
	uint8_t custom_bright[CUSTOM_COUNT][LED_CHANNEL_COUNT];		//自定义亮度
	uint32_t manual_reserved;

	/* Auto Mode */
	// bool sunrs_from_location;								//按照设置日出日落时间 或者 按照地理位置计算日出日落时间
	// bool moonrs_from_location;								//按照设置关断灯光 或者按照地理位置计算月落时间关断灯光
	// auto_para_t month_auto_para[12];
	// uint8_t auto_reserved[32];
	uint16_t sunrise_start;									//日出开始时间
	uint16_t sunrise_end;									//日出结束时间
	uint8_t day_bright[LED_CHANNEL_COUNT];						//白天亮度
	uint16_t sunset_start;									//日落开始时间
	uint16_t sunset_end;									//日落结束时间
	uint8_t night_bright[LED_CHANNEL_COUNT];					//夜晚亮度
	bool turnoff_enabled;									//晚上关灯使能
	uint16_t turnoff_time;									//晚上关灯时间

	/* Pro Mode */
	uint8_t point_count;									//时间点个数
	uint16_t point_timer[POINT_COUNT_MAX];					//时间点
	uint8_t point_bright[POINT_COUNT_MAX][LED_CHANNEL_COUNT];	//时间点亮度
} led_para_t;

typedef struct{
	xlink_device_t xlink_device;
	const uint8_t channel_count;
}user_led_t;

extern user_led_t user_led;

#endif /* USER_LED_H_ */
