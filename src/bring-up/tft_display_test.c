#include "driver/gpio.h"
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
#define DC      GPIO_NUM_4 // Data/command 
#define RST     GPIO_NUM_5

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

static const char *TAG = "TFT display test";

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

// todo expand this
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
        .sclk_io_num = SCLK,
        .mosi_io_num = MOSI,
        .miso_io_num = -1,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
        .max_transfer_sz = LCD_MAX_TRANSFER,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Creating panel handle...");
    // create handle 
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_cfg = {
        .dc_gpio_num = DC,
        .cs_gpio_num = CS,
        .pclk_hz = DOT_CLK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10 // todo test performance
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
        .reset_gpio_num = RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = LCD_RGB_BITS,
        .data_endian = LCD_RGB_DATA_ENDIAN_LITTLE
    };


    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_cfg, &panel_handle));

    // ESP_LOGI(TAG, "Encoding RGB...");
    // uint16_t colour = 0;
    // ESP_ERROR_CHECK(encode_rgb(0, 0, 0, &colour)); 
    // ESP_LOGD(TAG, "Colour is: %u", colour);

    uint16_t test_colour;

    ESP_ERROR_CHECK(encode_rgb(255,0,0, &test_colour));

    ESP_LOGD(TAG, "Filling frame buffer...");
    for (size_t row = 0; row < (size_t) LCD_V_RES; row++) {
        for (size_t col = 0; col < (size_t) LCD_H_RES; col++) {
            frame_buffer[row * LCD_H_RES + col] = test_colour;
        }
    }

    ESP_LOGI(TAG, "Starting panel up...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // ESP_LOGD(TAG, 
    //     "colour is: \t%u. \n
    //     First half: \t%u, \n
    //     Second half: \t%u \n", 
    //     colour, 
    //     *(uint8_t *) &colour,
    //     *((uint8_t *) &colour + 1)
    // );

    ESP_LOGI(TAG, "Beginning write sequence...");
    for (;;) {
        
        tft_display_test_write(panel_handle);
        taskYIELD();
    }   

}

void app_main(void) 
{
    ESP_LOGI(TAG, "Beginning test...");
    tft_display_test();
}