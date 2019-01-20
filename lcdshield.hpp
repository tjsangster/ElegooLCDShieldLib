

#ifndef __LCD_SHEILD__
#define __LCD_SHEILD__

#include <stdint.h>

class ScanLine;

struct BusConfig {
    BusConfig(uint16_t const& color);
    uint8_t PORTH_BIG_BYTE;
    uint8_t PORTE_BIG_BYTE;
    uint8_t PORTG_BIG_BYTE; 
    uint8_t PORTH_LITTLE_BYTE; 
    uint8_t PORTE_LITTLE_BYTE;
    uint8_t PORTG_LITTLE_BYTE;
};

/* Singleton */
class  LCDShield {
    //Useful commands. Refer to ILI9341 data sheet. 
    static constexpr uint8_t COMMAND_MEMORY_WRITE              = 0x2C;
    static constexpr uint8_t COMMAND_COLUMN_ADDRESS_SET        = 0x2A;
    static constexpr uint8_t COMMAND_PAGE_ADDRESS_SET          = 0x2B;
    static constexpr uint8_t COMMAND_POWER_CONTROL_1           = 0xC0;
    static constexpr uint8_t COMMAND_POWER_CONTROL_2           = 0xC1;
    static constexpr uint8_t COMMAND_VCOM_CONTROL_1            = 0xC5;
    static constexpr uint8_t COMMAND_VCOM_CONTROL_2            = 0xC7;
    static constexpr uint8_t COMMAND_MEMORY_ACCESS_CONTROL     = 0x36;
    static constexpr uint8_t COMMAND_PIXEL_FORMAT_SET          = 0x3A;
    static constexpr uint8_t COMMAND_FRAME_CONTROL_NORMAL_MODE = 0xB1;
    static constexpr uint8_t COMMAND_DISPLAY_FUNCTION_CONTROL  = 0xB6;
    static constexpr uint8_t COMMAND_ENTER_SLEEP_MODE          = 0x10;
    static constexpr uint8_t COMMAND_SLEEP_OUT                 = 0x11;
    static constexpr uint8_t COMMAND_DISPAY_OFF                = 0x28;
    static constexpr uint8_t COMMAND_DISPAY_ON                 = 0x29;

public:
    static constexpr uint16_t SCREEN_WIDTH  = 240;
    static constexpr uint16_t SCREEN_HEIGHT = 320;
    static constexpr uint32_t PIXEL_COUNT   = static_cast<uint32_t>(SCREEN_WIDTH) * static_cast<uint32_t>(SCREEN_HEIGHT);
    static constexpr double   RECOMMENDED_DELAY_PER_FRAME = 70.0; //(ms)

    static LCDShield& getInstance();
    LCDShield(LCDShield const&)      = delete;
    void operator=(LCDShield const&) = delete;
    void clear();
    void fill_screen(uint16_t color);
    void draw_rectangle_filled(uint16_t x, uint16_t y, uint16_t l, uint16_t w, uint16_t color);
    void draw_square_filled(uint16_t x, uint16_t y, uint16_t s, uint16_t color);
    void draw_scanline(ScanLine const& scan, uint16_t start_x, uint16_t start_y, uint16_t color, bool reverse = false);
    
private:
    LCDShield();
    void write_to_bus(uint8_t const& data);
    void send_command(uint8_t command);
    void send_data(uint8_t data);
    void init_lcd_interface();
    uint32_t set_address(uint16_t  x1, uint16_t x2, uint16_t y1, uint16_t y2);
    void _draw_rectangle_filled(uint16_t x, uint16_t y, uint16_t l, uint16_t w, BusConfig const& busconfig);
};

//Base Class for objects that require large memory allocation. For now only scanlines. 
class MPObject {

protected:
    static constexpr uint16_t MEMORY_POOL_SIZE = 512;
    static constexpr uint8_t  MEMORY_NUM_OF_BLOCKS = 2;
    static constexpr uint16_t MEMORY_POOL_BLOCK_SIZE = MEMORY_POOL_SIZE / MEMORY_NUM_OF_BLOCKS;
    static constexpr uint16_t MEMORY_POOL_SINGLE_STEP = sizeof(uint16_t);
    static constexpr uint16_t MEMORY_POOL_DOUBLE_STEP = 2 * MEMORY_POOL_SINGLE_STEP;
    static constexpr uint16_t MEMORY_POOL_BLOCK_SIZE_ACTUAL = MEMORY_POOL_SINGLE_STEP * MEMORY_POOL_BLOCK_SIZE;

    static uint16_t* start_of_memory_pool;
    static uint16_t* next_block;
    static uint8_t free_blocks;
    static uint8_t initialized_blocks;

    uint16_t* start_of_allocated_memory = nullptr;

public:
    MPObject();
    ~MPObject();
    bool is_allocated();

private:
    void allocate();
    void deallocate();
    uint16_t* getAddressFromIndex(uint16_t index) const;
    uint16_t  getIndexFromAddress(uint16_t const *address) const;

};

//Object used to draw complex shapes one line at a time.
class ScanLine : public MPObject {

public:
    ScanLine();
    bool horizontal = false; //False for vertical scanlines
    uint16_t starting_pos_x = 0;
    uint16_t starting_pos_y = 0;
    uint16_t current_location = 0;
    void add_line(uint16_t offset, uint16_t length);
    void reset();

private:
    friend void LCDShield::draw_scanline(ScanLine const& scan, uint16_t start_x, uint16_t start_y, uint16_t color, bool reverse = false);
};

#endif 