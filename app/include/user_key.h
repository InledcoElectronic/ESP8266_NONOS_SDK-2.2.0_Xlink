/*
 * user_key.h
 *
 *  Created on: 2017年9月11日
 *      Author: liruya
 */

#ifndef USER_KEY_H_
#define USER_KEY_H_

#include "gpio.h"
#include "os_type.h"
#include "osapi.h"

typedef void (* key_function_t)();

typedef struct {
	uint16_t rpt_count;
	uint16_t long_count;
	uint8_t gpio_id;
	uint8_t gpio_func;
	uint32_t gpio_name;
	os_timer_t tmr_20ms;
	key_function_t short_press;
	key_function_t long_press;
	key_function_t cont_press;
	key_function_t release;
} key_para_t;

typedef struct {
	uint8_t key_num;
	key_para_t **pkeys;
} key_list_t;

extern key_para_t *user_key_init_single( uint8_t gpio_id, uint8_t gpio_func, uint32_t gpio_name,
		key_function_t short_press,
		key_function_t long_press,
		key_function_t cont_press,
		key_function_t release);
extern void user_key_init_list( key_list_t *plist );
extern void user_key_set_long_count( key_para_t *pkey, uint16_t count );

#endif /* USER_KEY_H_ */
