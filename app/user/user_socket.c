/*
 * user_socket.c
 *
 *  Created on: 2018年6月8日
 *      Author: liruya
 */

#include "user_socket.h"
#include "user_key.h"

#define SOCKET_FIRMWARE_VERSION			1
#define SOCKET_PRODUCT_ID				"160fa2b60c5503e9160fa2b60c555e01"
#define	SOCKET_PRODUCT_KEY				"24149adeae917a014282b40d39228bed"

#define	USER_KEY_NUM					1
#define USER_KEY_LONG_TIME_NORMAL		40
#define USER_KEY_LONG_TIME_SMARTCONFIG	200

#define	PARA_SAVED_FLAG					0x55
#define	PARA_DEFAULT_FLAG				0xFF
#define	SOCKET_TIMER_AVAILABLE_FLAG		0x55

#define	SENSOR_NAME_MAX_LENGTH			DATAPOINT_STR_MAX_LEN

#define	SENSOR_FRAME_HEADER				0x68
#define	SENSOR_TYPE_UNKOWN				0x00
#define	SENSOR_TYPE_TEMPERATURE			0x01
#define	SENSOR_TYPE_HUMIDITY			0x02
#define	SENSOR_TYPE_WATER_LEVEL			0x03
#define	SENSOR_TYPE_WATER_TEMPERATURE	0x04
#define	SENSOR_TYPE_INTENSITY			0x05

#define	SENSOR_NAME_UNKOWN				"Unkown Sensor"
#define	SENSOR_NAME_TEMPERATURE			"Temperature"
#define	SENSOR_NAME_HUMIDITY			"Humidity"
#define	SENSOR_NAME_WATER_LEVEL			"Water Level"
#define	SENSOR_NAME_WATER_TEMPERATURE	"Water Temperature"
#define	SENSOR_NAME_INTENSITY			"Intensity"

#define newSensor(nm)					{ 	.name = (nm),\
											.available = false,\
											.value = 0 }
#define newSingleSocket(pin,ptmr)		{	.pin_num = (pin),\
											.power = false,\
											.p_timer = (ptmr) }

LOCAL socket_config_t m_socket_config;

typedef enum timer_error{ TIMER_DISABLED, TIMER_ENABLED, TIMER_INVALID } timer_error_t;
typedef enum socket_state{ STATE_LOW, STATE_HIGH, STATE_INVALID } socket_state_t;

LOCAL void user_socket_init();
LOCAL void user_socket_process();
LOCAL void user_socket_datapoint_init();
LOCAL void user_socket_datapoint_changed_cb();
LOCAL void user_socket_detect_sensor();

LOCAL key_para_t *pkeys[USER_KEY_NUM];
LOCAL key_list_t key_list;
LOCAL bool m_sensor_detected;
LOCAL char sensor_name[SENSOR_COUNT_MAX][SENSOR_NAME_MAX_LENGTH];
LOCAL os_timer_t socket_proc_tmr;
LOCAL os_timer_t sensor_detect_tmr;
user_socket_t user_socket = {
	.xlink_device = newXlinkDevice( SOCKET_PRODUCT_ID,
									SOCKET_PRODUCT_KEY,
									SOCKET_FIRMWARE_VERSION,
									user_socket_init,
									user_socket_datapoint_changed_cb ),
	.channel_count = SOCKET_CHANNEL_COUNT,
	.sensor = { newSensor( &sensor_name[0][0] ),
				newSensor( &sensor_name[1][0] ),
				newSensor( &sensor_name[2][0] ),
				newSensor( &sensor_name[3][0] ) },
	.m_socket = { newSingleSocket(RELAY1_IO_NUM, &m_socket_config.socket_timers[0]) } 
};


