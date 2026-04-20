#include <FastLED.h>
#include "ITVuMeter.hpp"
#include "ITLedMap.hpp"
#include <Arduino.h>
#include <sstream>

#define BLUR_AMOUNT 2

ITVuMeter::ITVuMeter(const fl::XYMap &xymap) :
        fl::Fx2d(xymap),
        LedFX{this},
        rate_{20}, lastGen_{0}, paletteChangeRate_{10000},
        lastPaletteChange_{0}, level_{0.0f}, levelGain_{100.0f}, fadeRate_{15}, currentPalette_{0},
        palettes_{{
            {&RainbowColors_p, "Rainbow"},
            {&ForestColors_p, "Forest colors"},
            {&CloudColors_p, "Cloud colors"},
            {&LavaColors_p, "Lava colors"},
            {&OceanColors_p, "Ocean colors"},
            {&PartyColors_p, "Party colors"}
        }},
        energyMax_{0.0f}, energyCell_{0}, lastEnergyChange_{0}
{
}


void ITVuMeter::draw(fl::Fx::DrawContext context)
{
    fadeToBlackBy(context.leds, fadeRate_);
    if((context.now - lastGen_) >= rate_){
        lastGen_ = context.now;
        int middle = getHeight()/2;
        int count = getHeight() - middle;
        int stopTop = fl::map_range_clamped<float, int>(level_, 0.0f, levelGain_, middle, getHeight());
        int stopBot = middle - fl::map_range_clamped<float, int>(level_, 0.0f, levelGain_, 0, middle);
        const TProgmemRGBPalette16* pal = palettes_[currentPalette_].palette;
        for(int y=middle;y<stopTop;++y){
            CRGB color = ColorFromPalette(*pal, map(y, middle, getHeight(), 0, 255));
            for(int x=0;x<getWidth();++x){
                fl::u16 index = getXYMap()(x,y);
                context.leds[index] = color;
            }
        }
        for(int y=middle-1;y>=stopBot;--y){
            CRGB color = ColorFromPalette(*pal, map(middle-y, 0, middle+1, 0, 255));
            for(int x=0;x<getWidth();++x){
                fl::u16 index = getXYMap()(x,y);
                context.leds[index] = color;
            }
        }
    }
}

fl::string ITVuMeter::fxName() const
{
    return "ITVuMeter";
}

void ITVuMeter::setRandomPalette()
{
    int enabledCount = getNbEnabledPalette();
    if(enabledCount == 1){
        for(int i=0;i<palettes_.size(); ++i){
            if(palettes_[i].enabled){
                currentPalette_ = i;
                return;
            }
        }
    }else if(enabledCount > 1){
        while(true){
            fl::u16 idx = random16(idx) % MAX_PALETTE;
            if(palettes_[idx].enabled && (idx != currentPalette_)){
                currentPalette_ = idx;
                return;
            }
        }
    }
}

void ITVuMeter::setPalette(int index)
{
    currentPalette_ = index % MAX_PALETTE; // Ensure the index wraps around
}

void ITVuMeter::getCustomConfiguration(JsonObject& obj) const
{
    createSetting<fl::u16>(obj, "rate", "Display rate [ms]", rate_);
    createSetting<fl::u32>(obj, "palRate", "Palette change rate [ms]", paletteChangeRate_);
    createSetting<fl::u8>(obj, "fadeRate", "Fade rate", fadeRate_, 0, 255);
    createSetting<float>(obj, "levelGain", "Level gain", levelGain_, 0, 200.0f);
    int i=0;
    for(const PaletteInfo& pal : palettes_){
        std::ostringstream palId;
        palId <<  "palette_" << ++i;
        std::string palName = pal.name + " palette?";
        createSetting(obj, palId.str().c_str(), palName.c_str(), pal.enabled);
    }
}

bool ITVuMeter::setCustomConfiguration(JsonObjectConst obj)
{
    setValueIfSet<fl::u16>(obj, "rate", rate_);
    setValueIfSet<fl::u8>(obj, "fadeRate", fadeRate_);
    setValueIfSet<fl::u32>(obj, "palRate", paletteChangeRate_);
    setValueIfSet<float>(obj, "levelGain", levelGain_);

    int i=0;
    for(PaletteInfo& pal : palettes_){
        std::ostringstream palId;
        palId <<  "palette_" << ++i;
        setValueIfSet(obj, palId.str().c_str(), pal.enabled);
    }
    if(getNbEnabledPalette() == 0){
        setEnabled(false);
    }
    return true;
}

void ITVuMeter::beforeDraw()
{
    // if((fl::millis() - lastPaletteChange_) >= paletteChangeRate_){
    //     setRandomPalette();
    //     lastPaletteChange_ = fl::millis();
    // }
}

void ITVuMeter::audioReactive(fl::audio::Reactive& reactive)
{
    float be = reactive.getBassEnergy();
    //Gets max over buffer
    if(be > energyMax_[energyCell_]){
        energyMax_[energyCell_] = be;
    }
    fl::u32 now = fl::millis();
    if((now - lastEnergyChange_) > 200){
        if(++energyCell_ >= energyMax_.size()){
            energyCell_ = 0;
        }
        energyMax_[energyCell_] = 0.0f;
        lastEnergyChange_ = now;
    }
    float maxInBuffer = 0.0f;
    for(float f : energyMax_){
        if(f>maxInBuffer){
            maxInBuffer = f;
        }
    }
    float newLevel = fl::map_range_clamped(be, 0.0f, maxInBuffer, 0.0f, 100.0f);
    if(newLevel != level_){
        level_ = newLevel;
        lastGen_ = 0;
    }
}

int ITVuMeter::getNbEnabledPalette()
{
    int ret = 0;
    for(const PaletteInfo& inf : palettes_){
        if(inf.enabled){
            ++ret;
        }
    }
    return ret;
}

bool ITVuMeter::needAudio()
{
    //We always need audio for our processor
    return true;
}
