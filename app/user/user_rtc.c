/*
 * user_rtc.c
 *
 *  Created on: 2018年4月2日
 *      Author: liruya
 */

#include "user_rtc.h"
#include "user_tcp_client.h"

xlink_datetime_t g_datetime;
LOCAL os_timer_t sync_timer;
LOCAL os_timer_t tmr_1sec;

LOCAL const MONTH[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

LOCAL void ICACHE_FLASH_ATTR user_rtc_1s_cb( void *arg );

void ICACHE_FLASH_ATTR user_rtc_init()
{
	os_memset( &g_datetime, 0, sizeof( g_datetime ) );
	g_datetime.year = 2000;
	g_datetime.month = 1;
	g_datetime.day = 1;

	os_timer_disarm( &tmr_1sec );
	os_timer_setfn( &tmr_1sec, user_rtc_1s_cb, NULL );
}

void ICACHE_FLASH_ATTR user_rtc_sync_cloud_cb( struct xlink_sdk_event_t *pevent )
{
	if ( pevent == NULL )
	{
		return;
	}
	os_timer_disarm( &sync_timer );
	os_timer_disarm( &tmr_1sec );
	g_datetime.year = pevent->event_struct_t.datetime_t.year;
	g_datetime.month = pevent->event_struct_t.datetime_t.month;
	g_datetime.day = pevent->event_struct_t.datetime_t.day;
	g_datetime.week = pevent->event_struct_t.datetime_t.week;
	g_datetime.hour = pevent->event_struct_t.datetime_t.hour;
	g_datetime.min = pevent->event_struct_t.datetime_t.min;
	g_datetime.second = pevent->event_struct_t.datetime_t.second;
	g_datetime.zone = pevent->event_struct_t.datetime_t.zone;
//	os_memcpy( &datetime, &pevent->event_struct_t.datetime_t, sizeof( datetime ) );
//	os_printf( "get time : %d-%d-%d %d:%d:%d  week:%d  timezone:%d\n", pevent->event_struct_t.datetime_t.year,
//																		pevent->event_struct_t.datetime_t.month,
//																		pevent->event_struct_t.datetime_t.day,
//																		pevent->event_struct_t.datetime_t.hour,
//																		pevent->event_struct_t.datetime_t.min,
//																		pevent->event_struct_t.datetime_t.second,
//																		pevent->event_struct_t.datetime_t.week,
//																		pevent->event_struct_t.datetime_t.zone);
	os_timer_arm( &sync_timer, 60000, 1 );
	os_timer_arm( &tmr_1sec, 1000, 1 );
}

LOCAL void ICACHE_FLASH_ATTR user_rtc_sync_cloud( void *arg )
{
	uint16_t msgid = 0;
	struct xlink_sdk_event_t event;
	struct xlink_sdk_event_t *pevent = &event;
	event.enum_event_type_t = EVENT_TYPE_REQ_DATETIME;
	xlink_post_event( &msgid, &pevent );
//	os_printf( "sync time..." );
}

void ICACHE_FLASH_ATTR user_rtc_get_time()
{
	user_rtc_sync_cloud( NULL );
	os_timer_disarm( &sync_timer );
	os_timer_setfn( &sync_timer, user_rtc_sync_cloud, NULL );
	os_timer_arm( &sync_timer, 5000, 1 );
}

LOCAL bool ICACHE_FLASH_ATTR user_rtc_is_leap_year( uint16_t year )
{
	if ( year%4 != 0 )
	{
		return false;
	}
	if ( year%100 == 0 && year%400 != 0 )
	{
		return false;
	}
	return true;
}

LOCAL void ICACHE_FLASH_ATTR user_rtc_1s_cb( void *arg )
{
	uint8_t month_days;
	g_datetime.second++;
	if ( g_datetime.second > 59 )
	{
		g_datetime.second = 0;
		g_datetime.min++;
		if ( g_datetime.min > 59 )
		{
			g_datetime.min = 0;
			g_datetime.hour++;
			if ( g_datetime.hour > 23 )
			{
				g_datetime.hour = 0;
				g_datetime.week++;
				if(g_datetime.week > 6)
				{
					g_datetime.week = 0;
				}
				g_datetime.day++;
				month_days = MONTH[g_datetime.month-1];
				if ( g_datetime.month == 2 && user_rtc_is_leap_year( g_datetime.year ) )
				{
					month_days += 1;
				}
				if ( g_datetime.day > month_days )
				{
					g_datetime.day = 1;
					g_datetime.month++;
					if ( g_datetime.month > 12 )
					{
						g_datetime.month = 1;
						g_datetime.year++;
					}
				}
			}
		}
	}
}

uint16_t ICACHE_FLASH_ATTR user_rtc_get_year()
{
	return g_datetime.year;
}

uint8_t ICACHE_FLASH_ATTR user_rtc_get_month()
{
	return g_datetime.month;
}

uint8_t ICACHE_FLASH_ATTR user_rtc_get_day()
{
	return g_datetime.day;
}

uint8_t ICACHE_FLASH_ATTR user_rtc_get_week()
{
	return g_datetime.week;
}

uint8_t ICACHE_FLASH_ATTR user_rtc_get_hour()
{
	return g_datetime.hour;
}

uint8_t ICACHE_FLASH_ATTR user_rtc_get_minute()
{
	return g_datetime.min;
}

uint8_t ICACHE_FLASH_ATTR user_rtc_get_second()
{
	return g_datetime.second;
}

uint16_t ICACHE_FLASH_ATTR user_rtc_get_zone()
{
	return g_datetime.zone;
}
