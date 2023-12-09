idf_component_register(SRCS "TCP_wifi_client.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash esp_netif lwip esp_wifi)
