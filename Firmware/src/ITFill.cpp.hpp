#include <FastLED.h>
#include "ITFill.h"
#include "ITLedMap.h"

#define BLUR_AMOUNT 2

ITFill::ITFill(const fl::XYMap &xymap) : fl::Fx2d(xymap), rate_{100}
{
}


void ITFill::draw(fl::Fx::DrawContext context)
{
    if((context.now - lastGen_) >= rate_){
        // if(step_ < 0)
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
    }
    fadeToBlackBy(context.leds, mNumLeds, 20);
}

fl::string ITFill::fxName() const
{
    return "ITFill";
}