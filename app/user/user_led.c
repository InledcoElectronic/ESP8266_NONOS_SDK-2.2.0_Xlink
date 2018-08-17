/*
 * user_led.c
 *
 *  Created on: 2018年4月2日
 *      Author: liruya
 */

#include "user_led.h"
#include "pwm.h"
#include "xlink_config.h"
#include "os_type.h"
#include "user_key.h"
#include "xlink_datapoint.h"
#include "driver/gpio16.h"
#include "xlink.h"
#include "app_config.h"

#define LED_FIRMWARE_VERSION	1
#define LED_PRODUCT_ID			"160fa8b5715b03e9160fa8b5715b4401"
#define LED_PRODUCT_KEY			"10bfe20dcb76aaee3f99668a068145a6"

#define PARA_SAVED_FLAG		0x55
#define PARA_DEFAULT_FLAG	0xFF

#define PWM_PERIOD			4500			//us duty max 100000
#define DUTY_GAIN			100
#define BRIGHT_MIN			0
#define BRIGHT_MAX			1000
#define BRIGHT_DELT			200
#define BRIGHT_MIN2			10
#define BRIGHT_DELT_TOUCH	10
#define BRIGHT_STEP_NORMAL	10

#define TIME_VALUE_MAX		1439

#define LED_STATE_OFF		0
#define LED_STATE_DAY		1
#define LED_STATE_NIGHT		2
#define LED_STATE_WIFI		3

#define FLASH_PERIOD		750				//ms

#define FRAME_HEADER		0x68
#define CMD_INC				0x08
#define CMD_DEC				0x09
#define CMD_PREV			0x0B
#define CMD_PREV_STOP		0x0C
#define CMD_SYNC_TIME		0x0E
#define CMD_FIND			0x0F

#define	USER_KEY_NUM					1
#define USER_KEY_LONG_TIME_NORMAL		40
#define USER_KEY_LONG_TIME_SMARTCONFIG	200

#define PREVIEW_INTERVAL	50

LOCAL key_para_t *pkeys[USER_KEY_NUM];
LOCAL key_list_t key_list;

LOCAL uint8_t cmd_buffer[DATAPOINT_BIN_MAX_LEN];
LOCAL uint32_t pwm_io_info[][3] = { {PWM_0_OUT_IO_MUX, PWM_0_OUT_IO_FUNC, PWM_0_OUT_IO_NUM},
									{PWM_1_OUT_IO_MUX, PWM_1_OUT_IO_FUNC, PWM_1_OUT_IO_NUM},
									{PWM_2_OUT_IO_MUX, PWM_2_OUT_IO_FUNC, PWM_2_OUT_IO_NUM},
									{PWM_3_OUT_IO_MUX, PWM_3_OUT_IO_FUNC, PWM_3_OUT_IO_NUM},
									{PWM_4_OUT_IO_MUX, PWM_4_OUT_IO_FUNC, PWM_4_OUT_IO_NUM}};
LOCAL const uint8_t *chn_names[LED_CHANNEL_COUNT] = { CHN1_NAME, CHN2_NAME, CHN3_NAME, CHN4_NAME, CHN5_NAME };
LOCAL uint32_t current_bright[LED_CHANNEL_COUNT] = {0};
LOCAL uint32_t target_bright[LED_CHANNEL_COUNT] = {0};
LOCAL bool day_rise;
LOCAL bool night_rise;
LOCAL bool find_flag;
LOCAL bool prev_flag_shadow;
LOCAL bool prev_flag;
LOCAL uint16_t prev_ct;
LOCAL os_timer_t find_timer;
LOCAL os_timer_t prev_timer;

LOCAL led_para_t led_para;
LOCAL os_timer_t ramp_timer;
LOCAL os_timer_t auto_pro_timer;

LOCAL void user_led_init();
LOCAL void user_led_ramp();
LOCAL void user_led_onShortPress();
LOCAL void user_led_onLongPress();
LOCAL void user_led_onContPress();
LOCAL void user_led_onRelease();
LOCAL void user_led_auto_pro_run( void *arg );
LOCAL void user_led_auto_pro_prev( void *arg );
LOCAL void user_led_turnoff_ramp();
LOCAL void user_led_turnon_ramp();
LOCAL void user_led_turnoff_direct();
LOCAL void user_led_turnmax_direct();
LOCAL void user_led_update_day_status();
LOCAL void user_led_update_night_status();
LOCAL void user_led_update_day_bright();
LOCAL void user_led_update_night_bright();
LOCAL void user_led_indicate_off();
LOCAL void user_led_indicate_day();
LOCAL void user_led_indicate_night();
LOCAL void user_led_indicate_wifi();
LOCAL void user_led_datapoint_init();
LOCAL void user_led_datapoint_changed_cb();

user_led_t user_led = {
		.xlink_device = newXlinkDevice( LED_PRODUCT_ID,
										LED_PRODUCT_KEY,
										LED_FIRMWARE_VERSION,
										user_led_init,
										user_led_datapoint_changed_cb),
		.channel_count = LED_CHANNEL_COUNT};