LOCAL timer_error_t ICACHE_FLASH_ATTR user_socket_check_timer( socket_timer_t *p_timer )
{
	if ( p_timer == NULL )
	{
		return TIMER_INVALID;
	}
	if ( p_timer->flag != SOCKET_TIMER_AVAILABLE_FLAG )
	{
		return TIMER_INVALID;
	}
	if ( p_timer->timer > 1439 )
	{
		return TIMER_INVALID;
	}
	if ( p_timer->enable )
	{
		return TIMER_ENABLED;
	}
	return TIMER_DISABLED;
}

LOCAL void ICACHE_FLASH_ATTR user_socket_print()
{
	uint8_t i, j, k;
	uint8_t len = 0;
	char str[256];
	char *week[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	os_memset( str, '\0', 256 );
	len += os_sprintf( str+len, "chn_cnt:%d\n", user_socket.channel_count );
	for ( i = 0; i < SENSOR_COUNT_MAX; i++ )
	{
		if ( user_socket.sensor[i].available )
		{
			len += os_sprintf( str+len, "sensor%d:%s    available:true    value:%d", i, user_socket.sensor[i].name, user_socket.sensor[i].value );
		}
	}
	for ( i = 0; i < user_socket.channel_count; i++ )
	{
		len += os_sprintf( str+len, "Socket%d: Power:%s timers:%d\n", i, user_socket.m_socket[i].power ? "On" : "Off", user_socket.m_socket[i].p_timer->count );
		for ( j = 0; j < user_socket.m_socket[i].p_timer->count && j < SOCKET_TIMER_MAX; j++ )
		{
			socket_timer_t *ptmr = &user_socket.m_socket[i].p_timer->timers[j];
			if ( user_socket_check_timer( ptmr ) != TIMER_INVALID )
			{
				len += os_sprintf( str+len, "\t%s ", ptmr->enable ? "Enabled" : "Disabled" );
				for ( k = 7; k > 0; k-- )
				{
					len += os_sprintf( str+len, " %s ", (ptmr->repeat & (1<<(k-1))) ? week[k-1] : "*" );
				}
				len += os_sprintf( str+len, " %d:%d ->%s\n", ptmr->timer/60, ptmr->timer%60, ptmr->power_state ? "Turnon" : "Turnoff" );
			}
		}
	}
	app_printf( "%s", str );
}

LOCAL void ICACHE_FLASH_ATTR user_socket_default_para()
{
	os_memset( (uint8_t *)&m_socket_config, 0, sizeof( m_socket_config ) );
	m_socket_config.saved_flag = PARA_DEFAULT_FLAG;
	m_socket_config.zone = 800;
}

LOCAL void ICACHE_FLASH_ATTR user_socket_save_para()
{
	m_socket_config.saved_flag = PARA_SAVED_FLAG;
	xlink_write_user_para( (uint8_t *)&m_socket_config, sizeof( m_socket_config ) );
}

LOCAL void ICACHE_FLASH_ATTR user_socket_para_init()
{
	xlink_read_user_para( (uint8_t *)&m_socket_config, sizeof( m_socket_config ) );
	if ( m_socket_config.saved_flag != PARA_SAVED_FLAG )
	{
		user_socket_default_para();
	}
	if ( m_socket_config.zone >= 2400 )
	{
		m_socket_config.zone = 0;
	}
	uint8_t i, j;
	for ( i = 0; i < SOCKET_CHANNEL_COUNT; i++ )
	{
		if ( m_socket_config.socket_timers[i].count > SOCKET_TIMER_MAX )
		{
			m_socket_config.socket_timers[i].count = 0;
		}
		for ( j = 0; j < m_socket_config.socket_timers[i].count; j++ )
		{
			if ( user_socket_check_timer( &m_socket_config.socket_timers[i].timers[j] ) == TIMER_INVALID )
			{
				m_socket_config.socket_timers[i].count = j;
				break;
			}
		}
	}
}

LOCAL void ICACHE_FLASH_ATTR user_socket_key_short_press_cb()
{
	if ( user_smartconfig_status() )
	{
		return;
	}
	user_tcp_disconnect();
	user_smartconfig_start();
	app_printf( "key pressed, smartconfig start..." );
}

LOCAL void ICACHE_FLASH_ATTR user_socket_key_long_press_cb()
{
	xlink_disconnect_cloud();
	xlink_reset();
	user_socket_default_para();
	user_socket_save_para();
}

LOCAL void ICACHE_FLASH_ATTR user_socket_key_cont_press_cb()
{

}

LOCAL void ICACHE_FLASH_ATTR user_socket_key_release_cb()
{
	system_restart();
}

LOCAL void ICACHE_FLASH_ATTR user_socket_key_init()
{
	pkeys[0] = user_key_init_single( BOOT_IO_NUM,
								  	BOOT_IO_FUNC,
									BOOT_IO_MUX,
									user_socket_key_short_press_cb,
									user_socket_key_long_press_cb,
									user_socket_key_cont_press_cb,
									user_socket_key_release_cb );
	pkeys[0]->long_count = 200;
	key_list.key_num = USER_KEY_NUM;
	key_list.pkeys = pkeys;
	user_key_init_list( &key_list );
}

LOCAL void ICACHE_FLASH_ATTR user_socket_init()
{
	user_socket_para_init();
	user_socket_datapoint_init();
	user_socket_key_init();
	os_timer_disarm( &socket_proc_tmr );
	os_timer_setfn( &socket_proc_tmr, user_socket_process, NULL );
	os_timer_arm( &socket_proc_tmr, 1000, 1 );
	os_timer_disarm( &sensor_detect_tmr );
	os_timer_setfn( &sensor_detect_tmr, user_socket_detect_sensor, NULL );
	os_timer_arm( &sensor_detect_tmr, 32, 1 );
}

LOCAL void ICACHE_FLASH_ATTR user_socket_process()
{
	uint8_t i, j, pin;
	bool flag = false;
	uint16_t ct;
	socket_timer_t *p = NULL;
	uint8_t sec = user_rtc_get_second();
	if ( sec == 0 )
	{
		ct = user_rtc_get_hour() * 60u + user_rtc_get_minute();
		for ( i = 0; i < SOCKET_CHANNEL_COUNT; i++ )
		{
			pin = user_socket.m_socket[i].pin_num;
			p = user_socket.m_socket[i].p_timer->timers;
			for ( j = 0; j < SOCKET_TIMER_MAX; j++ )
			{
				if ( user_socket_check_timer( p ) == TIMER_ENABLED && ct == p->timer )
				{
					GPIO_OUTPUT_SET( pin, p->power_state );
					if ( p->repeat == 0 )
					{
						p->enable = false;
						user_socket.m_socket[i].power = p->power_state;
						GPIO_OUTPUT_SET( pin, p->power_state );
						flag = true;
					}
					else if ( (p->repeat&(1<<user_rtc_get_week())) != 0 )
					{
						user_socket.m_socket[i].power = p->power_state;
						GPIO_OUTPUT_SET( pin, p->power_state );
						flag = true;
					}
				}
				p++;
			}
		}
		if ( flag )
		{
			user_socket_print();
			xlink_datapoint_update_all();
		}
	}
}

LOCAL void ICACHE_FLASH_ATTR user_socket_datapoint_init()
{
	uint8_t i;
	for ( i = 0; i < DATAPOINT_MAX_NUM; i++ )
	{
		p_datapoints[i] = NULL;
	}
	p_datapoints[0] = xlink_datapoint_init_byte( (uint8_t *)&user_socket.channel_count );
	p_datapoints[1] = xlink_datapoint_init_uint16( (uint8_t *)&m_socket_config.zone );
	/* sensor */
	for ( i = 0; i < SENSOR_COUNT_MAX; i++ )
	{
		p_datapoints[4+3*i] = xlink_datapoint_init_string( 0, (uint8_t*)user_socket.sensor[i].name );
		p_datapoints[5+3*i] = xlink_datapoint_init_byte( (uint8_t *)&user_socket.sensor[i].available );
		p_datapoints[6+3*i] = xlink_datapoint_init_int16( (uint8_t *)&user_socket.sensor[i].value );
	}
	/* socket */
	for ( i = 0; i < SOCKET_CHANNEL_COUNT; i++ )
	{
//		p_datapoints[16] = xlink_datapoint_init_string( 0, (uint8_t *)user_socket.sensor1.name );
		p_datapoints[17+3*i] = xlink_datapoint_init_byte( (uint8_t *)&user_socket.m_socket[i].power );
		p_datapoints[18+3*i] = xlink_datapoint_init_binary( 4 + 4*user_socket.m_socket[i].p_timer->count, (uint8_t *)user_socket.m_socket[i].p_timer );
	}
}

LOCAL void ICACHE_FLASH_ATTR user_socket_datapoint_changed_cb()
{
	uint8_t i;
	for ( i = 0; i < SOCKET_CHANNEL_COUNT; i++ )
	{
		GPIO_OUTPUT_SET( user_socket.m_socket[i].pin_num, user_socket.m_socket[i].power );
		app_printf( "power:%d timer:%d-", user_socket.m_socket[i].power, user_socket.m_socket[i].p_timer->count );
	}
	xlink_datapoint_update_all();
	user_socket_save_para();
	user_socket_print();
}

LOCAL void ICACHE_FLASH_ATTR user_socket_detect_sensor()
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
			user_socket.sensor[i].available = false;
			user_socket.sensor[i].value = 0;
			os_memset( sensor_name[i], '\0', SENSOR_NAME_MAX_LENGTH );
			p_datapoints[4+3*i]->length = 0;
		}
		xlink_datapoint_update_all();
		app_printf( "sensor removed..." );
	}
	else if ( m_sensor_detected == false && detect == true )
	{
		m_sensor_detected = true;
		app_printf( "sensor detected..." );
	}
}

