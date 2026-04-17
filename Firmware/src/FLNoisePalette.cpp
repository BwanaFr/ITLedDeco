#include <FLNoisePalette.hpp>

FLNoisePalette::FLNoisePalette(fl::XYMap xyMap) :
    LedFX{&npFx_}, npFx_{xyMap}, changePaletteTime_{10000},
    lastPaletteChange_{0}, rainbowEnabled_{true}, purpleGreenEnabled_{true},
    blackAndWhiteEnabled_{true}, forestColorEnabled_{true}, cloudColorsEnabled_{true},
    lavaColorsEnabled_{true}, oceanColorsEnabled_{true}, partyColorsEnabled_{true},
    rainbowStripColorsEnabled_{true}
{

}


void FLNoisePalette::beforeDraw()
{
    if((millis() - lastPaletteChange_) >= changePaletteTime_){
        fl::u8 prevIndex = npFx_.getPalettePreset();
        changePaletteRandom();
        lastPaletteChange_ = millis();
    }
}

void FLNoisePalette::getCustomConfiguration(JsonObject& obj) const
{
    createSetting<fl::u32>(obj, "changeTime", "Palette change time [ms]", changePaletteTime_, 200);
    createSetting<bool>(obj, "rainbow", "Rainbow palette?", rainbowEnabled_);
    createSetting<bool>(obj, "purpleGreen", "Purple and green palette?", purpleGreenEnabled_);
    createSetting<bool>(obj, "blackAndWhite", "B/W stripes palette?", blackAndWhiteEnabled_);
    createSetting<bool>(obj, "forestColor", "Forest palette?", forestColorEnabled_);
    createSetting<bool>(obj, "cloudColors", "Cloud palette?", cloudColorsEnabled_);
    createSetting<bool>(obj, "lavaColors", "Lava palette?", lavaColorsEnabled_);
    createSetting<bool>(obj, "oceanColors", "Ocean palette?", oceanColorsEnabled_);
    createSetting<bool>(obj, "partyColors", "Party palette?", partyColorsEnabled_);
    createSetting<bool>(obj, "rainbowStripColors", "Rainbow strip palette?", rainbowStripColorsEnabled_);

}

bool FLNoisePalette::setCustomConfiguration(JsonObjectConst obj)
{
    setValueIfSet(obj, "changeTime", changePaletteTime_);

    setValueIfSet(obj, "rainbow", rainbowEnabled_);
    setValueIfSet(obj, "purpleGreen", purpleGreenEnabled_);
    setValueIfSet(obj, "blackAndWhite", blackAndWhiteEnabled_);
    setValueIfSet(obj, "forestColor", forestColorEnabled_);
    setValueIfSet(obj, "cloudColors", cloudColorsEnabled_);
    setValueIfSet(obj, "lavaColors", lavaColorsEnabled_);
    setValueIfSet(obj, "oceanColors", oceanColorsEnabled_);
    setValueIfSet(obj, "partyColors", partyColorsEnabled_);
    setValueIfSet(obj, "rainbowStripColors", rainbowStripColorsEnabled_);
    int enabledCount = getEnabledPaletteCount();
    if(enabledCount == 0){
        //If no palette is ticked, disable this FX
        setEnabled(false);
    }else if(enabledCount == 1){
        //Only one, switch it right now
        changePaletteRandom();
    }
    return true;
}

int FLNoisePalette::getNextPalette()
{
    int actualPalette = npFx_.getPalettePreset();
    while(true){
        if(!paletteEnabled()){
            return -1;
        }
        actualPalette++;
        if(actualPalette > 11){
            actualPalette = -1;
        }else{
            if(isPaletteEnabled(actualPalette)){
                return actualPalette;
            }
        }
    }
    return -1;  //No palette activated
}

bool FLNoisePalette::isPaletteEnabled(fl::u8 index) const
{
    switch(index){
            case 0:
                return rainbowEnabled_;
            case 1:
                return purpleGreenEnabled_;
            case 2:
                return blackAndWhiteEnabled_;
            case 3:
                return forestColorEnabled_;
            case 4:
                return cloudColorsEnabled_;
            case 5:
                return lavaColorsEnabled_;
            case 6:
                return oceanColorsEnabled_;
            case 7:
                return partyColorsEnabled_;
            case 8:
            case 9:
            case 10:
                //No supported
                return false;
            case 11:
                return rainbowStripColorsEnabled_;
            default:
                return false;
        }
}

bool FLNoisePalette::paletteEnabled() const
{
    return getEnabledPaletteCount() > 0;
}

int FLNoisePalette::getEnabledPaletteCount() const
{
    int ret = 0;
    if(rainbowEnabled_) ++ret;
    if(purpleGreenEnabled_) ++ret;
    if(blackAndWhiteEnabled_) ++ret;
    if(forestColorEnabled_) ++ret;
    if(cloudColorsEnabled_) ++ret;
    if(lavaColorsEnabled_) ++ret;
    if(oceanColorsEnabled_) ++ret;
    if(partyColorsEnabled_) ++ret;
    if(rainbowStripColorsEnabled_) ++ret;
    return ret;
}

void FLNoisePalette::changePaletteRandom()
{
    int palCount = getEnabledPaletteCount();
    if(palCount == 1){
        for(fl::u8 i=0;i<12;++i){
            if(isPaletteEnabled(i)){
                npFx_.setPalettePreset(i);
                return;
            }
        }
    }else if(palCount > 1){
        //More than one
        while (true) {
            u8 new_idx = random8() % 12;
            if ((new_idx == npFx_.getPalettePreset()) || !isPaletteEnabled(new_idx)) {
                continue;
            }
            npFx_.setPalettePreset(new_idx);
            return;
        }
    }
}