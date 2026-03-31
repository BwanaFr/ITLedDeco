#pragma once
#include "ITLedMap.h"

fl::u16 IT::itUserMapFunc(fl::u16 x, fl::u16 y, fl::u16 width, fl::u16 height)
{
    if (y == 0) {
        if (x < LETTER_WIDTH) {
            // Bottom of the I
            return x;
        }
    } else if (y == LETTER_HEIGHT + 1) {
        // Top row
        if (x < LETTER_WIDTH) {
            return LETTER_WIDTH + LETTER_HEIGHT + x;
        } else if (x < LETTER_WIDTH * 2) {
            return LETTER_HEIGHT * 2 + LETTER_WIDTH + x;
        }
    } else if (x == LETTER_WIDTH / 2) {
        if (y <= LETTER_HEIGHT) {
            return LETTER_WIDTH + y - 1;
        }
    } else if (x == LETTER_WIDTH + LETTER_WIDTH / 2) {
        if (y <= LETTER_HEIGHT) {
            return LETTER_WIDTH * 2 + LETTER_HEIGHT + y - 1;
        }
    }
    return NO_LED;
}

void IT::toXY(fl::u16 index, fl::u16& x, fl::u16& y)
{
    if(index < LETTER_WIDTH){
        y = 0;
        x = index;
    }else if(index < (LETTER_WIDTH + LETTER_HEIGHT)){
        x = LETTER_WIDTH / 2;
        y = index - LETTER_WIDTH + 1;
    }else if(index < (LETTER_WIDTH *2 + LETTER_HEIGHT)){
        y = LETTER_HEIGHT + 1;
        x = index - LETTER_WIDTH - LETTER_HEIGHT;
    }else if(index < (LETTER_WIDTH *2 + LETTER_HEIGHT*2)){
        y = index - (LETTER_WIDTH *2 + LETTER_HEIGHT) + 1;
        x = LETTER_WIDTH + LETTER_WIDTH / 2;
    }else{
        y = LETTER_HEIGHT + 1;
        x = index - 2*LETTER_HEIGHT - LETTER_WIDTH;
    }
}