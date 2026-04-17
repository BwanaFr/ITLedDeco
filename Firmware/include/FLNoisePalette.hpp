#pragma once

#include "FastLED.h"
#include "fl/fx/2d/noisepalette.h"
#include <LedFX.hpp>

class FLNoisePalette : public LedFX
{
public:
    FLNoisePalette(fl::XYMap xyMap);
    virtual ~FLNoisePalette() = default;
    void beforeDraw();
protected:
    void getCustomConfiguration(JsonObject& obj) const override;
    bool setCustomConfiguration(JsonObjectConst obj) override;
private:
    fl::NoisePalette npFx_;
    fl::u32 changePaletteTime_;
    fl::u32 lastPaletteChange_;
    bool rainbowEnabled_;
    bool purpleGreenEnabled_;
    bool blackAndWhiteEnabled_;
    bool forestColorEnabled_;
    bool cloudColorsEnabled_;
    bool lavaColorsEnabled_;
    bool oceanColorsEnabled_;
    bool partyColorsEnabled_;
    bool rainbowStripColorsEnabled_;

    /**
     * Gets next palette (or -1 if no palette is activated)
     */
    int getNextPalette();

    /**
     * Return true if one palette is active
     */
    bool paletteEnabled() const;

    bool isPaletteEnabled(fl::u8 index) const;

    int getEnabledPaletteCount() const;

    /**
     * Our pseudo palette change
     */
    void changePaletteRandom();
};