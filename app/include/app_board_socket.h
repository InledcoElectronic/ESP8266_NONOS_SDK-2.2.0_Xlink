/*
 * app_board_socket.h
 *
 *  Created on: 2018年6月5日
 *      Author: liruya
 */

#ifndef APP_BOARD_SOCKET_H_
#define APP_BOARD_SOCKET_H_

#include "osapi.h"
#include "app_config.h"

#define PERIPHS_IO_MUX_GPIO1_U		PERIPHS_IO_MUX_U0TXD_U
#define PERIPHS_IO_MUX_GPIO3_U		PERIPHS_IO_MUX_U0RXD_U
#define PERIPHS_IO_MUX_GPIO12_U		PERIPHS_IO_MUX_MTDI_U
#define PERIPHS_IO_MUX_GPIO13_U		PERIPHS_IO_MUX_MTCK_U
#define PERIPHS_IO_MUX_GPIO14_U		PERIPHS_IO_MUX_MTMS_U
#define PERIPHS_IO_MUX_GPIO15_U		PERIPHS_IO_MUX_MTDO_U

//#define GPIO16_OUTPUT

#define BOOT_IO_NUM			0
#define BOOT_IO_MUX			PERIPHS_IO_MUX_GPIO0_U
#define BOOT_IO_FUNC		FUNC_GPIO0

#define LEDR_IO_NUM			13
#define LEDR_IO_MUX			PERIPHS_IO_MUX_GPIO13_U
#define LEDR_IO_FUNC		FUNC_GPIO13
#define LEDG_IO_NUM			4
#define LEDG_IO_MUX			PERIPHS_IO_MUX_GPIO4_U
#define LEDG_IO_FUNC		FUNC_GPIO4
#define LEDB_IO_NUM			12
#define LEDB_IO_MUX			PERIPHS_IO_MUX_GPIO12_U
#define LEDB_IO_FUNC		FUNC_GPIO12
#define DETECT_IO_NUM		14
#define DETECT_IO_MUX		PERIPHS_IO_MUX_GPIO14_U
#define DETECT_IO_FUNC		FUNC_GPIO14
#define RELAY1_IO_NUM		5
#define RELAY1_IO_MUX		PERIPHS_IO_MUX_GPIO5_U
#define RELAY1_IO_FUNC		FUNC_GPIO5
//#define RELAY1_IO_NUM		4
//#define RELAY1_IO_MUX		PERIPHS_IO_MUX_GPIO4_U
//#define RELAY1_IO_FUNC		FUNC_GPIO4

#define ledr_on()           GPIO_OUTPUT_SET(LEDR_IO_NUM, 0)
#define ledr_off()          GPIO_OUTPUT_SET(LEDR_IO_NUM, 1)
#define ledg_on()           GPIO_OUTPUT_SET(LEDG_IO_NUM, 0)
#define ledg_off()          GPIO_OUTPUT_SET(LEDG_IO_NUM, 1)
#define ledb_on()           GPIO_OUTPUT_SET(LEDB_IO_NUM, 0)
#define ledb_off()          GPIO_OUTPUT_SET(LEDB_IO_NUM, 1)
#define relay_on()          GPIO_OUTPUT_SET(RELAY1_IO_NUM, 1)
#define relay_off()         GPIO_OUTPUT_SET(RELAY1_IO_NUM, 0)

extern void app_board_socket_init();

#endif /* APP_BOARD_SOCKET_H_ */
