#include <FastLED.h>
#include "ITFill.h"
#include "ITLedMap.h"
#include <Arduino.h>

#define BLUR_AMOUNT 2

ITFill::ITFill(const fl::XYMap &xymap) : fl::Fx2d(xymap), rate_{50}, lastGen_{0}, level_{0}
{
    width_ = xymap.getWidth();
    height_ = xymap.getHeight();
}


void ITFill::draw(fl::Fx::DrawContext context)
{
    if((context.now - lastGen_) >= rate_){
        lastGen_ = context.now;
        int middle = getHeight()/2;
        int topValue = middle + (middle*level_ / (255)) + 1;
        int botValue = middle - (middle*level_ / (255)) - 1;
        for(int x=0;x<getWidth();++x){
            for(int y=middle;y<topValue;++y){
                int index = getXYMap()(x,y);
                if(index != NO_LED){
                    context.leds[index] = CRGB::Red;
                }
            }
            for(int y=middle;y>=botValue;--y){
                int index = getXYMap()(x,y);
                if(index != NO_LED){
                    context.leds[index] = CRGB::Red;
                }
            }
        }
        // fill_solid(context.leds, mNumLeds, CRGB::Red);
    }
    fadeToBlackBy(context.leds, mNumLeds, 5);
}

fl::string ITFill::fxName() const
{
    return "ITFill";
}