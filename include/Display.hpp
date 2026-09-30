#pragma once

extern "C" {
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

#include "driver/spi_master.h"
#include "hal/lcd_types.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lcd_panel_ops.h"

#include "esp_freertos_hooks.h"
#include "esp_log.h"
}
#include <stddef.h>
#include <stdlib.h>

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

class Display {
public:
    Display(size_t w, size_t h, size_t bpp);
    ~Display();

    void init();
    
    void setRotation(uint8_t rotation);
    void pushImage(size_t x,size_t y,size_t w,size_t h,const uint16_t *data);
private:
    spi_bus_config_t m_spi_bus_config;
    esp_lcd_panel_io_handle_t m_io_handle;
    esp_lcd_panel_io_spi_config_t m_io_cfg;
    esp_lcd_panel_handle_t m_panel_handle;
    esp_lcd_panel_dev_config_t m_panel_cfg;

    uint16_t *m_fb;
    size_t m_screen_width;
    size_t m_screen_height;
    size_t m_bpp;
    uint8_t m_rotation; // 0-3, clockwise from north
};

/// @brief Adapted from TFT_eSPI by Bodmer
class Sprite {
public: 
    Sprite(Display *disp);
    ~Sprite() { deleteSprite(); };

    void setColorDepth(uint8_t);
    void drawPixel(int32_t x, int32_t y, uint32_t color);
    void createSprite(size_t w, size_t h);
    void deleteSprite();
    void fillSprite(uint16_t color);
    void pushSprite(uint32_t x, uint32_t y);

    void drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color);

    void fillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color);
    //                |      corner 1      |       corner 2      |        corner 3      |
    void fillTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color);
   
private:    
    void fillRectCorner(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, int32_t delta, uint32_t color);

    Display *m_disp;
protected:
    /* TFT_eSprite is more complex as it supports multiple bpp formats */

    uint8_t  m_bpp;     // bits per pixel (1, 4, 8 or 16)
    uint16_t *m_img;    // pointer to 16-bit sprite

    int32_t  m_sinra;   // Sine of rotation angle in fixed point
    int32_t  m_cosra;   // Cosine of rotation angle in fixed point

    bool     m_created; // A Sprite has been created and memory reserved
    bool     m_gFont = false; 


    int32_t  _iwidth, _iheight; // Sprite memory image bit width and height (swapped during rotations)
    int32_t  _dwidth, _dheight; // Real sprite width and height (for <8bpp Sprites)
    int32_t  _bitwidth;         // Sprite image bit width for drawPixel (for <8bpp Sprites, not swapped)
};