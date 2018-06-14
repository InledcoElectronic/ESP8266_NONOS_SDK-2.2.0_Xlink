/*
 * xlink_config.c
 *
 *  Created on: 2018年3月31日
 *      Author: liruya
 */

#include "xlink_config.h"
#include "xlink_sdk.h"

#define XLINK_CONFIG_BUFFER_SIZE		200
#define USER_PARA_BUFFER_SIZE			200

typedef struct {
	uint8_t auth_pwd_name_buf[XLINK_CONFIG_BUFFER_SIZE];
	uint32_t sw_version;
	uint8_t user_para_buf[USER_PARA_BUFFER_SIZE];
} xlink_para_t;

LOCAL xlink_para_t xlink_para;

uint32_t XLINK_FUNCTION xlink_write_config( uint8_t *data, uint32_t len )
{
	os_memcpy( xlink_para.auth_pwd_name_buf, data, len );
	spi_flash_erase_sector( PRIV_PARAM_START_SECTOR );
	spi_flash_write( PRIV_PARAM_START_SECTOR * SPI_FLASH_SECTOR_SIZE, (uint32_t *) &xlink_para, sizeof( xlink_para ) );
	return len;
}

uint32_t XLINK_FUNCTION xlink_read_config( uint8_t *data, uint32_t len )
{
	spi_flash_read( PRIV_PARAM_START_SECTOR * SPI_FLASH_SECTOR_SIZE, (uint32_t *) &xlink_para, sizeof( xlink_para ) );
	os_memcpy( data, xlink_para.auth_pwd_name_buf, len );
	return len;
}

uint32_t XLINK_FUNCTION xlink_write_version( uint16_t version )
{
	xlink_para.sw_version = version;
	spi_flash_erase_sector( PRIV_PARAM_START_SECTOR );
	spi_flash_write( PRIV_PARAM_START_SECTOR * SPI_FLASH_SECTOR_SIZE, (uint32_t *) &xlink_para, sizeof( xlink_para ) );
	return 0;
}

uint32_t XLINK_FUNCTION xlink_read_version( uint16_t *version )
{
	spi_flash_read( PRIV_PARAM_START_SECTOR * SPI_FLASH_SECTOR_SIZE, (uint32_t *) &xlink_para, sizeof( xlink_para ) );
	*version = xlink_para.sw_version;
	return 0;
}

uint32_t XLINK_FUNCTION xlink_write_user_para( uint8_t *data, uint32_t len )
{
	os_memcpy( xlink_para.user_para_buf, data, len );
	spi_flash_erase_sector( PRIV_PARAM_START_SECTOR );
	spi_flash_write( PRIV_PARAM_START_SECTOR * SPI_FLASH_SECTOR_SIZE, (uint32_t *) &xlink_para, sizeof( xlink_para ) );
	return len;
}

uint32_t XLINK_FUNCTION xlink_read_user_para( uint8_t *data, uint32_t len )
{
	spi_flash_read( PRIV_PARAM_START_SECTOR * SPI_FLASH_SECTOR_SIZE, (uint32_t *) &xlink_para, sizeof( xlink_para ) );
	os_memcpy( data, xlink_para.user_para_buf, len );
	return len;
}
