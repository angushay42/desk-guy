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

// note: D = MOSI, Q = MISO
#define MOSI    GPIO_NUM_13
#define MISO    GPIO_NUM_11
#define SCLK    GPIO_NUM_12
#define CS      GPIO_NUM_10

#define SPI_HOST SPI2_HOST

static const char *TAG = "SPI test";

void spi_test(void) {
    ESP_LOGD(TAG, "Testing CS pin...");
    gpio_config_t cs_cfg = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << CS
    };
    ESP_ERROR_CHECK(gpio_config(&cs_cfg));

    vTaskDelay(500/ portTICK_PERIOD_MS);

    gpio_set_level(CS, 1);
    vTaskDelay(500/ portTICK_PERIOD_MS);
    gpio_set_level(CS, 0);

    gpio_reset_pin(CS);
    ESP_LOGD(TAG, "CS Pin test complete");


    ESP_LOGI(TAG, "Creating bus config...");
    // create an SPI bus
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = SCLK,
        .mosi_io_num = MOSI,
        .miso_io_num = -1,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
        .max_transfer_sz = 64
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Creating SPI handle...");
    spi_device_handle_t spidev_handle = NULL;
    spi_device_interface_config_t spidev_cfg = {
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI_HOST, &spidev_cfg, &spidev_handle));

    ESP_LOGI(TAG, "Creating transaction...");

    static char test_buffer[64];
    strcpy(test_buffer, "SPI CAN YOU HEAR ME??\r\n");
    size_t len = strlen(test_buffer);

    spi_transaction_t trans = {
        .rx_buffer = NULL,          // no MISO phase
        .tx_buffer = test_buffer,   // 
        .length = len * 8,          // in bits !!
    };

    
    ESP_LOGI(TAG, "Starting loop...");
    for (;;) {
        ESP_LOGI(TAG, "SPI transmitting...");
        
        ESP_ERROR_CHECK(spi_device_transmit(spidev_handle, &trans));
        // vTaskDelay(500 / portTICK_PERIOD_MS);
        taskYIELD();
    }
}

bool test_idle_hook_cb(void) {
    ESP_LOGI(TAG, "Idle task called");
    return true;
}

void app_main(void) 
{
    esp_register_freertos_idle_hook_for_cpu(test_idle_hook_cb, 0);
    ESP_LOGI(TAG, "Beginning SPI test...");
    spi_test();
}