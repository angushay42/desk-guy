extern "C" {
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

#include "driver/spi_master.h"
#include "hal/lcd_types.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_ops.h"

#include "esp_freertos_hooks.h"
#include "esp_log.h"
#include <math.h>
}
#include <ESP_TFT_RoboEyes.hpp>
#include <Display.hpp>


static const char *TAG = "main";

bool test_idle_hook_cb(void) {
    ESP_LOGI(TAG, "Idle task called");
    return true;
}

extern "C" void app_main(void) 
{
    esp_register_freertos_idle_hook_for_cpu(test_idle_hook_cb, 0);
    static auto disp = Display(240, 340);
    static auto eyes = ESP_TFT_RoboEyes(disp, true, 0);  // portrait, rotations?

    disp.init();
    eyes.begin(100);    // 50fps?

    eyes.setAutoblinker(true, 2, 1);
    eyes.setIdleMode(true, 4, 0);
    eyes.setWidth(64,64);
    eyes.setHeight(64,64);
    eyes.setBorderradius(10,10);
    eyes.setSpacebetween(36);

    for (;;) 
    {
        eyes.update();
    }
}