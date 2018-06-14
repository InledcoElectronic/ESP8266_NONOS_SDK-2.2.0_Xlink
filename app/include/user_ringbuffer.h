/*
 * user_ringbuffer.h
 *
 *  Created on: 2017年10月30日
 *      Author: liruya
 */

#ifndef USER_RINGBUFFER_H_
#define USER_RINGBUFFER_H_

#include "c_types.h"

typedef struct{
	uint8_t *pbuf;
	uint32_t head;
	uint32_t tail;
	uint32_t size;
	bool lock;
}ringbuf_t;

extern void user_rb_init( ringbuf_t *p_rb, uint8_t *pbuf, uint32_t size );
extern void user_rb_reset( ringbuf_t *p_rb );
extern uint32_t user_rb_unread_size( ringbuf_t *p_rb );
extern uint32_t user_rb_put( ringbuf_t *p_rb, uint8_t *src, uint32_t len );
extern uint32_t user_rb_get( ringbuf_t *p_rb, uint8_t *des, uint32_t len );

#endif /* USER_RINGBUFFER_H_ */
