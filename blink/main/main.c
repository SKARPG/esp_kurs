#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BUILDIN_LED 22
#define DELAY_MS (1000 / portTICK_PERIOD_MS)


void app_main(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1 << BUILDIN_LED);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    while (1)
    {
        gpio_set_level(BUILDIN_LED, 1);
        printf("LED ON\n");
        vTaskDelay(DELAY_MS);

        gpio_set_level(BUILDIN_LED, 0);
        printf("LED OFF\n");
        vTaskDelay(DELAY_MS);
    }
}