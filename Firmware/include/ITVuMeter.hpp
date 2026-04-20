#pragma once

#include <FastLED.h>
#include "fl/fx/fx2d.h"
#include <LedFX.hpp>
#include <array>
#include <string>

class ITVuMeter : public fl::Fx2d, public LedFX {
public:
    ITVuMeter(const fl::XYMap &xymap);
    inline void setRate(const fl::u16 rate) { rate_ = rate; }
    inline void setFadeRate(const fl::u8 rate) { fadeRate_ = rate; }
    void draw(fl::Fx::DrawContext context) override;
    virtual fl::string fxName() const override;
    void setRandomPalette();
    void setPalette(int index);
    void getCustomConfiguration(JsonObject& obj) const override;
    void audioReactive(fl::audio::Reactive& reactive) override;
    bool setCustomConfiguration(JsonObjectConst obj) override;
    void beforeDraw() override;
    bool needAudio() override;
private:
    fl::u16 rate_;                          //!<< Sparks generation rate [ms]
    fl::u32 lastGen_;                       //!<< Last time FX was generated
    fl::u32 paletteChangeRate_;             //!<< Random palette change
    fl::u32 lastPaletteChange_;             //!<< Last random palette change
    float level_;                           //!<< Current step
    float levelGain_;                       //!<< Level gain
    fl::u8 fadeRate_;                       //!<< LED fade rate
    int currentPalette_;                    //!<< Current palette index
    static constexpr int MAX_PALETTE = 6;

    struct PaletteInfo{
        bool enabled;
        const TProgmemRGBPalette16* palette;
        std::string name;

        PaletteInfo(bool en, const TProgmemRGBPalette16* pal, const std::string& name) :
                    enabled{en}, palette{pal}, name{name}
        {}

        PaletteInfo(const TProgmemRGBPalette16* pal, const std::string& name) :
                    enabled{true}, palette{pal}, name{name}
        {}
    };
    std::array<PaletteInfo, MAX_PALETTE> palettes_;

    int getNbEnabledPalette();

    std::array<float, 50> energyMax_;
    int energyCell_;
    int lastEnergyChange_;
};