void ICACHE_FLASH_ATTR user_socket_decode_sensor( uint8_t *pbuf, uint8_t len )
{
	if( pbuf == NULL || len < 6 || pbuf[0] != SENSOR_FRAME_HEADER )
	{
		return;
	}
	if( len == pbuf[1] + 3 && pbuf[1]%3 == 0 )
	{
		uint8_t i;
		uint8_t xor = 0;
		for ( i = 0; i < len; i++ )
		{
			xor ^= pbuf[i];
		}
		if( xor == 0 )
		{
			uint8_t idx;
			for ( i = 2; i < len; i += 3 )
			{
				idx = (i-2)/3;
				user_socket.sensor[i].available = true;
				user_socket.sensor[i].value = (pbuf[i+2]<<8)|pbuf[i+1];
				switch( pbuf[i] )
				{
					case SENSOR_TYPE_TEMPERATURE:
						os_strcpy( sensor_name[idx], SENSOR_NAME_TEMPERATURE );
						break;	
					case SENSOR_TYPE_HUMIDITY:
						os_strcpy( sensor_name[idx], SENSOR_NAME_HUMIDITY );
						break;
					case SENSOR_TYPE_WATER_LEVEL:
						os_strcpy( sensor_name[idx], SENSOR_NAME_WATER_LEVEL );
						break;
					case SENSOR_TYPE_WATER_TEMPERATURE:
						os_strcpy( sensor_name[idx], SENSOR_NAME_WATER_TEMPERATURE );
						break;
					case SENSOR_TYPE_INTENSITY:
						os_strcpy( sensor_name[idx], SENSOR_NAME_INTENSITY );
						break;
					default:
						os_strcpy( sensor_name[idx], SENSOR_NAME_UNKOWN );
						break;
				}
				p_datapoints[4+3*idx]->length = os_strlen( sensor_name[idx] );
			}
			xlink_datapoint_update_all();
		}
	}
}
