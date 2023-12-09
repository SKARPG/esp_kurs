#include <stdio.h>
#include "TCP_wifi_client.h"

static const char* TAG = "TCP wifi client";
static const char* payload = "Message from ESP32 ";


/**
 * @brief wifi event handler
 * 
 * @param arg wifi event argument
 * @param event_base event base
 * @param event_id event id
 * @param event_data event data
*/
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    wifi_event_arg_t *wifi_event_arg = (wifi_event_arg_t*)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (wifi_event_arg->s_retry_num < MAX_RETRY)
        {
            esp_wifi_connect();
            wifi_event_arg->s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
            xEventGroupSetBits(wifi_event_arg->s_wifi_event_group, WIFI_FAIL_BIT);

        ESP_LOGI(TAG,"connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_event_arg->s_retry_num = 0;
        xEventGroupSetBits(wifi_event_arg->s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


/**
 * @brief wifi station initialization
 * 
 * @param tcp_conf TCP wifi client configuration struct
*/
static void wifi_init_sta(TCP_wifi_conf_t tcp_conf)
{
    EventGroupHandle_t s_wifi_event_group = xEventGroupCreate();
    uint32_t s_retry_num = 0;

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    wifi_event_arg_t wifi_event_arg = {
        .s_wifi_event_group = s_wifi_event_group,
        .s_retry_num = s_retry_num
    };

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, 
                        (void*)&wifi_event_arg, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler,
                        (void*)&wifi_event_arg, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            // .ssid = ,
            // .password = ,

            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_HUNT_AND_PECK,
            .sae_h2e_identifier = "",
        },
    };

    // copy ssid and password to wifi_config struct
    memcpy(&wifi_config.sta.ssid, tcp_conf.ssid, strlen(tcp_conf.ssid)*sizeof(char));
    memcpy(&wifi_config.sta.password, tcp_conf.password, strlen(tcp_conf.password)*sizeof(char));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    // xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened.
    if (bits & WIFI_CONNECTED_BIT)
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", tcp_conf.ssid, tcp_conf.password);
    else if (bits & WIFI_FAIL_BIT)
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", tcp_conf.ssid, tcp_conf.password);
    else
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
}


/**
 * @brief TCP wifi client initialization
 * 
 * @param tcp_conf TCP wifi client configuration struct
*/
void TCP_wifi_init(TCP_wifi_conf_t tcp_conf)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // connect to wifi
    wifi_init_sta(tcp_conf);

    char rx_buffer[128];
    char host_ip[] = "192.168.1.140";
    int addr_family = 0;
    int ip_protocol = 0;

    // memcpy(&tcp_conf, tcp_conf.host_ip, sizeof(tcp_conf.host_ip)*sizeof(char));

    while (1)
    {
        struct sockaddr_in dest_addr;
        inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(tcp_conf.port);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%lu", host_ip, tcp_conf.port);

        int err = connect(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        if (err != 0)
        {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1)
        {
            int err = send(sock, payload, strlen(payload), 0);
            if (err < 0)
            {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }

            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0)
            {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else
            {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer);
            }
        }

        if (sock != -1) 
        {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
}
