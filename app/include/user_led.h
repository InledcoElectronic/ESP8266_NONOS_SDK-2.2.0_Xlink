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
#define LEDR_FUNC				FUNC_GPIO12
#define LEDR_NUM				12
//#define LEDG_FUNC				FUNC_GPIO16
//#define LEDG_NUM				16
#define LEDB_FUNC				FUNC_GPIO1
#define LEDB_NUM				1
#define	USER_KEY_TOUCH_IO_MUX	PERIPHS_IO_MUX_MTMS_U
#define	USER_KEY_TOUCH_IO_NUM	14
#define	USER_KEY_TOUCH_FUNC		FUNC_GPIO14

#define CUSTOM_COUNT			4
#if	NIGHT_CHANNEL >= LED_CHANNEL_COUNT
#error "NIGHT_CHANNEL must be smaller than CHANNEL_COUNT."
#endif

#define POINT_COUNT_MIN			4
#define POINT_COUNT_MAX			10

#define	MODE_MANUAL				0
#define MODE_AUTO				1
#define	MODE_PRO				2

/* pwm 0 */
#define PWM_0_OUT_IO_MUX		PERIPHS_IO_MUX_GPIO4_U
#define PWM_0_OUT_IO_FUNC		FUNC_GPIO4
#define PWM_0_OUT_IO_NUM		4
/* pwm 1 */
#define PWM_1_OUT_IO_MUX		PERIPHS_IO_MUX_GPIO2_U
#define PWM_1_OUT_IO_FUNC		FUNC_GPIO2
#define PWM_1_OUT_IO_NUM		2
/* pwm 2 */
#define PWM_2_OUT_IO_MUX		PERIPHS_IO_MUX_MTDO_U
#define PWM_2_OUT_IO_FUNC		FUNC_GPIO15
#define PWM_2_OUT_IO_NUM		15
/* pwm 3 */
#define PWM_3_OUT_IO_MUX		PERIPHS_IO_MUX_MTCK_U
#define PWM_3_OUT_IO_FUNC		FUNC_GPIO13
#define PWM_3_OUT_IO_NUM		13
/* pwm 4 */
#define PWM_4_OUT_IO_MUX		PERIPHS_IO_MUX_GPIO5_U
#define PWM_4_OUT_IO_FUNC		FUNC_GPIO5
#define PWM_4_OUT_IO_NUM		5

#define PWM_5_OUT_IO_MUX		PERIPHS_IO_MUX_MTMS_U
#define PWM_5_OUT_IO_FUNC		FUNC_GPIO14
#define PWM_5_OUT_IO_NUM		14

typedef struct {
	/* Off All Blue WiFi */
	unsigned saved_flag : 8;
	unsigned last_mode : 2;
	unsigned state : 2;
	unsigned all_bright : 10;
	unsigned blue_bright : 10;

	/* WiFi Para */
	uint16_t zone;											//时区
	uint8_t mode;											//模式 Manual / Auto / Pro

	/* Manual Mode */
	uint8_t power;											//手动模式 开/关
	uint16_t bright[LED_CHANNEL_COUNT];							//通道亮度
	uint8_t custom_bright[CUSTOM_COUNT][LED_CHANNEL_COUNT];		//自定义亮度

	/* Auto Mode */
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
