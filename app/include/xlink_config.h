/*
 * xlink_config.h
 *
 *  Created on: 2018年3月31日
 *      Author: liruya
 */

#ifndef XLINK_CONFIG_H_
#define XLINK_CONFIG_H_

#include "c_types.h"

#define SPI_FLASH_SECTOR_SIZE       4096
/*need  to define for the User Data section*/
/* 2MBytes  512KB +  512KB  0x7C */
/* 2MBytes 1024KB + 1024KB  0xFC */
#define PRIV_PARAM_START_SECTOR     0x7C

extern uint32_t xlink_write_config( uint8_t *data, uint32_t len );
extern uint32_t xlink_read_config( uint8_t *data, uint32_t len );
extern uint32_t xlink_write_version( uint16_t version );
extern uint32_t xlink_read_version( uint16_t *version );
extern uint32_t xlink_write_user_para( uint8_t *data, uint32_t len );
extern uint32_t xlink_read_user_para( uint8_t *data, uint32_t len );

#endif /* XLINK_CONFIG_H_ */
