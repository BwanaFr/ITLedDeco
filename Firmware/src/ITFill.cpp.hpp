#pragma once
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
        //Orr use map_range_clamped
        int middle = getHeight()/2;
        int count = getHeight() - middle;
        int stopTop = middle + ((count + 1)*level_)/255;
        int stopBot = middle - ((count + 1)*level_)/255;
        for(int x=0;x<getWidth();++x){
            for(int y=middle;y<stopTop;++y){
                int index = getXYMap()(x,y);
                if(index != NO_LED){
                    context.leds[index] = ColorFromPalette(PartyColors_p, map(y+1, middle, getHeight(), 0, 255));
                }
            }
            for(int y=middle;y>stopBot;--y){
                int index = getXYMap()(x,y);
                if(index != NO_LED){
                    context.leds[index] = ColorFromPalette(PartyColors_p, map(middle-y, 0, middle, 0, 255));
                }
            }
        }
    }
    fadeToBlackBy(context.leds, 15);
}

fl::string ITFill::fxName() const
{
    return "ITFill";
}