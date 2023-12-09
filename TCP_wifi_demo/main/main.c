#include <stdio.h>
#include "TCP_wifi_client.h"

#define WIFI_SSID "AWG_Cisco"
#define WIFI_PASS "1q2w3e4r5t6y7u8i9o0"
#define WIFI_HOST_IP "192.168.1.140"
#define WIFI_PORT 3333


void app_main(void)
{
    TCP_wifi_conf_t tcp_conf = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
        .host_ip = WIFI_HOST_IP,
        .port = WIFI_PORT
    };

    TCP_wifi_init(tcp_conf);
}