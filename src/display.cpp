#include <Display.hpp>

static const char *TAG = "Display";

static bool on_color_done(
    esp_lcd_panel_io_handle_t io,
    esp_lcd_panel_io_event_data_t *ev, 
    void *ctx
)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR((SemaphoreHandle_t)ctx, &woken);
    return woken == pdTRUE;
}

Display::Display(size_t w, size_t h, size_t bpp) : 
    m_spi_bus_config{0},
    m_io_handle{NULL},
    m_io_cfg{},
    m_panel_handle{NULL},
    m_panel_cfg{},
    m_fb{nullptr},
    m_screen_width{w},
    m_screen_height{h}
{ 
    ESP_ERROR_CHECK(bpp != 16);
    m_bpp = 16;
}

Display::~Display()
{
    if (m_fb != NULL)
        free(m_fb);
}

void Display::init() 
{
    ESP_LOGD(TAG, "init called");
    ESP_LOGI(TAG, "Creating bus config...");
    
    // create an SPI bus
    m_spi_bus_config.sclk_io_num = SCLK;
    m_spi_bus_config.mosi_io_num = MOSI;
    m_spi_bus_config.miso_io_num = -1;
    m_spi_bus_config.quadhd_io_num = -1;
    m_spi_bus_config.quadwp_io_num = -1;
    m_spi_bus_config.max_transfer_sz = LCD_MAX_TRANSFER;

    ESP_ERROR_CHECK(
        spi_bus_initialize(LCD_HOST, &m_spi_bus_config, SPI_DMA_CH_AUTO)
    );

    ESP_LOGI(TAG, "Creating panel handle...");
    
    m_done_sem = xSemaphoreCreateBinary();
    m_io_cfg.dc_gpio_num = DC;
    m_io_cfg.cs_gpio_num = CS;
    m_io_cfg.pclk_hz = DOT_CLK_HZ;
    m_io_cfg.lcd_cmd_bits = LCD_CMD_BITS;
    m_io_cfg.lcd_param_bits = LCD_PARAM_BITS;
    m_io_cfg.spi_mode = 0;
    m_io_cfg.trans_queue_depth = 10; // todo test performance

    m_io_cfg.on_color_trans_done = on_color_done;
    m_io_cfg.user_ctx = (void *) m_done_sem;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(
        (esp_lcd_spi_bus_handle_t)LCD_HOST, 
        &m_io_cfg,
        &m_io_handle
    ));

    ESP_LOGI(TAG, "Creating ST7789 panel...");
    // These are required by the ST7789 header file
    m_panel_cfg.reset_gpio_num = RST;
    m_panel_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    m_panel_cfg.bits_per_pixel = LCD_RGB_BITS;
    m_panel_cfg.data_endian = LCD_RGB_DATA_ENDIAN_LITTLE;

    ESP_ERROR_CHECK(
        esp_lcd_new_panel_st7789(m_io_handle, &m_panel_cfg, &m_panel_handle)
    );



    ESP_LOGI(TAG, "Starting panel up...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(m_panel_handle, true));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(m_panel_handle, true));
}

void Display::setRotation(uint8_t rotation)
{
    if (rotation > 3)
        return;
    // todo     
    m_rotation = rotation;
}


void Display::pushImage(
    size_t x,
    size_t y,
    size_t w,
    size_t h,
    const uint16_t *data
)
{
    if (!m_panel_handle || !data) {
        ESP_LOGE(TAG, "Invalid panel handle or data");
        return;
    }
    if (x + w > m_screen_width || y + h > m_screen_height) {
        ESP_LOGW(TAG, "Image out of bounds");
        return;
    }
    
    ESP_LOGD(TAG, "Writing to display...");

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
        m_panel_handle, 
        x, y,
        x + w, y + h,
        data
    ));

    // wait until transfer is done
    if (xSemaphoreTake(m_done_sem, pdMS_TO_TICKS(200)) != pdTRUE)
        ESP_LOGE(TAG, "LCD transfer timed out");
}


Sprite::Sprite(Display *disp) :
    m_disp{disp},
    m_bpp{16},
    m_img{nullptr},
    m_created{false}
{ }



void Sprite::setColorDepth(uint8_t b)
{
    if (b != 16)
        return; // silent failure, unsupported
    m_bpp = b;
}

void Sprite::drawPixel(int32_t x, int32_t y, uint32_t color)
{
    if (!m_created) 
        return;
}


