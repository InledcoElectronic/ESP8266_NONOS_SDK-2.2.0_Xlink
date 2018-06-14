/*
 * user_upgrade.c
 *
 *  Created on: 2018年3月2日
 *      Author: liruya
 */

#include "user_upgrade.h"
#include "c_types.h"
#include "osapi.h"
#include "mem.h"
#include "os_type.h"
#include "user_interface.h"

LOCAL os_timer_t client_timer;

#define packet_size   (2 * 1024)
#define	XLINK_FIRMWARE_HOST	"static.xlink.cn"
#define PHEADBUFFER "Connection: keep-alive\r\n\
Cache-Control: no-cache\r\n\
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36 \r\n\
Accept: */*\r\n\
Accept-Encoding: gzip,deflate,sdch\r\n\
Accept-Language: en-US,en;q=0.8,ja;q=0.6,zh-CN;q=0.4,zh-TW;q=0.2\r\n\r\n"

LOCAL void ICACHE_FLASH_ATTR user_esp_platform_upgrade_rsp(void *arg)
{
	struct upgrade_server_info *server = arg;
//	struct espconn *pespconn = server->pespconn;
//	uint8 *pbuf = NULL;
	char *action = NULL;

//	pbuf = (char *)os_zalloc(packet_size);

	if (server->upgrade_flag == true) {
		os_printf("user_esp_platform_upgarde_successfully\n");
		action = "device_upgrade_success";
		os_timer_disarm(&client_timer);
		os_timer_setfn(&client_timer, (os_timer_func_t *)system_upgrade_reboot, NULL);
		os_timer_arm(&client_timer, 1000, 0);
//		os_sprintf(pbuf, UPGRADE_FRAME, devkey, action, server->pre_version, server->upgrade_version);
//		ESP_DBG("%s\n",pbuf);

//#ifdef CLIENT_SSL_ENABLE
//		espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
//#else
//		espconn_sent(pespconn, pbuf, os_strlen(pbuf));
//#endif

//		if (pbuf != NULL) {
//			os_free(pbuf);
//			pbuf = NULL;
//		}
	} else {
		os_printf("user_esp_platform_upgrade_failed\n");
		action = "device_upgrade_failed";
//		os_sprintf(pbuf, UPGRADE_FRAME, devkey, action,server->pre_version, server->upgrade_version);
//		ESP_DBG("%s\n",pbuf);
//
//#ifdef CLIENT_SSL_ENABLE
//		espconn_secure_sent(pespconn, pbuf, os_strlen(pbuf));
//#else
//		espconn_sent(pespconn, pbuf, os_strlen(pbuf));
//#endif

//		if (pbuf != NULL) {
//			os_free(pbuf);
//			pbuf = NULL;
//		}
	}

	os_free(server->url);
	server->url = NULL;
	os_free(server);
	server = NULL;
}

void ICACHE_FLASH_ATTR user_upgrade_start( struct upgrade_server_info *server )
{
	server->ip[0] = 116;
	server->ip[1] = 62;
	server->ip[2] = 99;
	server->ip[3] = 24;
	server->port = 80;
	server->check_cb = user_esp_platform_upgrade_rsp;
	server->check_times = 120000;

	if (server->url == NULL) {
		server->url = (uint8 *)os_zalloc(512);
	}

	os_sprintf(server->url, "GET /008a5037d9b687d3ec5cc992c638d17c HTTP/1.1\r\nHost: "XLINK_FIRMWARE_HOST"\r\n"PHEADBUFFER"");
//	os_sprintf(server->url, "GET /bf5b49719000717e6c42b277a5aa5515 HTTP/1.1\r\nHost: 116.62.99.24:80\r\n"PHEADBUFFER"");
	os_printf("%s\n",server->url);

	if (system_upgrade_start(server) == false) {
		os_printf("upgrade is already started\n");
	}
}