LOCAL void ICACHE_FLASH_ATTR user_led_load_duty( uint32_t value, uint8_t chn )
{
	pwm_set_duty( value * DUTY_GAIN, chn );
}

void ICACHE_FLASH_ATTR user_led_default_para()
{
	uint8_t i, j;
	led_para.last_mode = MODE_AUTO;
	led_para.state = LED_STATE_OFF;
	led_para.all_bright = BRIGHT_MAX;
	led_para.blue_bright = BRIGHT_MAX;
	led_para.zone = 800;
	led_para.mode = MODE_MANUAL;
	led_para.power = 0;
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		led_para.bright[i] = BRIGHT_MAX;
		led_para.day_bright[i] = 100;
		led_para.night_bright[i] = 0;
		for ( j = 0; j < CUSTOM_COUNT; j++ )
		{
			led_para.custom_bright[j][i] = 100;
		}
	}
	led_para.night_bright[NIGHT_CHANNEL] = 5;
	led_para.sunrise_start = 480;					//08:00
	led_para.sunrise_end = 540;						//09:00
	led_para.sunset_start = 1080;					//18:00
	led_para.sunset_end = 1140;						//19:00
	led_para.turnoff_enabled = true;
	led_para.turnoff_time = 1320;					//22:00
	led_para.point_count = 6;
	for ( i = 0; i < POINT_COUNT_MAX; i++ )
	{
		led_para.point_timer[i] = i * 120 + 120;
		for ( j = 0; j < LED_CHANNEL_COUNT; j++ )
		{
			led_para.point_bright[i][j] = 100;
		}
	}
}

void ICACHE_FLASH_ATTR user_led_para_init()
{
	uint8_t i, j;
	xlink_read_user_para( (uint8_t *)&led_para, sizeof( led_para ) );
	if ( led_para.saved_flag != PARA_SAVED_FLAG )
	{
		user_led_default_para();
	}
	if ( led_para.mode == MODE_AUTO || led_para.mode == MODE_PRO )
	{
		led_para.last_mode = led_para.mode;
	}
	else
	{
		led_para.mode = MODE_MANUAL;
		if ( led_para.last_mode == 0 || led_para.last_mode == 3 )
		{
			led_para.last_mode = MODE_AUTO;
		}
	}

	if ( led_para.power > 0 )
	{
		led_para.power = 1;
	}
	if ( led_para.turnoff_enabled > 0 )
	{
		led_para.turnoff_enabled = 1;
	}
	if ( led_para.all_bright > BRIGHT_MAX )
	{
		led_para.all_bright = BRIGHT_MAX;
	}
	if ( led_para.blue_bright > BRIGHT_MAX )
	{
		led_para.blue_bright = BRIGHT_MAX;
	}
#if	BRIGHT_MIN > 0
	if ( led_para.all_bright < BRIGHT_MIN )
	{
		led_para.all_bright = BRIGHT_MIN;
	}
	if ( led_para.blue_bright < BRIGHT_MIN )
	{
		led_para.blue_bright = BRIGHT_MIN;
	}
#endif
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		if ( led_para.bright[i] > BRIGHT_MAX )
		{
			led_para.bright[i] = BRIGHT_MAX;
		}
		if ( led_para.day_bright[i] > 100 )
		{
			led_para.day_bright[i] = 100;
		}
		if ( led_para.night_bright[i] > 100 )
		{
			led_para.night_bright[i] = 100;
		}
#if	BRIGHT_MIN > 0
		if ( led_para.bright[i] < BRIGHT_MIN )
		{
			led_para.bright[i] = BRIGHT_MIN;
		}
#endif
		for ( j = 0; j < CUSTOM_COUNT; j++ )
		{
			if ( led_para.custom_bright[j][i] > 100 )
			{
				led_para.custom_bright[j][i] = 100;
			}
		}
	}
	if ( led_para.sunrise_start > TIME_VALUE_MAX )
	{
		led_para.sunrise_start = 0;
	}
	if ( led_para.sunrise_end > TIME_VALUE_MAX )
	{
		led_para.sunrise_end = 0;
	}
	if ( led_para.sunset_start > TIME_VALUE_MAX )
	{
		led_para.sunset_start = 0;
	}
	if ( led_para.sunset_end > TIME_VALUE_MAX )
	{
		led_para.sunset_end = 0;
	}
	if ( led_para.turnoff_time > TIME_VALUE_MAX )
	{
		led_para.turnoff_time = 0;
	}

	if ( led_para.point_count < POINT_COUNT_MIN )
	{
		led_para.point_count = POINT_COUNT_MIN;
	}
	else if ( led_para.point_count > POINT_COUNT_MAX )
	{
		led_para.point_count = POINT_COUNT_MAX;
	}
	for ( i = 0; i < POINT_COUNT_MAX; i++ )
	{
		if ( led_para.point_timer[i] > TIME_VALUE_MAX )
		{
			led_para.point_timer[i] = 0;
		}
		for ( j = 0; j < LED_CHANNEL_COUNT; j++ )
		{
			if ( led_para.point_bright[i][j] > 100 )
			{
				led_para.point_bright[i][j] = 100;
			}
		}
	}

	switch ( led_para.state )
	{
		case LED_STATE_OFF:
			user_led_indicate_off();
			user_led_turnoff_direct();
			break;
		case LED_STATE_DAY:
			user_led_indicate_day();
			user_led_update_day_bright();
			user_led_update_day_status();
			break;
		case LED_STATE_NIGHT:
			user_led_indicate_night();
			user_led_update_night_bright();
			user_led_update_night_status();
			break;
		case LED_STATE_WIFI:
			user_led_indicate_wifi();
			if ( led_para.mode == MODE_MANUAL )
			{
				if ( led_para.power )
				{
					user_led_turnon_ramp();
				}
				else
				{
					user_led_turnoff_ramp();
				}
			}
			break;
	}
}