void Sprite::createSprite(size_t w, size_t h)
{
    if (m_created) 
        return;

    uint16_t *temp = (uint16_t*) calloc(w * h, m_bpp / 8);
    ESP_ERROR_CHECK(temp == NULL); 

    m_img = temp;
    m_created = true;
    _iwidth = w;
    _iheight = h;
}

void Sprite::deleteSprite()
{
    if (!m_created) 
        return;

    free(m_img);
    m_created = false;
    m_img = nullptr;
}

void Sprite::fillSprite(uint16_t color)
{
    if (!m_created) 
        return;

    ESP_LOGD(TAG, "FillSprite called");
    for (size_t i = 0; i < _iheight; i++)
        drawFastHLine(0, i, _iwidth, color);
}

void Sprite::pushSprite(uint32_t x, uint32_t y)
{
    if (!m_created) 
        return;
    
    m_disp->pushImage(x, y, _iwidth, _iheight, m_img);
}

void Sprite::fillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (!m_created) 
        return;

    for (uint32_t row = y; row < y + h; row++)
        drawFastHLine(x, row, w, color);
}

void Sprite::fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t r, uint32_t color)
{
    if (!m_created)
        return;

    fillRect(x, y+r, w, h - r - r, color);

    fillRectCorner(x+r, y+h-r-1,    r, 1, w-r-r-1, color);
    fillRectCorner(x+r, y+r,        r, 2, w-r-r-1, color);
}

void Sprite::drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color)
{
    // clipping
    if (!m_created || y < 0 || y >= (int32_t)_iheight) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > (int32_t)_iwidth) w = _iwidth - x;
    if (w < 1) return;

    while (w--) 
    {
        m_img[_iwidth * y + x++] = color;
    }
}

void Sprite::fillRectCorner(int32_t x0, int32_t y0, int32_t r, uint8_t cornername, int32_t delta, uint32_t color)
{
    if (cornername > 2)
        return; // silent failure

    int32_t f = 1 - r;
    int32_t ddF_x = 1;
    int32_t ddF_y = -r - r;
    int32_t y     = 0;

    delta++;

    while (y < r) {
        if (f >= 0) {
        drawFastHLine(
            x0 - y,
            y0 + ((cornername & 0x1) ? r: -r),
            y + y + delta,
            color 
        );
            r--;
            ddF_y += 2;
            f     += ddF_y;
        }

        y++;
        ddF_x += 2;
        f     += ddF_x;
        drawFastHLine(
            x0 - r,
            y0 + ((cornername & 0x1) ? y: -y),
            r + r + delta,
            color 
        );
    }
}

//                |      corner 1      |       corner 2      |        corner 3      |
void Sprite::fillTriangle(int32_t x0,int32_t y0, int32_t x1,int32_t y1, int32_t x2,int32_t y2, uint32_t color)
{
    if (!m_created) 
        return;
    int32_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        transpose(y0, y1); transpose(x0, x1);
    }
    if (y1 > y2) {
        transpose(y2, y1); transpose(x2, x1);
    }
    if (y0 > y1) {
        transpose(y0, y1); transpose(x0, x1);
    }

    if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if (x1 < a)      a = x1;
        else if (x1 > b) b = x1;
        if (x2 < a)      a = x2;
        else if (x2 > b) b = x2;
        drawFastHLine(a, y0, b - a + 1, color);
        return;
    }

    int32_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1,
    sa   = 0,
    sb   = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2) last = y1;  // Include y1 scanline
    else         last = y1 - 1; // Skip it

    for (y = y0; y <= last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;

        if (a > b) transpose(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;

        if (a > b) transpose(a, b);
        drawFastHLine(a, y, b - a + 1, color);
    }

}

void scale_rgb(uint8_t &src, size_t size) {
    if (size < 5 || size > 6) return ;
    volatile double r_max, t_max, temp;
    r_max = (double) ((1 << size) - 1);
    t_max = 255.0;

    temp = round(((double) src * r_max) / t_max);

    src = temp;
}

// todo expand this
/* always returns a 5-6-5 rgb into a uint16_t */
void encode_rgb(uint8_t r, uint8_t g, uint8_t b, uint16_t &rgb) {
    rgb = 0;
    scale_rgb(r, 5U);
    scale_rgb(g, 6U);
    scale_rgb(b, 5U);
    rgb |= r << 11;
    rgb |= g << 5;
    rgb |= b;
}