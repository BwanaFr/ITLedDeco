#pragma once
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
            fl::u16 index = random(NB_STRIP_LEDS) + 1;
            context.leds[index] = CRGB::White;
            context.leds[index].fadeLightBy(random(64));
        }
        blur2d(context.leds, getWidth(), getHeight(), BLUR_AMOUNT, getXYMap());
        lastGen_ = context.now;
    }
    fadeToBlackBy(context.leds, 20);
    for(int i=0;i<NB_STRIP_LEDS;++i){
        context.leds[i+1].addToRGB(4);
    }
}

fl::string ITSparks::fxName() const
{
    return "ITSparks";
}