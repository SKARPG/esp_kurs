#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define MAX_RETRY          10
#define BUF_SIZE           128
#define QUEUE_SIZE         100
#define QUEUE_TIMEOUT      (100 / portTICK_PERIOD_MS)

typedef struct TCP_wifi_conf_t
{
    char* ssid;
    char* password;
    char* host_ip;
    uint32_t port;
} TCP_wifi_conf_t;

typedef struct wifi_event_arg_t
{
    EventGroupHandle_t s_wifi_event_group;
    uint32_t s_retry_num;
} wifi_event_arg_t;


void TCP_wifi_init(TCP_wifi_conf_t tcp_conf);

void TCP_wifi_send(char* data);

void TCP_wifi_receive(char* data);