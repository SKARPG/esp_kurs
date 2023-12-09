#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/touch_sensor.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"
#include "esp_log.h"

#define FILTER_MODE                  0
#define INTERRUPT_MODE               1

#define TOUCH_THRESH_NO_USE          0
#define TOUCH_THRESH_PERCENT         80
#define TOUCHPAD_FILTER_TOUCH_PERIOD 10

static const char *TAG = "touch pad";


// interrupt handler
static void tp_rtc_intr(void* arg)
{
    bool* s_pad_activated = (bool*)arg;

    uint32_t pad_intr = touch_pad_get_status();

    // clear interrupt
    touch_pad_clear_status();
    for (uint8_t i = 0; i < TOUCH_PAD_MAX; i++)
    {
        if ((pad_intr >> i) & 0x01)
            s_pad_activated[i] = true;
        else
            s_pad_activated[i] = false;
    }
}


// read values sensed at all available touch pads
// use 2/3 of read value as the threshold to trigger interrupt when the pad is touched
// note: this routine demonstrates a simple way to configure activation threshold for the touch pads
// do not touch any pads when this routine is running (on application start)
static void tp_set_thresholds(uint32_t* s_pad_init_val)
{
    uint16_t touch_value;
    for (int i = 0; i < TOUCH_PAD_MAX; i++)
    {
        // read filtered value
        touch_pad_read_filtered(i, &touch_value);
        s_pad_init_val[i] = touch_value;
        ESP_LOGI(TAG, "test init: touch pad [%d] val is %d", i, touch_value);

        // set interrupt threshold
        ESP_ERROR_CHECK(touch_pad_set_thresh(i, touch_value * 2 / 3));
    }
}


// initialize the RTC IO before reading touch pad
static void tp_init()
{
    for (int i = 0; i < TOUCH_PAD_MAX; i++)
    {
        // init RTC IO and mode for touch pad.
        touch_pad_config(i, TOUCH_THRESH_NO_USE);
    }
}


void app_main(void)
{
    bool s_pad_activated[TOUCH_PAD_MAX]; // array for pads' activation status
    uint32_t s_pad_init_val[TOUCH_PAD_MAX]; // array for pads' initial values
    bool mode = INTERRUPT_MODE; // 0: filter mode, 1: interrupt mode
    uint32_t show_message = 0;

    ESP_ERROR_CHECK(touch_pad_init());

    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER); // set hardware timer
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V); // set voltage ranges

    tp_init();

    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD); // start filter

    tp_set_thresholds(s_pad_init_val);

    touch_pad_isr_register(tp_rtc_intr, s_pad_activated); // register interrupt handler

    while (1)
    {
        if (mode == 0) //interrupt mode, enable touch interrupt
        {
            touch_pad_intr_enable();
            for (uint8_t i = 0; i < TOUCH_PAD_MAX; i++)
            {
                if (s_pad_activated[i] == true)
                {
                    ESP_LOGI(TAG, "T%d activated!", i);
                    // wait a while for the pad being released
                    vTaskDelay(200 / portTICK_PERIOD_MS);

                    // clear information on pad activation
                    s_pad_activated[i] = false;

                    // reset the counter triggering a message that application is running
                    show_message = 1;
                }
            }
        }
        else
        {
            // filter mode, disable touch interrupt
            touch_pad_intr_disable();
            touch_pad_clear_status();

            for (uint8_t i = 0; i < TOUCH_PAD_MAX; i++)
            {
                uint16_t value = 0;
                touch_pad_read_filtered(i, &value);

                if (value < s_pad_init_val[i] * TOUCH_THRESH_PERCENT / 100) 
                {
                    ESP_LOGI(TAG, "T%d activated!", i);
                    ESP_LOGI(TAG, "value: %"PRIu16"; init val: %"PRIu32, value, s_pad_init_val[i]);
                    vTaskDelay(200 / portTICK_PERIOD_MS);

                    // reset the counter triggering a message that application is running
                    show_message = 1;
                }
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);

        // if no pad is touched, every couple of seconds, show a message that application is running
        if (show_message++ % 500 == 0)
            ESP_LOGI(TAG, "Waiting for any pad being touched...");
    }
}