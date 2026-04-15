#include <FastLED.h>
#include "ITSparks.hpp"
#include "ITLedMap.hpp"

#define BLUR_AMOUNT 2

ITSparks::ITSparks(const fl::XYMap &xymap) :
    fl::Fx2d(xymap),
    LedFX{this},
    nbSparks_{10}, rate_{100}
{
}


void ITSparks::draw(fl::Fx::DrawContext context)
{
    if((context.now - lastGen_) >= rate_){
        for(int i=0;i<nbSparks_;++i){
            fl::u16 index = random16(NB_STRIP_LEDS);
            context.leds[index] = CRGB::White;
            context.leds[index].fadeLightBy(random(20));
        }
        // blur2d(context.leds, getWidth(), getHeight(), BLUR_AMOUNT, getXYMap());
        lastGen_ = context.now;
    }
    fadeToBlackBy(context.leds, 20);
    for(int i=0;i<NB_STRIP_LEDS;++i){
        context.leds[i].addToRGB(4);
    }
}

fl::string ITSparks::fxName() const
{
    return "ITSparks";
}

void ITSparks::getConfiguration(JsonObject& obj) const
{
    createSetting<int>(obj, "sparks", "Sparks count", nbSparks_, 0, 20);
    createSetting<int>(obj, "rate", "Generation rate [ms]", rate_, 0);
}