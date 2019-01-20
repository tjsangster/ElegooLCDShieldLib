


#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <stdlib.h>
#include "lcdshield.hpp"

LCDShield* lcd;

int main(void) {

    srand(42);

    lcd = &LCDShield::getInstance();

    unsigned char i;
    const unsigned char max_circle_radius = LCDShield::SCREEN_WIDTH / 2;

    //set initial colors and declare positioning vars
    uint16_t color1 = 0x00FF;
    uint16_t color2 = 0xF000;
    uint16_t start_x1, start_x2, start_y1, start_y2;

    lcd->clear();

    while(true) {

        ScanLine sline1;
        ScanLine sline2;
        sline1.horizontal = true;
        sline2.horizontal = true;

        unsigned char circle_radius1 = rand() % max_circle_radius;
        unsigned circle_radius2 = rand() % max_circle_radius;

        segment_length = 0;
        for(i=0; i < circle_radius1; ++i) {
            segment_length = sqrt(square(circle_radius1) - square(i));
            sline1.add_line(circle_radius1 - segment_length, 2 * segment_length);
        } 

        for(i=0; i < circle_radius2; ++i) {
            segment_length = sqrt(square(circle_radius2) - square(i));
            sline2.add_line(circle_radius2 - segment_length, 2 * segment_length);
        }

        start_x1 = rand() % (LCDShield::SCREEN_WIDTH - 2 * circle_radius1);
        start_y1 = rand() % (LCDShield::SCREEN_HEIGHT - 2 * circle_radius1);
        start_x2 = rand() % (LCDShield::SCREEN_WIDTH - 2 * circle_radius2);
        start_y2 = rand() % (LCDShield::SCREEN_HEIGHT - 2 * circle_radius2);
        lcd->draw_scanline(sline1, start_x1, start_y1, color1, true);
        lcd->draw_scanline(sline1, start_x1, start_y1 + circle_radius1, color1, false);
        lcd->draw_scanline(sline2, start_x2, start_y2, color2, true);
        lcd->draw_scanline(sline2, start_x2, start_y2 + circle_radius2, color2, false);

        color1 += 0x0102;  
        color2 += 0x0021;
    }

}