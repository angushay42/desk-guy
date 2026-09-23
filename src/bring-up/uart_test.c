#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "driver/uart.h"

#include "esp_log.h"

#define UART_PORT 0
#define UART_TX GPIO_NUM_43
#define UART_RX GPIO_NUM_44

const char *TAG = "UART test";

void uart_test(void) {
    uart_config_t uart_cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    int intr_alloc_flags = 0;   // from echo example
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, 1024, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    const char *test = "UART TESTING!!!\r\n";
    size_t len = strlen(test);
    for (;;) {
        uart_write_bytes(UART_PORT, test, len);
    }

}

void app_main(void) 
{
    ESP_LOGI(TAG, "Beginning SPI test...");
    uart_test();
}