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

// note: D = MOSI, Q = MISO
#define MOSI    GPIO_NUM_16
#define MISO    GPIO_NUM_18
#define SCLK    GPIO_NUM_17
#define CS      GPIO_NUM_15
#define DC      GPIO_NUM_12 // Data/command 
#define RST     GPIO_NUM_11

#define LCD_HOST SPI2_HOST  // should be the fast SPI
#define DOT_CLK_HZ 18 * 1000 * 1000  // I think it is 18MHz, unsure... 
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8

#define LCD_H_RES 240
#define LCD_V_RES 320
/** it doesn't say this anywhere I can find; 
 * but, if 18bit color is used then 3 bytes are required. 
 */
#define LCD_RGB_BITS 16 // bits
#define LCD_MAX_LINES 20 // arbitrary number
#define LCD_MAX_TRANSFER LCD_MAX_LINES * LCD_H_RES * sizeof(uint16_t)

#define UART_PORT static_cast<uart_port_t>(0)
#define UART_TX GPIO_NUM_43
#define UART_RX GPIO_NUM_44

// todo ?
// #define SPI_PIN(x) GPIO_NUM_ x
// SPI_PIN(MOSI);

// todo
// static hex_to_rgb(const char *hx, int rgb_bits) {
//     assert(hx != NULL);
//     if ()
// }

static const char *TAG = "main";

static esp_err_t scale_rgb(uint8_t src, uint8_t *dest, size_t size) {
    assert(dest != NULL);
    if (size < 5 || size > 6)
        return ESP_ERR_INVALID_ARG;
    volatile double r_max, t_max, temp;
    r_max = (double) ((1 << size) - 1);
    t_max = 255.0;

    // char uart_buf[255];
    // size_t len;
    // // format message
    // len = snprintf(uart_buf, 255, "Scaling colour: %i. Reading max=%i, target max=%i.", src, r_max, t_max);

    // uart_write_bytes(
    //     UART_PORT, 
    //     (const char *) uart_buf,
    //     len
    // );
    ESP_LOGD(TAG, "Scaling colour: %i. Reading max=%f, target max=%f.", src, r_max, t_max);
    temp = round(((double) src * r_max) / t_max);
    ESP_LOGD(TAG, "Scaled src %u to %f", src, temp);
    *dest = (uint8_t) temp;

    return ESP_OK;
}

/* always returns a 5-6-5 rgb into a uint16_t */
static esp_err_t encode_rgb(uint8_t r, uint8_t g, uint8_t b, uint16_t *rgb) {
    assert(rgb != NULL);

    *rgb = 0;
    ESP_ERROR_CHECK(scale_rgb(r, &r, 5U));
    ESP_ERROR_CHECK(scale_rgb(g, &g, 6U));
    ESP_ERROR_CHECK(scale_rgb(b, &b, 5U));
    *rgb |= r << 11;
    *rgb |= g << 5;
    *rgb |= b;

    return ESP_OK;
}

// prototype
void blink_test(void);
void tft_display_test(void);

static bool led = false;
/* NOTE: this is an interrupt context */
static bool blink_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *usr_ctx) {
    
    gpio_set_level(GPIO_NUM_12, (led) ? 1: 0);
    led = !led;
    return false;
}

void blink_test(void) {
    /* configure gpio pin 12 as output */
    gpio_config_t config = {};
    config.intr_type = GPIO_INTR_DISABLE;
    config.pin_bit_mask = 1ULL << GPIO_NUM_12;
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;

    /* set gpio pin with config */
    ESP_ERROR_CHECK(gpio_config(&config));
    /* set 0 to start */
    gpio_set_level(GPIO_NUM_12, 0);

    /* create handle and config*/
    gptimer_handle_t blink_gptimer;
    gptimer_config_t blink_timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000, // 1MHz
    };
    /* create the timer */
    ESP_ERROR_CHECK(gptimer_new_timer(&blink_timer_config, &blink_gptimer));

    /* configure alarm */
    gptimer_alarm_config_t blink_alarm_config = {
        .alarm_count = 1 * 1000 * 1000,     // period of the alarm
        .reload_count = 0,                  // set counter value to this when event occurs
        .flags = {0}
    };
    blink_alarm_config.flags.auto_reload_on_alarm = true;
    // set action (bit amgbiguous naming)
    ESP_ERROR_CHECK(gptimer_set_alarm_action(blink_gptimer, &blink_alarm_config));

    gptimer_event_callbacks_t blink_callbacks = {
        .on_alarm = blink_callback
    };

    ESP_ERROR_CHECK(gptimer_register_event_callbacks(blink_gptimer, &blink_callbacks, NULL));
    ESP_ERROR_CHECK(gptimer_enable(blink_gptimer));
    ESP_ERROR_CHECK(gptimer_start(blink_gptimer));


    for (;;) {
        taskYIELD();
    }   

}

