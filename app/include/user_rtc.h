/*
 * user_rtc.h
 *
 *  Created on: 2018年4月2日
 *      Author: liruya
 */

#ifndef USER_RTC_H_
#define USER_RTC_H_

#include "xlink_sdk.h"

typedef void (* user_rtc_second_callback_t)();

extern void user_rtc_init();
extern void user_rtc_get_time();
extern void user_rtc_sync_cloud_cb( struct xlink_sdk_event_t *pevent );

#endif /* USER_RTC_H_ */
