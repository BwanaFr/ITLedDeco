#pragma once

#include "fl/fx/2d/noisepalette.h"
#include <LedFX.hpp>

class FLNoisePalette : public LedFX
{
public:
    FLNoisePalette(fl::XYMap xyMap);
    virtual ~FLNoisePalette() = default;
    void beforeDraw();
    void getConfiguration(JsonObject& obj) const override;
private:
    fl::NoisePalette npFx_;
    fl::u32 changePaletteTime_;
    fl::u32 lastPaletteChange_;
};