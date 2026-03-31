#pragma once
#include <FastLED.h>
#include "ITFill.h"
#include "ITLedMap.h"

#define BLUR_AMOUNT 2

ITFill::ITFill(const fl::XYMap &xymap) : fl::Fx2d(xymap), step_{0}, rate_{100}
{
}


void ITFill::draw(fl::Fx::DrawContext context)
{
    if((context.now - lastGen_) >= rate_){
        // if(step_ < 0)
        lastGen_ = context.now;
    }
    fadeToBlackBy(context.leds, mNumLeds, 20);
    for(int i=0;i<mNumLeds;++i){
        context.leds[i].addToRGB(8);
    }
}

fl::string ITFill::fxName() const
{
    return "ITFill";
}