#pragma once

#include "fl/fx/fx2d.h"



class ITVuMeter : public fl::Fx2d {
public:
    ITVuMeter(const fl::XYMap &xymap);
    inline void setRate(const fl::u16 rate) { rate_ = rate; }
    inline void setFadeRate(const fl::u8 rate) { fadeRate_ = rate; }
    void draw(fl::Fx::DrawContext context) override;
    virtual fl::string fxName() const override;
    void setLevel(fl::u8 level);
    void setRandomPalette();
    void setPalette(int index);
private:
    fl::u16 rate_;                      //!<< Sparks generation rate [ms]
    fl::u32 lastGen_;                   //!<< Last time sparks was generated
    fl::u8 level_;                      //!<< Current step
    fl::u8 fadeRate_;                   //!<< LED fade rate
    fl::CRGBPalette16 currentPalette_;  //!<< Current palette
    static constexpr int MAX_PALETTE = 8;

    void SetupGrayAndWhiteStripedPalette();

    void SetupPurpleAndGreenPalette();
};

#include "ITVuMeter.cpp.hpp"
