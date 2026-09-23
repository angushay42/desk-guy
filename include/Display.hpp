#pragma once

#include <stddef.h>

class Display {
public:
    Display(size_t w, size_t h);
    ~Display();

    void init();


    void setRotation(int rotations);
};

class Sprite {
public: 
    Sprite(Display &disp);
    void setColorDepth(uint8_t);
    void createSprite(size_t w, size_t h);
    void deleteSprite();
    void fillSprite(uint16_t color);
    void pushSprite(uint32_t x, uint32_t y);
    void fillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
    void fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color);
    //                |      corner 1      |       corner 2      |        corner 3      |
    void fillTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color);
};