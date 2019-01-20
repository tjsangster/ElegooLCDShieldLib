
#include "lcdshield.hpp"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

//Begin MPObject

//static variables
uint16_t* MPObject::start_of_memory_pool = nullptr;
uint16_t* MPObject::next_block = nullptr;
uint8_t MPObject::free_blocks = MEMORY_NUM_OF_BLOCKS;
uint8_t MPObject::initialized_blocks = 0;

MPObject::MPObject() {
    //If memory pool is not allocated.
    if(!start_of_memory_pool) {
        start_of_memory_pool = static_cast<uint16_t*>(malloc(sizeof(uint16_t)*MEMORY_POOL_SIZE));
        next_block = start_of_memory_pool;
    }

    //Claim block of memory from memory pool
    this->allocate();
}

MPObject::~MPObject() {
    this->deallocate();
}

void MPObject::allocate() {

    if(is_allocated())
        return;

    //Lazy init memory pool blocks
    if(initialized_blocks < MEMORY_NUM_OF_BLOCKS) {
        uint16_t* new_block = getAddressFromIndex(initialized_blocks);
        ++initialized_blocks;
        *new_block = initialized_blocks;
    }

    //If a free block exists remove from head of list and assign to object.
    if(free_blocks > 0) {
        start_of_allocated_memory = next_block;
        --free_blocks;

        if(free_blocks > 0)
            next_block = getAddressFromIndex(*next_block);
        else
            next_block = nullptr;
    }

}

void MPObject::deallocate() {

    if(!is_allocated())
        return;

    //If next_block is not null add to head of list 
    if(!next_block) {
        *start_of_allocated_memory = getIndexFromAddress(next_block);
        next_block = start_of_allocated_memory;
    } else {
        *start_of_allocated_memory = MEMORY_NUM_OF_BLOCKS;
        next_block = start_of_allocated_memory;
    }

    ++free_blocks;
}

uint16_t* MPObject::getAddressFromIndex(uint16_t index) const {
    return start_of_memory_pool + (index * MEMORY_POOL_BLOCK_SIZE_ACTUAL);
}

uint16_t  MPObject::getIndexFromAddress(uint16_t const *address) const {
    return static_cast<uint16_t>(address - start_of_memory_pool) / MEMORY_POOL_BLOCK_SIZE_ACTUAL;
}

bool MPObject::is_allocated() {
    return start_of_allocated_memory != nullptr;
}

//Begin ScanLine 

ScanLine::ScanLine() {}

void ScanLine::add_line(uint16_t offset, uint16_t length) {
    if(!is_allocated())
        return;
    if(current_location > MEMORY_POOL_BLOCK_SIZE_ACTUAL)
        return;

    *(start_of_allocated_memory + current_location) = offset;
    current_location += MEMORY_POOL_SINGLE_STEP;
    *(start_of_allocated_memory + current_location) = length;
    current_location += MEMORY_POOL_SINGLE_STEP;
}

void ScanLine::reset() {
    if(!is_allocated())
        return;

    current_location = 0;
}

//Begin LCDShield

LCDShield::LCDShield() {
    this->init_lcd_interface();
}

LCDShield& LCDShield::getInstance() {
    static LCDShield lcd_instance;
    return lcd_instance;
}

void LCDShield::write_to_bus(uint8_t const& data) {
    //set pins
    PORTH &= ~(0x78);
    PORTH |= ((data & 0xC0) >> 3) | ((data & 0x03) << 5);
    PORTE &= ~(0x38);
    PORTE |= ((data & 0x0C) << 2) | ((data & 0x20) >> 2);
    PORTG &= ~(0x20);
    PORTG |= (data & 0x10) << 1;
    
    //write signal
    PORTF &= ~(0x02);
    PORTF |= 0x02;
}

void LCDShield::send_command(uint8_t command) {
    PORTF &= ~(0x04);
    write_to_bus(command);
}

void LCDShield::send_data(uint8_t data) {
    PORTF |= (0x04);
    write_to_bus(data);
}