void ICACHE_FLASH_ATTR user_led_init_key()
{
	pkeys[0] = user_key_init_single( USER_KEY_TOUCH_IO_NUM,
							  	USER_KEY_TOUCH_FUNC,
								USER_KEY_TOUCH_IO_MUX,
								user_led_onShortPress,
								user_led_onLongPress,
								user_led_onContPress,
								user_led_onRelease );
	if ( led_para.state == LED_STATE_WIFI )
	{
		pkeys[0]->long_count = USER_KEY_LONG_TIME_SMARTCONFIG;
	}
	else
	{
		pkeys[0]->long_count = USER_KEY_LONG_TIME_NORMAL;
	}
	key_list.key_num = USER_KEY_NUM;
	key_list.pkeys = pkeys;
	user_key_init_list( &key_list );
}

LOCAL void ICACHE_FLASH_ATTR user_led_init()
{
	pwm_init( PWM_PERIOD, current_bright, LED_CHANNEL_COUNT, pwm_io_info );
	pwm_start();
	user_led_para_init();
	user_led_datapoint_init();
//	xlink_setOnDatapointChangedCallback( user_led_datapoint_changed_cb );
	user_led_init_key();
	os_timer_disarm( &ramp_timer );
	os_timer_setfn( &ramp_timer, user_led_ramp, NULL );
	os_timer_arm( &ramp_timer, 10, 1 );
	os_timer_disarm( &auto_pro_timer );
	os_timer_setfn( &auto_pro_timer, user_led_auto_pro_run, NULL );
	os_timer_arm( &auto_pro_timer, 1000, 1 );
}

LOCAL void ICACHE_FLASH_ATTR user_led_save_para()
{
	led_para.saved_flag = PARA_SAVED_FLAG;
	xlink_write_user_para( (uint8_t *) &led_para, sizeof( led_para ) );
}

