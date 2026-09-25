#pragma once

#include <stddef.h>

class Display {
public:
    Display(size_t w, size_t h);
    ~Display();

    void init();

    void setRotation(int rotations);
};

/// @brief Adapted from TFT_eSPI by Bodmer
class Sprite {
public: 
    Sprite(Display *disp);
    void setColorDepth(uint8_t);
    void Sprite::drawPixel(int32_t x, int32_t y, uint32_t color);
    void *createSprite(size_t w, size_t h);
    void deleteSprite();
    void fillSprite(uint16_t color);
    void pushSprite(uint32_t x, uint32_t y);
    void fillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color);
    //                |      corner 1      |       corner 2      |        corner 3      |
    void fillTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color);
private:    
    Display *_disp;
protected:

    uint8_t  _bpp;     // bits per pixel (1, 4, 8 or 16)
    uint16_t *_img;    // pointer to 16-bit sprite
    uint8_t  *_img8;   // pointer to  1 and 8-bit sprite frame 1 or frame 2
    uint8_t  *_img4;   // pointer to  4-bit sprite (uses color map)
    uint8_t  *_img8_1; // pointer to frame 1
    uint8_t  *_img8_2; // pointer to frame 2

    uint16_t *_colorMap; // color map pointer: 16 entries, used with 4-bit color map.

    int32_t  _sinra;   // Sine of rotation angle in fixed point
    int32_t  _cosra;   // Cosine of rotation angle in fixed point

    bool     _created; // A Sprite has been created and memory reserved
    bool     _gFont = false; 

    int32_t  _xs, _ys, _xe, _ye, _xptr, _yptr; // for setWindow
    int32_t  _sx, _sy; // x,y for scroll zone
    uint32_t _sw, _sh; // w,h for scroll zone
    uint32_t _scolor;  // gap fill colour for scroll zone

    int32_t  _iwidth, _iheight; // Sprite memory image bit width and height (swapped during rotations)
    int32_t  _dwidth, _dheight; // Real sprite width and height (for <8bpp Sprites)
    int32_t  _bitwidth;         // Sprite image bit width for drawPixel (for <8bpp Sprites, not swapped)
};