void LCDShield::init_lcd_interface() {

    //set pins for output
    DDRH |= 0x78;
    DDRE |= 0x38;
    DDRG |= 0x20;
    DDRF |= 0x1F;

    //set selected control pins to high
    PORTF |= 0x1F;

    //reset shield
    _delay_ms(5.0);
    PORTF &= ~(1<<PORTF4);
    _delay_ms(15.0);
    PORTF |= (1<<PORTF4);
    _delay_ms(15.0);

    PORTF &= ~(1<<PORTF3);

    send_command(COMMAND_POWER_CONTROL_1);    
    send_data(0x23);
 
    send_command(COMMAND_POWER_CONTROL_1);     
    send_data(0x10);

    send_command(COMMAND_VCOM_CONTROL_1);    
    send_data(0x3e);
    send_data(0x28);
 
    send_command(COMMAND_VCOM_CONTROL_2);    
    send_data(0x86);
 
    send_command(COMMAND_MEMORY_ACCESS_CONTROL);    
    send_data(0x48);

    send_command(COMMAND_PIXEL_FORMAT_SET);    
    send_data(0x55);

    send_command(COMMAND_FRAME_CONTROL_NORMAL_MODE);    
    send_data(0x00);
    send_data(0x18);
 
    send_command(COMMAND_DISPLAY_FUNCTION_CONTROL);
    send_data(0x08);
    send_data(0x82);
    send_data(0x27);  

    send_command(COMMAND_SLEEP_OUT);    //Exit Sleep 
    _delay_ms(120.0); 
				
    send_command(COMMAND_DISPAY_ON); 

    send_command(COMMAND_MEMORY_WRITE);

    PORTF |= (1<<PORTF3);

}

void LCDShield::clear() {
    uint32_t i;

    PORTF &= ~(1<<PORTF3);

    send_command(COMMAND_MEMORY_WRITE);
    set_address(0,SCREEN_WIDTH,0,SCREEN_HEIGHT);

    PORTH &= ~(0x78);
    PORTE &= ~(0x38);
    PORTG &= ~(0x20);

    PORTF |= (0x04);
    for(i=0;i<PIXEL_COUNT;++i) {
        //write signal
        PORTF &= ~(0x2);
        PORTF |= 0x02;
        PORTF &= ~(0x2);
        PORTF |= 0x02;
    }

    PORTF |= (1<<PORTF3);
}

void LCDShield::fill_screen(uint16_t color) {
    uint32_t i;

    cli();

    //Local variables are noticeably faster here. 
    const uint8_t BIG_BYTE = color >> 8;
    const uint8_t LITTLE_BYTE = color;
    const uint8_t PORTH_BIG_BYTE = (PORTH & ~(0x78)) | ((BIG_BYTE & 0xC0) >> 3) | ((BIG_BYTE & 0x3) << 5);
    const uint8_t PORTE_BIG_BYTE = (PORTE & ~(0x38)) | ((BIG_BYTE & 0xC) << 2) | ((BIG_BYTE & 0x20) >> 2);
    const uint8_t PORTG_BIG_BYTE = (PORTG & ~(0x20)) | (BIG_BYTE & 0x10) << 1; 
    const uint8_t PORTH_LITTLE_BYTE = (PORTH & ~(0x78)) | ((LITTLE_BYTE & 0xC0) >> 3) | ((LITTLE_BYTE & 0x3) << 5);
    const uint8_t PORTE_LITTLE_BYTE = (PORTE & ~(0x38)) | ((LITTLE_BYTE & 0xC) << 2) | ((LITTLE_BYTE & 0x20) >> 2);
    const uint8_t PORTG_LITTLE_BYTE = (PORTG & ~(0x20)) | (LITTLE_BYTE & 0x10) << 1;
    
    PORTF &= ~(1<<PORTF3);

    send_command(COMMAND_MEMORY_WRITE);
    set_address(0,SCREEN_WIDTH,0,SCREEN_HEIGHT);

    PORTF |= (0x04);
    for(i=0;i<PIXEL_COUNT;++i) {
        //set big byte
        PORTH = PORTH_BIG_BYTE;
        PORTE = PORTE_BIG_BYTE;
        PORTG = PORTG_BIG_BYTE;
        //write signal
        PORTF &= ~(0x2);
        PORTF |= 0x02;
        //set little byte
        PORTH = PORTH_LITTLE_BYTE;
        PORTE = PORTE_LITTLE_BYTE;
        PORTG = PORTG_LITTLE_BYTE;
        //write signal
        PORTF &= ~(0x2);
        PORTF |= 0x02;
    }

    sei();

    PORTF |= (1<<PORTF3);
}

void LCDShield::_draw_rectangle_filled(uint16_t x, uint16_t y, uint16_t l, uint16_t w, BusConfig const& busconfig) {
    uint32_t i;

    PORTF &= ~(1<<PORTF3);
 
    send_command(COMMAND_MEMORY_WRITE);
    const auto pixel_count = set_address(x,x+w,y,y+l);
    
    PORTF |= (0x04);
    for(i=0;i<pixel_count;++i) {
        //set big byte
        PORTH = busconfig.PORTH_BIG_BYTE;
        PORTE = busconfig.PORTE_BIG_BYTE;
        PORTG = busconfig.PORTG_BIG_BYTE;
        //write signal
        PORTF &= ~(0x2);
        PORTF |= 0x02;
        //set little byte
        PORTH = busconfig.PORTH_LITTLE_BYTE;
        PORTE = busconfig.PORTE_LITTLE_BYTE;
        PORTG = busconfig.PORTG_LITTLE_BYTE;
        //write signal
        PORTF &= ~(0x2);
        PORTF |= 0x02;
    }

    PORTF |= (1<<PORTF3);
}

