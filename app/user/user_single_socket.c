/*
 * user_single_socket.c
 *
 *  Created on: 2018年6月8日
 *      Author: liruya
 */

#include "user_single_socket.h"
#include "user_key.h"

#define GPIO_OUTPUT_GET(gpio_no)		((GPIO_REG_READ(GPIO_OUT_ADDRESS)>>(gpio_no))&BIT0)

#define SOCKET_FIRMWARE_VERSION			1
#define SOCKET_PRODUCT_ID				"160fa2b60c5503e9160fa2b60c555e01"
#define	SOCKET_PRODUCT_KEY				"24149adeae917a014282b40d39228bed"

#define	USER_KEY_NUM					1
#define USER_KEY_LONG_TIME_NORMAL		40
#define USER_KEY_LONG_TIME_SMARTCONFIG	200

#define	PARA_SAVED_FLAG					0x55
#define	PARA_DEFAULT_FLAG				0xFF

#define FRAME_HEADER	0x68
#define CMD_SET			0x10
#define CMD_GET			0x11

LOCAL socket_config_t m_socket_config;

typedef enum timer_error{ TIMER_DISABLED, TIMER_ENABLED, TIMER_INVALID } timer_error_t;
typedef enum socket_state{ STATE_LOW, STATE_HIGH, STATE_INVALID } socket_state_t;

LOCAL void user_single_socket_init();
LOCAL void user_single_socket_process();
LOCAL void user_single_socket_datapoint_init();
LOCAL void user_single_socket_datapoint_changed_cb();
LOCAL void user_single_socket_update_datapoint();
LOCAL void user_single_socket_detect_sensor();
LOCAL bool user_single_socket_linkage_process();
LOCAL void thermostat_linkage_process( user_sensor_t *psensor );
LOCAL void user_single_socket_set_sensor();

LOCAL key_para_t *pkeys[USER_KEY_NUM];
LOCAL key_list_t key_list;
LOCAL bool m_sensor_detected;
LOCAL os_timer_t socket_proc_tmr;
LOCAL os_timer_t sensor_detect_tmr;
user_single_socket_t user_single_socket = {
	.xlink_device = newXlinkDevice( SOCKET_PRODUCT_ID,
									SOCKET_PRODUCT_KEY,
									SOCKET_FIRMWARE_VERSION,
									user_single_socket_init,
									user_single_socket_datapoint_changed_cb ),
	.sensor = { newSensor( SENSOR_UNKOWN ),
				newSensor( SENSOR_UNKOWN ) },
	.pin = RELAY1_IO_NUM,
	.power = false,
	.p_timer = &m_socket_config.socket_timer
};
user_sensor_t user_sensor[SENSOR_COUNT_MAX];

