#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"

#include "esp_freertos_hooks.h"
#include "esp_log.h"


#define TEST_PIN    GPIO_NUM_10
#define PIN_SEL(x) 1ULL << x

static const char *TAG = "Blink test";

static volatile bool led = false;
/* NOTE: this is an interrupt context */
static bool blink_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *usr_ctx) 
{
    gpio_set_level(TEST_PIN, (led) ? 1: 0);
    led = !led;
    return false;
}

void blink_test(void) {
    
    ESP_LOGI(TAG, "Creating config for GPIO pin...");
    /* configure gpio pin 12 as output */
    gpio_config_t config = {};
    config.intr_type = GPIO_INTR_DISABLE;
    config.pin_bit_mask = PIN_SEL(TEST_PIN);
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;

    /* set gpio pin with config */
    ESP_ERROR_CHECK(gpio_config(&config));

    ESP_LOGI(TAG, "Setting pin %d to 0...", TEST_PIN);
    /* set 0 to start */
    ESP_ERROR_CHECK(gpio_set_level(TEST_PIN, 0));

    ESP_LOGI(TAG, "Creating GPTimer handle and configuring...");
    /* create handle and config*/
    gptimer_handle_t blink_gptimer;
    gptimer_config_t blink_timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000 // 1MHz
    };
    /* create the timer */
    ESP_ERROR_CHECK(gptimer_new_timer(&blink_timer_config, &blink_gptimer));

    ESP_LOGI(TAG, "Configuring GPTimer alarm (interrupt)...");
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

    ESP_LOGI(TAG, "Begin loop...");
    for (;;) {
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }   
}


bool test_idle_hook_cb(void) {
    ESP_LOGI(TAG, "Idle task called");
    return true;
}


void app_main(void) 
{
    esp_register_freertos_idle_hook_for_cpu(test_idle_hook_cb, 0);
    ESP_LOGI(TAG, "Beginning test...");
    blink_test();
}