LOCAL void ICACHE_FLASH_ATTR user_led_turnoff_ramp()
{
	uint8_t i;
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		target_bright[i] = 0;
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_turnon_ramp()
{
	uint8_t i;
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		target_bright[i] = led_para.bright[i];
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_turnoff_direct()
{
	uint8_t i;
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		current_bright[i] = 0;
		target_bright[i] = 0;
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_turnmax_direct()
{
	uint8_t i;
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		current_bright[i] = BRIGHT_MAX;
		target_bright[i] = BRIGHT_MAX;
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_update_day_status()
{
	if ( led_para.all_bright > BRIGHT_MAX - BRIGHT_DELT )
	{
		day_rise = false;
	}
	else if ( led_para.all_bright < BRIGHT_MIN + BRIGHT_DELT )
	{
		day_rise = true;
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_update_night_status()
{
	if ( led_para.blue_bright > BRIGHT_MAX - BRIGHT_DELT )
	{
		night_rise = false;
	}
	else if ( led_para.blue_bright < BRIGHT_MIN + BRIGHT_DELT )
	{
		night_rise = true;
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_update_day_bright()
{
	uint8_t i;
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		current_bright[i] = led_para.all_bright;
		target_bright[i] = led_para.all_bright;
		led_para.bright[i] = led_para.all_bright;
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_update_night_bright()
{
	uint8_t i;
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		if ( i == NIGHT_CHANNEL )
		{
			current_bright[i] = led_para.blue_bright;
			target_bright[i] = led_para.blue_bright;
			led_para.bright[i] = led_para.blue_bright;
		}
		else
		{
			current_bright[i] = 0;
			target_bright[i] = 0;
			led_para.bright[i]  = 0;
		}
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_indicate_off()
{
	GPIO_OUTPUT_SET( LEDR_NUM, 0 );
#ifdef DEMO_BOARD
	GPIO_OUTPUT_SET( LEDG_NUM, 1 );
#else
	gpio16_output_set( 1 );
#endif
#ifndef	USE_TX_DEBUG
	GPIO_OUTPUT_SET( LEDB_NUM, 1 );
#endif
}

LOCAL void ICACHE_FLASH_ATTR user_led_indicate_day()
{
	GPIO_OUTPUT_SET( LEDR_NUM, 0 );
#ifdef DEMO_BOARD
	GPIO_OUTPUT_SET( LEDG_NUM, 0 );
#else
	gpio16_output_set( 0 );
#endif
#ifndef	USE_TX_DEBUG
	GPIO_OUTPUT_SET( LEDB_NUM, 0 );
#endif
}

LOCAL void ICACHE_FLASH_ATTR user_led_indicate_night()
{
	GPIO_OUTPUT_SET( LEDR_NUM, 1 );
#ifdef DEMO_BOARD
	GPIO_OUTPUT_SET( LEDG_NUM, 1 );
#else
	gpio16_output_set( 1 );
#endif
#ifndef	USE_TX_DEBUG
	GPIO_OUTPUT_SET( LEDB_NUM, 0 );
#endif
}

LOCAL void ICACHE_FLASH_ATTR user_led_indicate_wifi()
{
	GPIO_OUTPUT_SET( LEDR_NUM, 1 );
#ifdef DEMO_BOARD
	GPIO_OUTPUT_SET( LEDG_NUM, 0 );
#else
	gpio16_output_set( 0 );
#endif
#ifndef	USE_TX_DEBUG
	GPIO_OUTPUT_SET( LEDB_NUM, 1 );
#endif
}

LOCAL void ICACHE_FLASH_ATTR user_led_off_onShortPress()
{
	led_para.state++;
	led_para.power = 1;
	user_led_indicate_day();
	user_led_update_day_bright();
	user_led_update_day_status();
	user_led_save_para();
	xlink_datapoint_update_all();
}

LOCAL void ICACHE_FLASH_ATTR user_led_off_onLongPress()
{
	xlink_disconnect_cloud();
	xlink_reset();
//	system_restart();
//	led_para.state++;
//	led_para.power = 1;
//	led_para.all_bright = BRIGHT_MIN2;
//	day_rise = true;
//	user_led_indicate_day();
//	user_led_update_day_bright();
}

LOCAL void ICACHE_FLASH_ATTR user_led_off_onContPress()
{

}

LOCAL void ICACHE_FLASH_ATTR user_led_off_onRelease()
{

}

LOCAL void ICACHE_FLASH_ATTR user_led_day_onShortPress()
{
	led_para.state++;
	led_para.power = 1;
	user_led_indicate_night();
	user_led_update_night_bright();
	user_led_update_night_status();
	user_led_save_para();
	xlink_datapoint_update_all();
}

LOCAL void ICACHE_FLASH_ATTR user_led_day_onLongPress()
{

}

LOCAL void ICACHE_FLASH_ATTR user_led_day_onContPress()
{
	if ( day_rise )
	{
		if ( led_para.all_bright + BRIGHT_DELT_TOUCH <= BRIGHT_MAX )
		{
			led_para.all_bright += BRIGHT_DELT_TOUCH;
		}
		else
		{
			led_para.all_bright = BRIGHT_MAX;
		}
	}
	else
	{
		if ( led_para.all_bright >= BRIGHT_MIN2 + BRIGHT_DELT_TOUCH )
		{
			led_para.all_bright -= BRIGHT_DELT_TOUCH;
		}
		else
		{
			led_para.all_bright = BRIGHT_MIN2;
		}
	}
	user_led_update_day_bright();
}

LOCAL void ICACHE_FLASH_ATTR user_led_day_onRelease()
{
	user_led_update_day_status();
	user_led_save_para();
	xlink_datapoint_update_all();
}

LOCAL void ICACHE_FLASH_ATTR user_led_night_onShortPress()
{
	led_para.state++;
	if ( led_para.last_mode == MODE_AUTO || led_para.last_mode == MODE_PRO )
	{
		led_para.mode = led_para.last_mode;
	}
	else
	{
		led_para.mode = MODE_AUTO;
	}
	pkeys[0]->long_count = USER_KEY_LONG_TIME_SMARTCONFIG;
	user_led_indicate_wifi();
	user_led_save_para();
	xlink_datapoint_update_all();
}

LOCAL void ICACHE_FLASH_ATTR user_led_night_onLongPress()
{

}

LOCAL void ICACHE_FLASH_ATTR user_led_night_onContPress()
{
	if ( night_rise )
	{
		if ( led_para.blue_bright + BRIGHT_DELT_TOUCH <= BRIGHT_MAX )
		{
			led_para.blue_bright += BRIGHT_DELT_TOUCH;
		}
		else
		{
			led_para.blue_bright = BRIGHT_MAX;
		}
	}
	else
	{
		if ( led_para.blue_bright >= BRIGHT_MIN2 + BRIGHT_DELT_TOUCH )
		{
			led_para.blue_bright -= BRIGHT_DELT_TOUCH;
		}
		else
		{
			led_para.blue_bright = BRIGHT_MIN2;
		}
	}
	user_led_update_night_bright();
}

LOCAL void ICACHE_FLASH_ATTR user_led_night_onRelease()
{
	user_led_update_night_status();
	user_led_save_para();
	xlink_datapoint_update_all();
}

LOCAL void ICACHE_FLASH_ATTR user_led_wifi_onShortPress()
{
	if ( user_smartconfig_status() )
	{
		return;
	}
	led_para.state++;
	led_para.power = 0;
	pkeys[0]->long_count = USER_KEY_LONG_TIME_NORMAL;
	user_led_indicate_off();
	user_led_turnoff_direct();
	user_led_save_para();
	xlink_datapoint_update_all();
}

LOCAL void ICACHE_FLASH_ATTR user_led_wifi_onLongPress()
{
	if ( user_smartconfig_status() )
	{
		return;
	}
	user_tcp_disconnect();
	user_smartconfig_start();
	app_printf( "key pressed, smartconfig start..." );
}

LOCAL void ICACHE_FLASH_ATTR user_led_wifi_onContPress()
{

}

LOCAL void ICACHE_FLASH_ATTR user_led_wifi_onRelease()
{

}

LOCAL void ICACHE_FLASH_ATTR user_led_onShortPress()
{
	LOCAL key_function_t short_press_func[] = { user_led_off_onShortPress,
												user_led_day_onShortPress,
												user_led_night_onShortPress,
												user_led_wifi_onShortPress };
	short_press_func[led_para.state]();
}

LOCAL void ICACHE_FLASH_ATTR user_led_onLongPress()
{
	LOCAL key_function_t long_press_func[] = { 	user_led_off_onLongPress,
												user_led_day_onLongPress,
												user_led_night_onLongPress,
												user_led_wifi_onLongPress };
	long_press_func[led_para.state]();
}

LOCAL void ICACHE_FLASH_ATTR user_led_onContPress()
{
	LOCAL key_function_t cont_press_func[] = { 	user_led_off_onContPress,
												user_led_day_onContPress,
												user_led_night_onContPress,
												user_led_wifi_onContPress };
	cont_press_func[led_para.state]();
}

LOCAL void ICACHE_FLASH_ATTR user_led_onRelease()
{
	LOCAL key_function_t release_func[] = { user_led_off_onRelease,
											user_led_day_onRelease,
											user_led_night_onRelease,
											user_led_wifi_onRelease };
	release_func[led_para.state]();
}

LOCAL void ICACHE_FLASH_ATTR user_led_ramp( void *arg )
{
	uint8_t i;
//	if ( find_flag || prev_flag || led_para.mode || led_para.state != LED_STATE_WIFI )
//	{
//		return;
//	}
	if ( find_flag )
	{
		return;
	}
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		if ( current_bright[i] + BRIGHT_STEP_NORMAL < target_bright[i] )
		{
			current_bright[i] += BRIGHT_STEP_NORMAL;
		}
		else if ( current_bright[i] > target_bright[i] + BRIGHT_STEP_NORMAL )
		{
			current_bright[i] -= BRIGHT_STEP_NORMAL;
		}
		else
		{
			current_bright[i] = target_bright[i];
		}
		user_led_load_duty( current_bright[i], i );
	}
	pwm_start();
}

LOCAL void ICACHE_FLASH_ATTR user_led_flash_cb( void *arg )
{
	LOCAL uint8_t count = 0;
	count++;
	if ( count&0x01 )
	{
		user_led_turnmax_direct();
	}
	else
	{
		user_led_turnoff_direct();
	}
	if ( count >= 6 )
	{
		count = 0;
		find_flag = false;
		os_timer_disarm( &find_timer );
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_flash()
{
	if ( find_flag )
	{
		return;
	}
	find_flag = true;
	os_timer_disarm( &find_timer );
	os_timer_setfn( &find_timer, user_led_flash_cb, NULL );
	os_timer_arm( &find_timer, FLASH_PERIOD, 1 );
}

LOCAL void ICACHE_FLASH_ATTR user_led_prev_stop()
{
	prev_ct = 0;
	prev_flag = false;
	os_timer_disarm( &prev_timer );
}

LOCAL void ICACHE_FLASH_ATTR user_led_prev_start()
{
	prev_ct = 0;
	prev_flag = true;
	os_timer_disarm( &prev_timer );
	os_timer_setfn( &prev_timer, user_led_auto_pro_prev, NULL );
	os_timer_arm( &prev_timer, PREVIEW_INTERVAL, 1 );
}

LOCAL inline ICACHE_FLASH_ATTR user_led_auto_proccess( uint16_t ct, uint8_t sec )
{
	if ( ct > 1439 || sec > 59 )
	{
		return;
	}
	if ( led_para.mode != MODE_AUTO || find_flag || led_para.state != LED_STATE_WIFI )
	{
		return;
	}
	uint8_t i, j, k;
	uint8_t count = 4;
	uint16_t st, et, duration;
	uint32_t dt;
	uint8_t dbrt;
	uint16_t tm[6] = { led_para.sunrise_start,
						led_para.sunrise_end,
						led_para.sunset_start,
						led_para.sunset_end,
						led_para.turnoff_time,
						led_para.turnoff_time};
	uint8_t brt[6][LED_CHANNEL_COUNT];
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		if ( led_para.turnoff_enabled )
		{
			brt[0][i] = 0;
		}
		else
		{
			brt[0][i] = led_para.night_bright[i];
		}
		brt[1][i] = led_para.day_bright[i];
		brt[2][i] = led_para.day_bright[i];
		brt[3][i] = led_para.night_bright[i];
		brt[4][i] = led_para.night_bright[i];
		brt[5][i] = 0;
	}
	if ( led_para.turnoff_enabled )
	{
		count = 6;
	}
	for ( i = 0; i < count; i++ )
	{
		j = ( i + 1 ) % count;
		st = tm[i];
		et = tm[j];
		if ( et >= st )
		{
			if ( ct >= st && ct < et )
			{
				duration = et - st;
				dt = ( ct - st ) * 60u + sec;
			}
			else
			{
				continue;
			}
		}
		else
		{
			if ( ct >= st || ct < et )
			{
				duration = 1440u - st + et;
				if ( ct >= st )
				{
					dt = ( ct - st ) * 60u + sec;
				}
				else
				{
					dt = ( 1440u - st + ct ) * 60u + sec;
				}
			}
			else
			{
				continue;
			}
		}
		for ( k = 0; k < LED_CHANNEL_COUNT; k++ )
		{
			if ( brt[j][k] >= brt[i][k] )
			{
				dbrt = brt[j][k] - brt[i][k];
				target_bright[k] = brt[i][k] * 10u + dbrt * dt / ( duration * 6u );
			}
			else
			{
				dbrt = brt[i][k] - brt[j][k];
				target_bright[k] = brt[i][k] * 10u - dbrt * dt / ( duration * 6u );
			}
		}
	}
}

LOCAL inline void ICACHE_FLASH_ATTR user_led_pro_process( uint16_t ct, uint8_t sec )
{
	if ( ct > 1439 || sec > 59 )
	{
		return;
	}
	if ( led_para.mode != MODE_PRO || find_flag || led_para.state != LED_STATE_WIFI )
	{
		return;
	}
	if ( led_para.point_count < POINT_COUNT_MIN )
	{
		led_para.point_count = POINT_COUNT_MIN;
	}
	else if ( led_para.point_count > POINT_COUNT_MAX )
	{
		led_para.point_count = POINT_COUNT_MAX;
	}
	int i, j;
	int index[POINT_COUNT_MAX];
	for ( i = 0; i < POINT_COUNT_MAX; i++ )
	{
		index[i] = i;
	}
	for ( i = led_para.point_count - 1;  i > 0; i-- )
	{
		for ( j = 0; j < i; j++ )
		{
			if ( led_para.point_timer[index[j]] > led_para.point_timer[index[j+1]] )
			{
				int tmp = index[j];
				index[j] = index[j+1];
				index[j+1] = tmp;
			}
		}
	}
	int duration;
	int dt;
	int start, end;
	bool flag = false;
	int ts = led_para.point_timer[index[0]];
	int te = led_para.point_timer[index[led_para.point_count-1]];
	if ( ct >= te && ct <= 1439 )
	{
		duration = 1440 - te + ts;
		dt = (ct - te)*60u + sec;
		start = index[led_para.point_count - 1];
		end = index[0];
		flag = true;
	}
	else if ( ct >= 0 && ct < ts )
	{
		duration = 1440 - te + ts;
		dt = (1440 - te + ct)*60u + sec;
		start = index[led_para.point_count - 1];
		end = index[0];
		flag = true;
	}
	else
	{
		for ( i = 0; i < led_para.point_count - 1; i++ )
		{
			start = index[i];
			end = index[i+1];
			if ( ct >= led_para.point_timer[start] && ct < led_para.point_timer[end] )
			{

				duration = led_para.point_timer[end] - led_para.point_timer[start];
				dt = (ct - led_para.point_timer[start])*60u + sec;
				flag = true;
				break;
			}
		}
	}
	if ( flag )
	{
		for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
		{
			int dbrt;
			if ( led_para.point_bright[start][i] < led_para.point_bright[end][i] )
			{
				dbrt = led_para.point_bright[end][i] - led_para.point_bright[start][i];
				target_bright[i] = led_para.point_bright[start][i] * 10u + dbrt * dt / (duration * 6u);
			}
			else
			{
				dbrt = led_para.point_bright[start][i] - led_para.point_bright[end][i];
				target_bright[i] = led_para.point_bright[start][i] * 10u - dbrt * dt / (duration * 6u);
			}
		}
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_auto_pro_run( void *arg )
{
	if ( prev_flag )
	{
		return;
	}
	uint16_t ct = user_rtc_get_hour() * 60u + user_rtc_get_minute();
	uint8_t sec = user_rtc_get_second();
	if ( led_para.mode == MODE_AUTO )
	{
		user_led_auto_proccess( ct, sec );
	}
	else if ( led_para.mode == MODE_PRO )
	{
		user_led_pro_process( ct, sec );
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_auto_pro_prev( void *arg )
{
	if ( !prev_flag )
	{
		user_led_prev_stop();
		return;
	}
	prev_ct++;
	if ( prev_ct > 1439 )
	{
		user_led_prev_stop();
		prev_flag_shadow = false;
		xlink_datapoint_update_all();
		return;
	}
	if ( led_para.mode == MODE_AUTO )
	{
		user_led_auto_proccess( prev_ct, 0 );
	}
	else if ( led_para.mode == MODE_PRO )
	{
		user_led_pro_process( prev_ct, 0 );
	}
}

LOCAL void ICACHE_FLASH_ATTR user_led_datapoint_init()
{
	uint8_t i;
	uint16_t idx = 0;
	p_datapoints[0] = xlink_datapoint_init_byte( (uint8_t *) &user_led.channel_count );
	p_datapoints[1] = xlink_datapoint_init_uint16( (uint8_t *) &led_para.zone );
	p_datapoints[10] = xlink_datapoint_init_byte( &led_para.mode );
	p_datapoints[11] = xlink_datapoint_init_byte( (uint8_t *) &led_para.power );
	for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
	{
		p_datapoints[2+i] = xlink_datapoint_init_string( os_strlen( chn_names[i] ), (uint8_t *) chn_names[i] );
		p_datapoints[12+i] = xlink_datapoint_init_uint16( (uint8_t *) &led_para.bright[i] );
	}
	for ( i = 0; i < CUSTOM_COUNT; i++ )
	{
		p_datapoints[20+i] = xlink_datapoint_init_binary( LED_CHANNEL_COUNT, (uint8_t *) &led_para.custom_bright[i][0] );
	}

	p_datapoints[24] = xlink_datapoint_init_uint16( (uint8_t *) &led_para.sunrise_start );
	p_datapoints[25] = xlink_datapoint_init_uint16( (uint8_t *) &led_para.sunrise_end );
	p_datapoints[26] = xlink_datapoint_init_binary( LED_CHANNEL_COUNT, (uint8_t *) &led_para.day_bright[0] );
	p_datapoints[27] = xlink_datapoint_init_uint16( (uint8_t *) &led_para.sunset_start );
	p_datapoints[28] = xlink_datapoint_init_uint16( (uint8_t *) &led_para.sunset_end );
	p_datapoints[29] = xlink_datapoint_init_binary( LED_CHANNEL_COUNT, (uint8_t *) &led_para.night_bright[0] );
	p_datapoints[30] = xlink_datapoint_init_byte( (uint8_t *) &led_para.turnoff_enabled );
	p_datapoints[31] = xlink_datapoint_init_uint16( (uint8_t *) &led_para.turnoff_time );

	p_datapoints[32] = xlink_datapoint_init_byte( (uint8_t *) &led_para.point_count );
	for ( i = 0; i < POINT_COUNT_MAX; i++ )
	{
		p_datapoints[33+2*i] = xlink_datapoint_init_uint16( (uint8_t *) &led_para.point_timer[i] );
		p_datapoints[34+2*i] = xlink_datapoint_init_binary( LED_CHANNEL_COUNT, (uint8_t *) &led_para.point_bright[i][0] );
	}
	p_datapoints[111] = xlink_datapoint_init_byte( (uint8_t *) &prev_flag_shadow );
	p_datapoints[121] = xlink_datapoint_init_string( 0, (uint8_t *) user_led.xlink_device.upgrade_url );
	p_datapoints[127] = xlink_datapoint_init_binary( 0, (uint8_t *) &cmd_buffer[0] );
}

LOCAL void ICACHE_FLASH_ATTR user_led_decode_preview( char *pdata, uint16_t len )
{
	uint8_t i;
	if ( len == LED_CHANNEL_COUNT * 2 + 3 )
	{
		for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
		{
			uint16_t val = ( pdata[2+2*i] << 8 ) | pdata[3+2*i];
			if ( val <= BRIGHT_MAX )
			{
				current_bright[i] = val;
			}
			user_led_load_duty( current_bright[i], i );
		}
		pwm_start();
		user_led_prev_start();
	}
}

void ICACHE_FLASH_ATTR user_led_decode( void *arg, char *pdata, uint16_t len )
{
	if ( find_flag || pdata == NULL || len < 3 || pdata[0] != FRAME_HEADER )
	{
		return;
	}
	uint8_t i;
	uint8_t xor = 0;
	for ( i = 0; i < len; i++ )
	{
		xor ^= pdata[i];
	}
	if ( xor == 0 )
	{
		switch ( pdata[1] )
		{
			case CMD_PREV:
				user_led_decode_preview( pdata, len );
				break;

			case CMD_PREV_STOP:
				if ( len == 3 )
				{
					user_led_prev_stop( NULL );
				}
				break;

			default:
				break;
		}
	}
}

void ICACHE_FLASH_ATTR user_led_decode_command()
{
	uint8_t i;
	uint8_t xor = 0;
	datapoint_t *pdp = p_datapoints[199];
	if ( pdp != NULL && pdp->length > 0 && pdp->pdata[0] == FRAME_HEADER )
	{
		for ( i = 0; i < pdp->length; i++ )
		{
			xor ^= pdp->pdata[i];
		}
		if ( xor == 0 )
		{
			switch ( pdp->pdata[1] )
			{
				case CMD_INC:
					if ( pdp->length == 5 && !find_flag && !prev_flag && led_para.mode == MODE_MANUAL && led_para.power && pdp->pdata[2] < LED_CHANNEL_COUNT )
					{
						if ( led_para.bright[pdp->pdata[2]] + pdp->pdata[3] < BRIGHT_MAX )
						{
							led_para.bright[pdp->pdata[2]] += pdp->pdata[3];
						}
						else
						{
							led_para.bright[pdp->pdata[2]] = BRIGHT_MAX;
						}
					}
					break;

				case CMD_DEC:
					if ( pdp->length == 5 && !find_flag && !prev_flag && led_para.mode == MODE_MANUAL && led_para.power && pdp->pdata[2] < LED_CHANNEL_COUNT )
					{
						if ( led_para.bright[pdp->pdata[2]] > pdp->pdata[3] + BRIGHT_MIN )
						{
							led_para.bright[pdp->pdata[2]] -= pdp->pdata[3];
						}
						else
						{
							led_para.bright[pdp->pdata[2]] = BRIGHT_MIN;
						}
					}
					break;

				case CMD_PREV:
					if ( pdp->length == LED_CHANNEL_COUNT * 2 + 3 && !find_flag )
					{
						for ( i = 0; i < LED_CHANNEL_COUNT; i++ )
						{
							uint16_t val = ( pdp->pdata[2+2*i] << 8 ) | pdp->pdata[3+2*i];
							if ( val <= BRIGHT_MAX )
							{
								current_bright[i] = val;
							}
							user_led_load_duty( current_bright[i], i );
						}
						pwm_start();
						user_led_prev_start();
					}
					break;

				case CMD_PREV_STOP:
					if ( pdp->length == 3 )
					{
						user_led_prev_stop( NULL );
					}
					break;

				case CMD_SYNC_TIME:

					break;

				case CMD_FIND:
					if ( pdp->length == 3 )
					{
						user_led_flash();
					}
					break;

				default:
					break;
			}
		}
	}
}

void ICACHE_FLASH_ATTR user_led_on_update_start()
{
	char msg[] = "Device firmware update start";
	uint32_t len = os_strlen( msg );
	os_memset( user_led.xlink_device.upgrade_url, 0, XLINK_UPGRADE_URL_MAX_LENGTH );
	os_memcpy( user_led.xlink_device.upgrade_url, msg, len );
	p_datapoints[121]->length = len;
	xlink_datapoint_update_all();
}

void ICACHE_FLASH_ATTR user_led_on_update_failed()
{
	char msg[] = "Device firmware update failed";
	uint32_t len = os_strlen( msg );
	os_memset( user_led.xlink_device.upgrade_url, 0, XLINK_UPGRADE_URL_MAX_LENGTH );
	os_memcpy( user_led.xlink_device.upgrade_url, msg, len );
	p_datapoints[121]->length = len;
	xlink_datapoint_update_all();
}

void ICACHE_FLASH_ATTR user_led_on_update_success()
{
	char msg[] = "Device firmware update success";
	uint32_t len = os_strlen( msg );
	os_memset( user_led.xlink_device.upgrade_url, 0, XLINK_UPGRADE_URL_MAX_LENGTH );
	os_memcpy( user_led.xlink_device.upgrade_url, msg, len );
	p_datapoints[121]->length = len;
	xlink_datapoint_update_all();
}

void ICACHE_FLASH_ATTR user_led_datapoint_changed_cb()
{
	led_para.state = LED_STATE_WIFI;
	pkeys[0]->long_count = USER_KEY_LONG_TIME_SMARTCONFIG;
	user_led_indicate_wifi();
	uint32_t len = os_strlen( user_led.xlink_device.upgrade_url );
	char target_url[XLINK_UPGRADE_URL_MAX_LENGTH];
	if ( len > 0 )
	{
		os_memset( target_url, 0, XLINK_UPGRADE_URL_MAX_LENGTH );
		os_memcpy( target_url, user_led.xlink_device.upgrade_url, len );
		os_memset( user_led.xlink_device.upgrade_url, 0, XLINK_UPGRADE_URL_MAX_LENGTH );
		p_datapoints[121]->length = 0;
		xlink_upgrade_start( target_url );
		return;
	}
//	user_led_decode_command();
	if ( find_flag )
	{
		return;
	}
	if ( prev_flag_shadow && !prev_flag )
	{
		user_led_prev_start();
	}
	else if ( !prev_flag_shadow && prev_flag )
	{
		user_led_prev_stop();
	}
	if ( led_para.mode == MODE_AUTO || led_para.mode == MODE_PRO )
	{
		led_para.last_mode = led_para.mode;
	}
	else
	{
		led_para.mode = MODE_MANUAL;
		if ( led_para.power == 0 )
		{
			user_led_turnoff_ramp();
		}
		else
		{
			user_led_turnon_ramp();
		}
	}
	xlink_datapoint_update_all();
	user_led_save_para();
}
