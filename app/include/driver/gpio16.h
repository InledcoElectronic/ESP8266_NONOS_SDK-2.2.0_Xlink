/*
 * gpio16.h
 *
 *  Created on: 2018年4月12日
 *      Author: liruya
 */

#ifndef GPIO16_H_
#define GPIO16_H_

#include "os_type.h"

extern void gpio16_output_conf(void);
extern void gpio16_output_set(uint8 value);
extern bool gpio16_output_get();

#endif /* GPIO16_H_ */
