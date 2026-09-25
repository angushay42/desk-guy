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
    ESP_ERROR_CHECK(bpp == 16);
    m_bpp = 16;
}

Display::~Display()
{
    if (m_fb != NULL)
        free(m_fb);
}

void Display::init() 
{
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
    
    m_io_cfg.dc_gpio_num = DC;
    m_io_cfg.cs_gpio_num = CS;
    m_io_cfg.pclk_hz = DOT_CLK_HZ;
    m_io_cfg.lcd_cmd_bits = LCD_CMD_BITS;
    m_io_cfg.lcd_param_bits = LCD_PARAM_BITS;
    m_io_cfg.spi_mode = 0;
    m_io_cfg.trans_queue_depth = 10; // todo test performance

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

    uint16_t *temp = (uint16_t *) createFrameBuffer();
    ESP_ERROR_CHECK(temp != NULL);

    fillScreen();   // default is 0 (black)

    ESP_LOGI(TAG, "Starting panel up...");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(m_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(m_panel_handle, true));

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(m_panel_handle, true));
}

void Display::setRotation(int rotations)
{

}

void *Display::createFrameBuffer()
{
    uint16_t *ret = (uint16_t *) calloc(m_screen_width * m_screen_height, m_bpp);
    if (ret != NULL)
        m_fb = ret;
    return ret;
}

void Display::sendScreen()
{
    ESP_LOGD(TAG, "Writing to display...");
    for (size_t row = 0; row < (size_t) m_screen_height; row += LCD_MAX_LINES) 
    {
        ESP_LOGD(TAG, "Writing row %zu...", row);
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(
            m_panel_handle, 
            0,
            row,
            m_screen_height,
            row + m_screen_width,
            m_fb
        ));
    }
}

void Display::fillScreen(uint16_t color) 
{
    ESP_LOGD(TAG, "Filling frame buffer...");
    for (size_t row = 0; row < (size_t) LCD_V_RES; row++) {
        for (size_t col = 0; col < (size_t) LCD_H_RES; col++) {
            m_fb[row * LCD_H_RES + col] = color;
        }
    }
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
    return nullptr;
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