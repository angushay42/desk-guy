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

#include "esp_heap_caps.h"

#include "esp_freertos_hooks.h"
#include "esp_log.h"
#include <math.h>
}
#include <ESP_IDF_RoboEyes.hpp>
#include <Display.hpp>


static const char *TAG = "main";

bool test_idle_hook_cb(void) {
    // ESP_LOGI(TAG, "Idle task called");
    return true;
}

void eyes_main(void) 
{
    ESP_LOGD(TAG, "Creating display...");

    static auto disp = Display(240, 320, 16);
    ESP_LOGD(TAG, "Creating RoboEyes...");

    static auto eyes = ESP_TFT_RoboEyes(disp, true, 0);  // portrait, rotations?

    disp.init();
    ESP_LOGD(TAG, "RoboEyes.begin()");
    eyes.begin(50);    // 50fps?

    eyes.setAutoblinker(true, 2, 1);
    eyes.setIdleMode(true, 4, 0);
    eyes.setWidth(64,64);
    eyes.setHeight(64,64);
    eyes.setBorderradius(10,10);
    eyes.setSpacebetween(36);


    for (;;) 
    {
        ESP_LOGD(TAG, "Loop...");

        eyes.update();
        taskYIELD();
    }   
}

void test_main(void)
{
    ESP_LOGD(TAG, "Starting test...");
    
    static auto disp = Display(240, 320, 16);

    disp.init();

    static auto sprite = Sprite(&disp);
    sprite.createSprite(240, 320);

    uint16_t red;
    encode_rgb(255, 0, 0, red);

    sprite.fillSprite(0);   // black
    // sprite.fillRect(100, 140, 20, 20, red); 

    // sprite.fillRoundRect(0, 0, 100, 20, 5, red);

    sprite.fillTriangle(40, 120 , 200, 120, 60, 20, red);
    

    sprite.pushSprite(0,0);
    for (;;) 
    {
        ESP_LOGD(TAG, "Created is: %s", (sprite.isCreated()) ? "true": "false");
        vTaskDelay(100);
    }
}

extern "C" void app_main(void) 
{
    esp_register_freertos_idle_hook_for_cpu(test_idle_hook_cb, 0);

    eyes_main();
    // test_main();
    
}