LOCAL timer_error_t ICACHE_FLASH_ATTR user_single_socket_check_timer( socket_timer_t *p_timer )
{
	if ( p_timer == NULL )
	{
		return TIMER_INVALID;
	}
	if ( p_timer->timer > 1439 )
	{
		return TIMER_INVALID;
	}
	if( (p_timer->action > 1 && p_timer->action < 5) || p_timer->action >= 60 )
	{
		return TIMER_INVALID;
	}
	if ( p_timer->enable )
	{
		return TIMER_ENABLED;
	}
	return TIMER_DISABLED;
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_update_timers()
{
	uint8_t i;
	if ( m_socket_config.socket_timer.count > SOCKET_TIMER_MAX )
	{
		m_socket_config.socket_timer.count = 0;
	}
	for ( i = 0; i < m_socket_config.socket_timer.count; i++ )
	{
		if ( user_single_socket_check_timer( &m_socket_config.socket_timer.timers[i] ) == TIMER_INVALID )
		{
			m_socket_config.socket_timer.count = i;
			break;
		}
	}
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_print()
{
	// uint8_t i, j, k;
	// uint8_t len = 0;
	// char str[256];
	// char *week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	// os_memset( str, '\0', 256 );
	// for ( i = 0; i < SENSOR_COUNT_MAX; i++ )
	// {
	// 	if ( user_single_socket.sensor[i].available )
	// 	{
	// 		len += os_sprintf( str+len, "sensor%d:%d    available:true    value:%d", i, user_single_socket.sensor[i].type, user_single_socket.sensor[i].value );
	// 	}
	// }
	// len += os_sprintf( str+len, "Socket Power:%s timers:%d\n", user_single_socket.power ? "On" : "Off", user_single_socket.p_timer->count );
	// for ( j = 0; j < user_single_socket.p_timer->count && j < SOCKET_TIMER_MAX; j++ )
	// {
	// 	socket_timer_t *ptmr = &user_single_socket.p_timer->timers[j];
	// 	if ( user_single_socket_check_timer( ptmr ) != TIMER_INVALID )
	// 	{
	// 		len += os_sprintf( str+len, "\t%s ", ptmr->enable ? "Enabled" : "Disabled" );
	// 		for ( k = 7; k > 0; k-- )
	// 		{
	// 			len += os_sprintf( str+len, " %s ", (ptmr->repeat & (1<<(k-1))) ? week[k-1] : "*" );
	// 		}
	// 		len += os_sprintf( str+len, " %d:%d ->%s\n", ptmr->timer/60, ptmr->timer%60, ptmr->action ? "Turnon" : "Turnoff" );
	// 	}
	// }
	// app_printf( "%s", str );
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_default_para()
{
	os_memset( (uint8_t *)&m_socket_config, 0, sizeof( m_socket_config ) );
	m_socket_config.saved_flag = PARA_DEFAULT_FLAG;
	m_socket_config.zone = 800;
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_save_para()
{
	m_socket_config.saved_flag = PARA_SAVED_FLAG;
	xlink_write_user_para( (uint8_t *)&m_socket_config, sizeof( m_socket_config ) );
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_para_init()
{
	xlink_read_user_para( (uint8_t *)&m_socket_config, sizeof( m_socket_config ) );
	if ( m_socket_config.saved_flag != PARA_SAVED_FLAG )
	{
		user_single_socket_default_para();
	}
	if ( m_socket_config.zone > 2400 )
	{
		m_socket_config.zone = 0;
	}
	user_single_socket_update_timers();
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_key_short_press_cb()
{
	uint8_t i;
	xlink_disconnect_cloud();
	xlink_reset();
	system_restart();
	/* when use linkage with sensor, manual control is diabled */
	// for ( i = 0; i < SENSOR_COUNT_MAX; i++ )
	// {
	// 	user_sensor_t *psensor = &user_single_socket.sensor[i];
	// 	if ( psensor->available && psensor->linkage_enable )
	// 	{
	// 		return;
	// 	}
	// }
	// user_single_socket.power = !user_single_socket.power;
	// GPIO_OUTPUT_SET( user_single_socket.pin, user_single_socket.power );
	// GPIO_OUTPUT_SET( LEDB_IO_NUM, user_single_socket.power ? 0 : 1 );
	// xlink_datapoint_update_all();
//	if ( user_smartconfig_status() )
//	{
//		return;
//	}
//	user_tcp_disconnect();
//	user_smartconfig_start();
//	app_printf( "key pressed, smartconfig start..." );
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_key_long_press_cb()
{
	if ( user_smartconfig_status() )
	{
		return;
	}
	user_tcp_disconnect();
	user_smartconfig_start();
	app_printf( "key pressed, smartconfig start..." );
//	xlink_disconnect_cloud();
//	xlink_reset();
//	user_single_socket_default_para();
//	user_single_socket_save_para();
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_key_cont_press_cb()
{

}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_key_release_cb()
{
//	system_restart();
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_key_init()
{
	pkeys[0] = user_key_init_single( BOOT_IO_NUM,
								  	BOOT_IO_FUNC,
									BOOT_IO_MUX,
									user_single_socket_key_short_press_cb,
									user_single_socket_key_long_press_cb,
									user_single_socket_key_cont_press_cb,
									user_single_socket_key_release_cb );
	pkeys[0]->long_count = 200;
	key_list.key_num = USER_KEY_NUM;
	key_list.pkeys = pkeys;
	user_key_init_list( &key_list );
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_init()
{
	user_single_socket_para_init();
	user_single_socket_datapoint_init();
	user_single_socket_key_init();
	os_timer_disarm( &socket_proc_tmr );
	os_timer_setfn( &socket_proc_tmr, user_single_socket_process, NULL );
	os_timer_arm( &socket_proc_tmr, 1000, 1 );
	os_timer_disarm( &sensor_detect_tmr );
	os_timer_setfn( &sensor_detect_tmr, user_single_socket_detect_sensor, NULL );
	os_timer_arm( &sensor_detect_tmr, 32, 1 );
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_process()
{
	/* when use linkage with sensor, timer disabled */
	if ( user_single_socket_linkage_process() )
	{
		return;
	}
	uint8_t i;
	bool flag = false;
	bool action = false;
	socket_timer_t *p = user_single_socket.p_timer->timers;
	uint8_t sec = user_rtc_get_second();
	uint16_t ct = user_rtc_get_hour() * 60u + user_rtc_get_minute();
	user_single_socket_update_timers();
	for ( i = 0; i < user_single_socket.p_timer->count; i++ )
	{
		if ( user_single_socket_check_timer( p ) == TIMER_ENABLED && ct == p->timer )
		{
			if(sec == 0)
			{
				if ( p->repeat == 0 )
				{
					if(p->action == 0 || p->action == 1)
					{
						p->enable = false;
					}
					action = p->action > 0 ? true : false;
					flag = true;
				}
				else if ( (p->repeat&(1<<user_rtc_get_week())) != 0 )
				{
					action = p->action > 0 ? true : false;
					flag = true;
				}
			}
			else
			{
				if(p->action >= 5 && p->action < 60 && sec == p->action)
				{
					if(p->repeat == 0)
					{
						p->enable = false;
						flag = true;
						action = false;
					}
					else if ( (p->repeat&(1<<user_rtc_get_week())) != 0 )
					{
						flag = true;
						action = false;
					}
				}
			}
		}
		p++;
	}
	if ( flag )
	{
		user_single_socket.power = action;
		GPIO_OUTPUT_SET( user_single_socket.pin, action );
		GPIO_OUTPUT_SET( LEDB_IO_NUM, action ? 0 : 1 );
		// user_single_socket_print();
		user_single_socket_update_datapoint();
		xlink_datapoint_update_all();
	}
}

LOCAL bool user_single_socket_linkage_process()
{
	uint8_t i;
	bool flag = false;
	for ( i = 0; i < SENSOR_COUNT_MAX; i++ )
	{
		user_sensor_t *psensor = &user_single_socket.sensor[i];
		if ( psensor->available && psensor->linkage_enable )
		{
			flag = true;
			switch( psensor->type )
			{
				case SENSOR_TEMPERATURE:
					thermostat_linkage_process( psensor );
					break;
			}
		}
	}
	return flag;
}

LOCAL void thermostat_linkage_process( user_sensor_t *psensor )
{
	uint8_t version = psensor->linkage_arg.version;
	thermostat_arg_t *pthermostat = &psensor->linkage_arg.thermostat_arg;
	int8_t thrd = pthermostat->threshold;
	if ( pthermostat->night_start > 1439 || pthermostat->night_end > 1439 )
	{
		return;
	}
	uint16_t ct = user_rtc_get_hour() * 60u + user_rtc_get_minute();
	if ( pthermostat->night_start > pthermostat->night_end )
	{
		if ( ct >= pthermostat->night_start || ct < pthermostat->night_end )
		{
			/* night */
			thrd = pthermostat->night_threshold;
		}
	}
	else if ( pthermostat->night_start < pthermostat->night_end )
	{
		if ( ct >= pthermostat->night_start && ct < pthermostat->night_end )
		{
			/* night */
			thrd = pthermostat->night_threshold;
		}
	}
	if ( psensor->value < thrd * 10 )
	{
		if ( GPIO_OUTPUT_GET(user_single_socket.pin) == false )
		{
			GPIO_OUTPUT_SET(user_single_socket.pin, 1);
			GPIO_OUTPUT_SET( LEDB_IO_NUM, 0 );
			user_single_socket.power = true;
			user_single_socket_update_datapoint();
			xlink_datapoint_update_all();
		}
	}
	else if ( psensor->value > thrd * 10 + 5 )
	{
		if ( GPIO_OUTPUT_GET(user_single_socket.pin) )
		{
			GPIO_OUTPUT_SET(user_single_socket.pin, 0);
			GPIO_OUTPUT_SET( LEDB_IO_NUM, 1 );
			user_single_socket.power = false;
			user_single_socket_update_datapoint();
			xlink_datapoint_update_all();
		}
	}
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_datapoint_init()
{
	uint8_t i;
	for ( i = 0; i < DATAPOINT_MAX_NUM; i++ )
	{
		p_datapoints[i] = NULL;
	}
	p_datapoints[0] = xlink_datapoint_init_uint16( (uint8_t *)&m_socket_config.zone );
	/* sensor */
	for ( i = 0; i < SENSOR_COUNT_MAX; i++ )
	{
		p_datapoints[4+6*i] = xlink_datapoint_init_byte( (uint8_t*)&user_single_socket.sensor[i].available );
		p_datapoints[5+6*i] = xlink_datapoint_init_byte( (uint8_t *)&user_single_socket.sensor[i].type );
		p_datapoints[6+6*i] = xlink_datapoint_init_int16( (uint8_t *)&user_single_socket.sensor[i].value );
		p_datapoints[7+6*i] = xlink_datapoint_init_byte( (uint8_t *)&user_single_socket.sensor[i].notify_enable );
		p_datapoints[8+6*i] = xlink_datapoint_init_byte( (uint8_t *)&user_single_socket.sensor[i].linkage_enable );
		p_datapoints[9+6*i] = xlink_datapoint_init_binary( user_single_socket.sensor[i].linkage_arg.length+2, (uint8_t *)&user_single_socket.sensor[i].linkage_arg );
	}
	/* socket */
//	p_datapoints[16] = xlink_datapoint_init_string( 0, (uint8_t *)user_single_socket.sensor1.name );
	p_datapoints[17] = xlink_datapoint_init_byte( (uint8_t *)&user_single_socket.power );
	p_datapoints[18] = xlink_datapoint_init_binary( 4 + 4*user_single_socket.p_timer->count, (uint8_t *)user_single_socket.p_timer );
}

LOCAL bool ICACHE_FLASH_ATTR user_single_socket_linkage_args_changed()
{
	uint8_t i, j;
	for(i = 0; i < SENSOR_COUNT_MAX; i++)
	{
		if(user_single_socket.sensor[i].notify_enable != user_sensor[i].notify_enable)
		{
			return true;
		}
		if(user_single_socket.sensor[i].linkage_enable != user_sensor[i].linkage_enable)
		{
			return true;
		}
		for(j = 0; j < SENSOR_ARGS_MAX; j++)
		{
			if(user_single_socket.sensor[i].linkage_arg.arg_array[j] != user_sensor[i].linkage_arg.arg_array[j])
			{
				return true;
			}
		}
	}
	return false;
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_datapoint_changed_cb()
{
	if(user_single_socket_linkage_args_changed())
	{
		user_single_socket_set_sensor();
	}
	GPIO_OUTPUT_SET( user_single_socket.pin, user_single_socket.power );
	GPIO_OUTPUT_SET( LEDB_IO_NUM, user_single_socket.power ? 0 : 1 );
	user_single_socket_update_datapoint();
	xlink_datapoint_update_all();
	user_single_socket_save_para();
	// user_single_socket_print();
}

LOCAL void user_single_socket_update_datapoint()
{
	uint8_t i;
	for ( i = 0; i < SENSOR_COUNT_MAX; i++ )
	{
		p_datapoints[9+6*i]->length = user_single_socket.sensor[i].linkage_arg.length+2;
	}
	user_single_socket_update_timers();
	p_datapoints[18]->length = 4 + 4*user_single_socket.p_timer->count;
}

LOCAL void ICACHE_FLASH_ATTR user_single_socket_detect_sensor()
{
	LOCAL uint8_t trg;
	LOCAL uint8_t cont;
	uint8_t rd = GPIO_INPUT_GET( DETECT_IO_NUM ) ^ 0x01 ;
	trg = rd & ( rd ^ cont );
	cont = rd;
	bool detect = trg ^ cont;
	if ( m_sensor_detected == true && detect == false )
	{
		m_sensor_detected = false;
		uint8_t i;
		for ( i = 0; i < SENSOR_COUNT_MAX; i++ )
		{
			os_memset( &user_sensor[i], 0, sizeof( user_sensor[i] ) );
			os_memset( &user_single_socket.sensor[i], 0, sizeof( user_single_socket.sensor[i] ) );
		}
		user_single_socket_update_datapoint();
		xlink_datapoint_update_all();
		app_printf( "sensor removed..." );
	}
	else if ( m_sensor_detected == false && detect == true )
	{
		m_sensor_detected = true;
		app_printf( "sensor detected..." );
	}
}

/**
 * get:	FRM_HDR CMD_GET XOR
 * set:	FRM_HDR CMD_SET {ntfy linkage version len args...}... XOR
 * rsp: FRM_HDR CMD_GET {TYPE vL vH ntfy linkage version len args...}... XOR
 */
LOCAL void ICACHE_FLASH_ATTR user_single_socket_get_sensor()
{
	uint8_t xor = 0;
	xor ^= uart0_send_byte( FRAME_HEADER );
	xor ^= uart0_send_byte( CMD_GET );
	uart0_send_byte( xor );
}

/**
 * get:	FRM_HDR CMD_GET+chn XOR
 * set:	FRM_HDR CMD_SET+chn ntfy linkage version len args... XOR
 * rsp: FRM_HDR CMD_GET+chn TYPE vL vH ntfy linkage version len args... XOR
 */
// LOCAL void ICACHE_FLASH_ATTR user_single_socket_set_sensor( uint8_t chn )
// {
// 	if ( chn >= SENSOR_COUNT_MAX )
// 	{
// 		return;
// 	}
// 	user_sensor_t *psensor = &user_single_socket.sensor[chn];
// 	if ( psensor->linkage_arg.length > 0 && psensor->linkage_arg.length < SENSOR_ARGS_MAX )
// 	{
// 		uint8_t i;
// 		uint8_t xor = 0;
// 		xor ^= uart0_send_byte( FRAME_HEADER );
// 		xor ^= uart0_send_byte( CMD_SET + chn );
// 		xor ^= uart0_send_byte( psensor->notify_enable );
// 		xor ^= uart0_send_byte( psensor->linkage_enable );
// 		xor ^= uart0_send_byte( psensor->linkage_arg.version );
// 		xor ^= uart0_send_byte( psensor->linkage_arg.length );
// 		for ( i = 0; i < psensor->linkage_arg.length; i++ )
// 		{
// 			xor ^= uart0_send_byte( psensor->linkage_arg.arg_array[i] );
// 		}
// 		uart0_send_byte( xor );
// 	}
// }

/**
 * get:	FRM_HDR CMD_GET XOR
 * set:	FRM_HDR CMD_SET {ntfy linkage version len args...}... XOR
 * rsp: FRM_HDR CMD_GET {TYPE vL vH ntfy linkage version len args...}... XOR
 */
LOCAL void ICACHE_FLASH_ATTR user_single_socket_set_sensor()
{
	uint8_t i, j;
	uint8_t xor = 0;
	xor ^= uart0_send_byte( FRAME_HEADER );
	xor ^= uart0_send_byte( CMD_SET );
	for(i = 0; i < SENSOR_COUNT_MAX; i++)
	{
		if(user_single_socket.sensor[i].available)
		{
			xor ^= uart0_send_byte( user_single_socket.sensor[i].notify_enable );
			xor ^= uart0_send_byte( user_single_socket.sensor[i].linkage_enable );
			xor ^= uart0_send_byte( user_single_socket.sensor[i].linkage_arg.version );
			xor ^= uart0_send_byte( user_single_socket.sensor[i].linkage_arg.length );
			for ( j = 0; j < user_single_socket.sensor[i].linkage_arg.length; j++ )
			{
				xor ^= uart0_send_byte( user_single_socket.sensor[i].linkage_arg.arg_array[j] );
			}
		}
		else
		{
			break;
		}
	}
	uart0_send_byte( xor );
}

/**
 * get:	FRM_HDR CMD_GET XOR
 * set:	FRM_HDR CMD_SET {ntfy linkage version len args...}... XOR
 * rsp: FRM_HDR CMD_GET {TYPE vL vH ntfy linkage version len args...}... XOR
 */
void ICACHE_FLASH_ATTR user_single_socket_decode_sensor( uint8_t *pbuf, uint8_t len )
{
	if( pbuf == NULL || len < 10 || pbuf[0] != FRAME_HEADER || pbuf[1] != CMD_GET )
	{
		return;
	}
	uint8_t i;
	uint8_t xor = 0;
	for ( i = 0; i < len; i++ )
	{
		xor ^= pbuf[i];
	}
	if( xor != 0 )
	{
		return;
	}
	if(len == pbuf[8]+10)
	{
		user_sensor[0].available = true;
		user_sensor[0].type = pbuf[2];
		user_sensor[0].value = (pbuf[4]<<8)|pbuf[3];
		user_sensor[0].notify_enable = pbuf[5];
		user_sensor[0].linkage_enable = pbuf[6];
		os_memcpy((uint8_t *) &user_sensor[0].linkage_arg, &pbuf[7], pbuf[8]+2);
		user_sensor[1].available = false;

		os_memcpy(&user_single_socket.sensor[0], &user_sensor[0], sizeof(user_sensor_t));
		os_memcpy(&user_single_socket.sensor[1], &user_sensor[1], sizeof(user_sensor_t));
		user_single_socket_update_datapoint();
		xlink_datapoint_update_all();
	}
	else if(len == pbuf[8] + pbuf[pbuf[8]+15] + 17)
	{
		user_sensor[0].available = true;
		user_sensor[0].type = pbuf[2];
		user_sensor[0].value = (pbuf[4]<<8)|pbuf[3];
		user_sensor[0].notify_enable = pbuf[5];
		user_sensor[0].linkage_enable = pbuf[6];
		os_memcpy((uint8_t *) &user_sensor[0].linkage_arg, &pbuf[7], pbuf[8]+2);

		uint8_t idx = 9 + pbuf[8];
		user_sensor[1].available = true;
		user_sensor[1].type = pbuf[idx];
		user_sensor[1].value = (pbuf[idx+2]<<8)|pbuf[idx+1];
		user_sensor[1].notify_enable = pbuf[idx+3];
		user_sensor[1].linkage_enable = pbuf[idx+4];
		os_memcpy((uint8_t *) &user_sensor[1].linkage_arg, &pbuf[idx+5], pbuf[idx+6]+2);

		os_memcpy(&user_single_socket.sensor[0], &user_sensor[0], sizeof(user_sensor_t));
		os_memcpy(&user_single_socket.sensor[1], &user_sensor[1], sizeof(user_sensor_t));
		user_single_socket_update_datapoint();
		xlink_datapoint_update_all();
	}
	// uint8_t chn = 0;
	// if ( pbuf[1] >= CMD_GET && pbuf[1] < CMD_GET + SENSOR_COUNT_MAX )
	// {
	// 	chn = pbuf[1] - CMD_GET;
	// 	user_sensor_t *psensor = &user_single_socket.sensor[chn];
	// 	psensor->available = true;
	// 	psensor->type = pbuf[2];
	// 	psensor->value = (pbuf[4]<<8)|pbuf[3];
	// 	psensor->notify_enable = pbuf[5];
	// 	psensor->linkage_enable = pbuf[6];
	// 	os_memcpy( (uint8_t *)&psensor->linkage_arg, &pbuf[7], pbuf[8]+2 );
	// 	user_single_socket_update_datapoint();
	// 	xlink_datapoint_update_all();
	// }
}
