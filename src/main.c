#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "hal/gpio_types.h"

static bool led = false;

/* NOTE: this is an interrupt context */
static bool blink_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *usr_ctx) {
    
    gpio_set_level(GPIO_NUM_12, (led) ? GPIO_INTR_HIGH_LEVEL: GPIO_INTR_LOW_LEVEL);
    led = !led;
    return false;
}

void app_main(void) 
{
    /* configure gpio pin 12 as output */
    gpio_config_t config = {};
    config.intr_type = GPIO_INTR_DISABLE;
    config.pin_bit_mask = 1ULL << GPIO_NUM_12;
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;

    /* set gpio pin with config */
    ESP_ERROR_CHECK(gpio_config(&config));
    
    /* create handle and config*/
    gptimer_handle_t blink_gptimer;
    gptimer_config_t blink_timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000 // 1MHz
    };
    /* create the timer */
    ESP_ERROR_CHECK(gptimer_new_timer(&blink_timer_config, &blink_gptimer));

    /* configure alarm */
    gptimer_alarm_config_t blink_alarm_config = {
        .reload_count = 0,                  // set counter value to this when event occurs
        .alarm_count = 1 * 1000 * 1000,              // period of the alarm
        .flags.auto_reload_on_alarm = true  // periodic
    };
    // set action (bit amgbiguous naming)
    ESP_ERROR_CHECK(gptimer_set_alarm_action(blink_gptimer, &blink_alarm_config));

    gptimer_event_callbacks_t blink_callbacks = {
        .on_alarm = blink_callback
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(blink_gptimer, &blink_callbacks, NULL));
    ESP_ERROR_CHECK(gptimer_enable(blink_gptimer));
    ESP_ERROR_CHECK(gptimer_start(blink_gptimer));


    for (;;) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }   
}