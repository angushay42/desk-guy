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

#include <Display.hpp>

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

static const char *TAG = "Display";


Display::Display(size_t w, size_t h) 
{

}

Display::~Display()
{

}

void Display::init() 
{

}

void Display::setRotation(int rotations)
{

}

Sprite::Sprite(Display *disp)
{
    
}


void Sprite::setColorDepth(uint8_t b)
{
  
}

void Sprite::drawPixel(int32_t x, int32_t y, uint32_t color)
{
  
}


void *Sprite::createSprite(size_t w, size_t h)
{
    
}
void Sprite::deleteSprite()
{

}

void Sprite::fillSprite(uint16_t color)
{

}
void Sprite::pushSprite(uint32_t x, uint32_t y)
{

}
void Sprite::fillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    
}

void Sprite::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color)
{

}
//                |      corner 1      |       corner 2      |        corner 3      |
void Sprite::fillTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color)
{

}