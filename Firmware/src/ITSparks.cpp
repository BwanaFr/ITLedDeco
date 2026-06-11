#include <FastLED.h>
#include "ITSparks.hpp"
#include "ITLedMap.hpp"

static const char* TAG = "ITSparks";

ITSparks::ITSparks(const fl::XYMap &xymap) :
    fl::Fx2d(xymap),
    LedFX{this},
    nbSparks_{10}, rate_{100}, lastGen_{0},
    randomLight_{20}, fadeRate_{20}, baseLight_{4},
    blurAmount_{2}, lastBeatDetected_{0}, beatDetectionAdd_{0}
{
}

void ITSparks::draw(fl::Fx::DrawContext context)
{
    unsigned long lastSample = ::millis();
    if((context.now - lastGen_) >= rate_){
        for(int i=0;i<nbSparks_;++i){
            fl::u16 index = random16(NB_STRIP_LEDS);
            context.leds[index] = CRGB::White;
            context.leds[index].fadeLightBy(random8(randomLight_));
        }

        lastGen_ = context.now;
    }
    fadeToBlackBy(context.leds, fadeRate_);
    fl::u8 bg = baseLight_;
    if((::millis() - lastBeatDetected_) < 100){
        bg = fl::clamp((bg + beatDetectionAdd_), 0, 255);
    }
    for(int i=0;i<NB_STRIP_LEDS;++i){
        context.leds[i].addToRGB(bg);
    }
}

fl::string ITSparks::fxName() const
{
    return "ITSparks";
}

void ITSparks::getCustomConfiguration(JsonObject& obj) const
{
    createSetting<fl::u16>(obj, "sparks", "Sparks count", nbSparks_, 0, 20);
    createSetting<fl::u16>(obj, "rate", "Generation rate [ms]", rate_, 0);
    createSetting<fl::u8>(obj, "rndLight", "Random fade", randomLight_, 0, 255);
    createSetting<fl::u8>(obj, "fadeRate", "Fade rate", fadeRate_, 0, 255);
    createSetting<fl::u8>(obj, "baseLight", "Base light", baseLight_, 0, 255);
    createSetting<fl::u8>(obj, "blur", "Blur amount", blurAmount_, 0, 255);
    createSetting<fl::u8>(obj, "sndReactiveAdd", "Beat add", beatDetectionAdd_, 0, 255);
}

bool ITSparks::setCustomConfiguration(JsonObjectConst obj)
{
    setValueIfSet(obj, "sparks", nbSparks_);
    setValueIfSet(obj, "rate", rate_);
    setValueIfSet(obj, "rndLight", randomLight_);
    setValueIfSet(obj, "fadeRate", fadeRate_);
    setValueIfSet(obj, "baseLight", baseLight_);
    setValueIfSet(obj, "blur", blurAmount_);
    setValueIfSet(obj, "sndReactiveAdd", beatDetectionAdd_);
    return true;
}

void ITSparks::audioReactive(const AudioReactiveData* reactive)
{
    if(reactive){
        lastBeatDetected_ = reactive->lastBeat;
    }
}