uint16_t frame_buffer[LCD_H_RES * LCD_V_RES];

void tft_display_test_write(esp_lcd_panel_handle_t handle) {
    ESP_LOGD(TAG, "Writing to display...");
    for (size_t row = 0; row < (size_t) LCD_V_RES; row += LCD_MAX_LINES) 
    {
        ESP_LOGD(TAG, "Writing row %zu...", row);
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
            handle, 
            0,
            row,
            LCD_H_RES,
            row + LCD_MAX_LINES,
            frame_buffer
        ));
    }
}

void tft_display_test(void) {
    ESP_LOGI(TAG, "Creating bus config...");
    // create an SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = MOSI,
        .miso_io_num = -1,
        .sclk_io_num = SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_MAX_TRANSFER
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Creating panel handle...");
    // create handle 
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = CS,
        .dc_gpio_num = DC,
        .spi_mode = 0,
        .pclk_hz = DOT_CLK_HZ,
        .trans_queue_depth = 10, // todo test performance
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_HOST, 
        &io_cfg,
        &io_handle
    ));

    ESP_LOGI(TAG, "Creating ST7789 panel...");
    // These are required by the ST7789 header file
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_cfg = {
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_RGB_BITS,
        .reset_gpio_num = RST,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &panel_handle));

    ESP_LOGI(TAG, "Encoding RGB...");
    uint16_t colour = 0;
    ESP_ERROR_CHECK(encode_rgb(255U, 0U, 64U, &colour)); 
    ESP_LOGD(TAG, "Colour is: %u", colour);

    ESP_LOGI(TAG, "Filling frame buffer...");
    for (size_t row = 0; row < (size_t) LCD_H_RES; row++) {
        for (size_t col = 0; col < (size_t) LCD_V_RES; col++) {
            frame_buffer[row * LCD_V_RES + col] = colour;
        }
    }

    ESP_LOGI(TAG, "Starting panel up...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Beginning write sequence...");
    for (;;) {
        
        tft_display_test_write(panel_handle);
        taskYIELD();
    }   

}

void spi_test(void) {
    ESP_LOGI(TAG, "Creating bus config...");
    // create an SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = MOSI,
        .miso_io_num = -1,
        .sclk_io_num = SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_MAX_TRANSFER,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Creating SPI handle...");
    spi_device_handle_t spidev_handle = NULL;
    spi_device_interface_config_t spidev_cfg = {
        .address_bits = 32,
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .clock_speed_hz = 20 * 1000 * 1000,
        .spics_io_num = CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(LCD_HOST, &spidev_cfg, &spidev_handle));

    ESP_LOGI(TAG, "Creating transaction...");
    const char *test = "SPI CAN YOU HEAR ME??\r\n";
    size_t len = strlen(test);
    spi_transaction_t trans = {
        .addr = (uint32_t) test,
        .length = len,
        .rx_buffer = NULL, // no MISO phase
    };
    ESP_LOGI(TAG, "Starting loop...");
    for (;;) {
        ESP_LOGI(TAG, "SPI transmitting...");
        
        spi_device_transmit(spidev_handle, &trans);
        taskYIELD();
    }
}

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
}

bool test_idle_hook_cb(void) {
    ESP_LOGI(TAG, "Idle task called");
    return true;
}

extern "C" void app_main(void) 
{
    esp_register_freertos_idle_hook_for_cpu(test_idle_hook_cb, 0);
    // ESP_LOGI(TAG, "Initialising UART...");
    // uart_test();
    // ESP_LOGI(TAG, "Beginning display test...");
    // tft_display_test();
    ESP_LOGI(TAG, "Beginning SPI test...");
    spi_test();
}