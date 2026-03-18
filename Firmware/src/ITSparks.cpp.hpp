#include <FastLED.h>
#include "ITSparks.h"
#include "ITLedMap.h"

#define BLUR_AMOUNT 2

ITSparks::ITSparks(const fl::XYMap &xymap) : fl::Fx2d(xymap), nbSparks_{10}, rate_{100}
{
}


void ITSparks::draw(fl::Fx::DrawContext context)
{
    if((context.now - lastGen_) >= rate_){
        for(int i=0;i<nbSparks_;++i){
            fl::u16 index = random(NB_STRIP_LEDS);
            fl::u16 x = 0;
            fl::u16 y = 0;
            IT::toXY(index, x, y);
            context.leds[mXyMap(x,y)] = CRGB::White;
            context.leds[mXyMap(x,y)].fadeLightBy(random(64));
            // blur2d(context.leds, getWidth(), getHeight(), BLUR_AMOUNT, getXYMap());
        }
        lastGen_ = context.now;
    }
    fadeToBlackBy(context.leds, mNumLeds, 20);
    for(int i=0;i<mNumLeds;++i){
        context.leds[i].addToRGB(8);
    }
}

fl::string ITSparks::fxName() const
{
    return "ITSparks";
}