void LCDShield::draw_rectangle_filled(uint16_t x, uint16_t y, uint16_t l, uint16_t w, uint16_t color) {
    cli();
    const BusConfig config(color);
    _draw_rectangle_filled(x, y, l, w, config);
    sei();
}


void LCDShield::draw_square_filled(uint16_t x, uint16_t y, uint16_t s, uint16_t color) {
    cli();
    const BusConfig config(color);
    _draw_rectangle_filled(x, y, s, s, config);
    sei();
}

void LCDShield::draw_scanline(ScanLine const& scan, uint16_t start_x, uint16_t start_y, uint16_t color, bool reverse) {
    
    PORTF &= ~(1<<PORTF3);

    send_command(COMMAND_MEMORY_WRITE);

    cli();
    
    const BusConfig config(color);

    uint16_t *i;
    uint16_t j;
    uint16_t *length = nullptr;
    uint16_t *i_stop = scan.start_of_allocated_memory + scan.current_location;
    uint16_t current_line = !reverse ? 0:scan.current_location / scan.MEMORY_POOL_DOUBLE_STEP;
    uint16_t x_start, y_start;

    for(i = scan.start_of_allocated_memory; i < i_stop; i += scan.MEMORY_POOL_DOUBLE_STEP) {
        
        length = i + scan.MEMORY_POOL_SINGLE_STEP;
        
        if(scan.horizontal) {
            x_start = start_x + *i;
            y_start = start_y + current_line;
            set_address(x_start, x_start + *length, y_start, y_start);
        }
        else {
            x_start = start_x + current_line;
            y_start = start_y + *i;
            set_address(x_start, x_start, y_start, y_start + *length);
        }

        current_line += !reverse ? 1:-1;

        PORTF |= (0x04);

        for(j=0;j<*length;++j) {
            //set big byte
            PORTH = config.PORTH_BIG_BYTE;
            PORTE = config.PORTE_BIG_BYTE;
            PORTG = config.PORTG_BIG_BYTE;
            //write signal
            PORTF &= ~(0x2);
            PORTF |= 0x02;
            //set little byte
            PORTH = config.PORTH_LITTLE_BYTE;
            PORTE = config.PORTE_LITTLE_BYTE;
            PORTG = config.PORTG_LITTLE_BYTE;
            //write signal
            PORTF &= ~(0x2);
            PORTF |= 0x02;
        }
    }

    sei();

    PORTF |= (1<<PORTF3);

}

//Sets address and returns pixel count in area
uint32_t LCDShield::set_address(
            uint16_t x1, 
            uint16_t x2,
            uint16_t y1, 
            uint16_t y2
        ) {

    send_command(COMMAND_COLUMN_ADDRESS_SET);
    send_data(x1>>8);
    send_data(x1);
    send_data(x2>>8);
    send_data(x2);
    send_command(COMMAND_PAGE_ADDRESS_SET);
    send_data(y1>>8);
    send_data(y1);
    send_data(y2>>8);
    send_data(y2);
    send_command(COMMAND_MEMORY_WRITE);
        
    return static_cast<uint32_t>(x2-x1) * static_cast<uint32_t>(y2-y1); 
}

//BEGIN BusConfig

BusConfig::BusConfig(uint16_t const& color) {
    const uint8_t BIG_BYTE = color >> 8;
    const uint8_t LITTLE_BYTE = color;
    PORTH_BIG_BYTE = (PORTH & ~(0x78)) | ((BIG_BYTE & 0xC0) >> 3) | ((BIG_BYTE & 0x3) << 5);
    PORTE_BIG_BYTE = (PORTE & ~(0x38)) | ((BIG_BYTE & 0xC) << 2) | ((BIG_BYTE & 0x20) >> 2);
    PORTG_BIG_BYTE = (PORTG & ~(0x20)) | (BIG_BYTE & 0x10) << 1; 
    PORTH_LITTLE_BYTE = (PORTH & ~(0x78)) | ((LITTLE_BYTE & 0xC0) >> 3) | ((LITTLE_BYTE & 0x3) << 5);
    PORTE_LITTLE_BYTE = (PORTE & ~(0x38)) | ((LITTLE_BYTE & 0xC) << 2) | ((LITTLE_BYTE & 0x20) >> 2);
    PORTG_LITTLE_BYTE = (PORTG & ~(0x20)) | (LITTLE_BYTE & 0x10) << 1;
}