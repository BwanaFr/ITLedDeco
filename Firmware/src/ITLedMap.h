#pragma once

/**
 * The LED strip is arranged to form IT letters. Here is the LED strip layout
 *
 * ExxxxxxxxxxF       IxxxxxxxxxxJ
 *      D                  H
 *      x                  x
 *      x                  x
 *      C                  G
 * BxxxxxxxxxxA
 *
 * Where B-A == F-E == J-I  (called LETTER_WIDTH)
 * and D-C == H-G           (called LETTER_HEIGHT)
 */

#include "FastLED.h"

#define LETTER_WIDTH 20
#define LETTER_HEIGHT 25

//2D screen dimension
#define SCREEN_WIDTH 2*LETTER_WIDTH
#define SCREEN_HEIGHT (LETTER_HEIGHT + 2)

#define NB_STRIP_LEDS ((3*LETTER_WIDTH) + (2*LETTER_HEIGHT))

#define NO_LED NB_STRIP_LEDS

class IT{
public:
    IT() = delete;
    ~IT() = delete;
    static fl::u16 itUserMapFunc(fl::u16 x, fl::u16 y, fl::u16 width, fl::u16 height);
    static void toXY(fl::u16 index, fl::u16& x, fl::u16& y);
};

#include "ITLedMap.cpp.hpp"