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
    _disp = disp;

    _iwidth = 0;
    _iheight = 0;
    _bpp = 16;

    _created = false;

    _xs = 0;
    _ys = 0;
    _xe = 0;
    _ye = 0;

    // pushColor coordinates?
    _xptr = 0;
    _yptr = 0;

    _colorMap = nullptr;
}


void Sprite::setColorDepth(uint8_t b)
{
  // Do not re-create the sprite if the colour depth does not change
  if (_bpp == b) return _img8_1;

  // Validate the new colour depth
  if ( b > 8 ) _bpp = 16;  // Bytes per pixel
  else if ( b > 4 ) _bpp = 8;
  else if ( b > 1 ) _bpp = 4;
  else _bpp = 1;

  // Can't change an existing sprite's colour depth so delete and create a new one
  if (_created) {
    deleteSprite();
    return createSprite(_dwidth, _dheight);
  }

  return nullptr;
}

void Sprite::drawPixel(int32_t x, int32_t y, uint32_t color)
{
  if (!_created || _vpOoB) return;

  x+= _xDatum;
  y+= _yDatum;

  // Range checking
  if ((x < _vpX) || (y < _vpY) ||(x >= _vpW) || (y >= _vpH)) return;

  if (_bpp == 16)
  {
    color = (color >> 8) | (color << 8);
    _img[x+y*_iwidth] = (uint16_t) color;
  }
  else if (_bpp == 8)
  {
    _img8[x+y*_iwidth] = (uint8_t)((color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3);
  }
  else if (_bpp == 4)
  {
    uint8_t c = color & 0x0F;
    int index = (x+y*_iwidth)>>1;;
    if ((x & 0x01) == 0) {
      _img4[index] = (uint8_t)((c << 4) | (_img4[index] & 0x0F));
    }
    else {
      _img4[index] =  (uint8_t)(c | (_img4[index] & 0xF0));
    }
  }
  else // 1 bpp
  {
    if (rotation == 1)
    {
      uint16_t tx = x;
      x = _dwidth - y - 1;
      y = tx;
    }
    else if (rotation == 2)
    {
      x = _dwidth - x - 1;
      y = _dheight - y - 1;
    }
    else if (rotation == 3)
    {
      uint16_t tx = x;
      x = y;
      y = _dheight - tx - 1;
    }

    if (color) _img8[(x + y * _bitwidth)>>3] |=  (0x80 >> (x & 0x7));
    else       _img8[(x + y * _bitwidth)>>3] &= ~(0x80 >> (x & 0x7));
  }
}


void *Sprite::createSprite(size_t w, size_t h)
{
    if ( _created ) 
        return _img8_1;

    if ( w < 1 || h < 1 ) 
        return nullptr;

    _iwidth  = _dwidth  = _bitwidth = w;
    _iheight = _dheight = h;

    cursor_x = 0;
    cursor_y = 0;

    // Default scroll rectangle and gap fill colour
    _sx = 0;
    _sy = 0;
    _sw = w;
    _sh = h;
    _scolor = TFT_BLACK;

    _img8   = (uint8_t*) callocSprite(w, h, frames);
    _img8_1 = _img8;
    _img8_2 = _img8;
    _img    = (uint16_t*) _img8;
    _img4   = _img8;

    if ( (_bpp == 16) && (frames > 1) ) 
    {
        _img8_2 = _img8 + (w * h * 2 + 1);
    }

    if ( (_bpp == 8) && (frames > 1) ) 
    {
        _img8_2 = _img8 + (w * h + 1);
    }

    // This is to make it clear what pointer size is expected to be used
    // but casting in the user sketch is needed due to the use of void*
    if ( (_bpp == 1) && (frames > 1) )
    {
        w = (w+7) & 0xFFF8;
        _img8_2 = _img8 + ( (w>>3) * h + 1 );
    }

    if (_img8)
    {
        _created = true;
        if ( (_bpp == 4) && (_colorMap == nullptr)) createPalette(default_4bit_palette);

        rotation = 0;
        setViewport(0, 0, _dwidth, _dheight);
        setPivot(_iwidth/2, _iheight/2);
        return _img8_1;
    }

    return nullptr;
}
void Sprite::deleteSprite()
{
  if (_colorMap != nullptr)
  {
    free(_colorMap);
    _colorMap = nullptr;
  }

  if (_created)
  {
    free(_img8_1);
    _img8 = nullptr;
    _created = false;
    _vpOoB   = true;  // TFT_eSPI class write() uses this to check for valid sprite
  }
}

void Sprite::fillSprite(uint16_t color)
{

}
void Sprite::pushSprite(uint32_t x, uint32_t y)
{

}
void Sprite::fillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (!_created || _vpOoB) return;

    x+= _xDatum;
    y+= _yDatum;

    // Clipping
    if ((x >= _vpW) || (y >= _vpH)) return;

    if (x < _vpX) { w += x - _vpX; x = _vpX; }
    if (y < _vpY) { h += y - _vpY; y = _vpY; }

    if ((x + w) > _vpW) w = _vpW - x;
    if ((y + h) > _vpH) h = _vpH - y;

    if ((w < 1) || (h < 1)) return;

    int32_t yp = _iwidth * y + x;

    if (_bpp == 16)
    {
        color = (color >> 8) | (color << 8);
        uint32_t iw = w;
        int32_t ys = yp;
        if(h--)  {while (iw--) _img[yp++] = (uint16_t) color;}
        yp = ys;
        while (h--)
        {
        yp += _iwidth;
        memcpy( _img+yp, _img+ys, w<<1);
        }
    }
    else if (_bpp == 8)
    {
        color = (color & 0xE000)>>8 | (color & 0x0700)>>6 | (color & 0x0018)>>3;
        while (h--)
        {
        memset(_img8 + yp, (uint8_t)color, w);
        yp += _iwidth;
        }
    }
    else if (_bpp == 4)
    {
        uint8_t c1 = (uint8_t)color & 0x0F;
        uint8_t c2 = c1 | ((c1 << 4) & 0xF0);
        if ((x & 0x01) == 0 && (w & 0x01) == 0)
        {
        yp = (yp >> 1);
        while (h--)
        {
            memset(_img4 + yp, c2, (w>>1));
            yp += (_iwidth >> 1);
        }
        }
        else if ((x & 0x01) == 0)
        {

        // same as above but you have a hangover on the right.
        yp = (yp >> 1);
        while (h--)
        {
            if (w > 1)
            memset(_img4 + yp, c2, (w-1)>>1);
            // handle the rightmost pixel by calling drawPixel
            drawPixel(x+w-1-_xDatum, y+h-_yDatum, c1);
            yp += (_iwidth >> 1);
        }
        }
        else if ((w & 0x01) == 1)
        {
        yp = (yp + 1) >> 1;
        while (h--) {
            drawPixel(x-_xDatum, y+h-_yDatum, color & 0x0F);
            if (w > 1)
            memset(_img4 + yp, c2, (w-1)>>1);
            // same as above but you have a hangover on the left instead
            yp += (_iwidth >> 1);
        }
        }
        else
        {
        yp = (yp + 1) >> 1;
        while (h--) {
            drawPixel(x-_xDatum, y+h-_yDatum, color & 0x0F);
            if (w > 1) drawPixel(x+w-1-_xDatum, y+h-_yDatum, color & 0x0F);
            if (w > 2)
            memset(_img4 + yp, c2, (w-2)>>1);
            // maximal hacking, single pixels on left and right.
            yp += (_iwidth >> 1);
        }
        }
    }
    else
    {
        x -= _xDatum;
        y -= _yDatum;
        while (h--)
        {
        int32_t ww = w;
        int32_t xx = x;
        while (ww--) drawPixel(xx++, y, color);
        y++;
        }
    }
}

void Sprite::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color)
{

}
//                |      corner 1      |       corner 2      |        corner 3      |
void Sprite::fillTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color)
{

}