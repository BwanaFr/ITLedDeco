#include <FLNoisePalette.hpp>

FLNoisePalette::FLNoisePalette(fl::XYMap xyMap) :
    LedFX{&npFx_}, npFx_{xyMap}, changePaletteTime_{10000},
    lastPaletteChange_{0}
{

}


void FLNoisePalette::beforeDraw()
{
    if((millis() - lastPaletteChange_) >= changePaletteTime_){
        npFx_.changeToRandomPalette();
        lastPaletteChange_ = millis();
    }
}

void FLNoisePalette::getCustomConfiguration(JsonObject& obj) const
{
    createSetting<fl::u32>(obj, "changeTime", "Palette change time [ms]", changePaletteTime_, 0);
}