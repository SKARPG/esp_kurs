#include <stdio.h>
#include "TCP_wifi_client.h"

#define WIFI_SSID       "piwo"
#define WIFI_PASS       "stary_pijany"
#define WIFI_HOST_IP    "192.168.0.84"
#define WIFI_PORT       23


void app_main(void)
{
    TCP_wifi_conf_t tcp_conf = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
        .host_ip = WIFI_HOST_IP,
        .port = WIFI_PORT
    };

    TCP_wifi_init(tcp_conf);

    char data[128];

    while (1)
    {
        TCP_wifi_send("Hello World!");
        TCP_wifi_receive(data);

        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // puts(data);
    }
}