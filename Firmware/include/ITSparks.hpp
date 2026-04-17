#pragma once

#include "fl/fx/fx2d.h"
#include <LedFX.hpp>


class ITSparks : public fl::Fx2d, public LedFX {
public:
    ITSparks(const fl::XYMap &xymap);
    inline void setNbSparks(const fl::u16 count) { nbSparks_ = count; }
    inline void setRate(const fl::u16 rate) { rate_ = rate; }
    virtual void draw(fl::Fx::DrawContext context) override;
    virtual fl::string fxName() const override;

    virtual void getCustomConfiguration(JsonObject& obj) const override;
    bool setCustomConfiguration(JsonObjectConst obj) override;
private:

    fl::u16 nbSparks_;      //!<< Number of sparks
    fl::u16 rate_;          //!<< Sparks generation rate [ms]
    fl::u32 lastGen_;       //!<< Last time sparks was generated
    fl::u8 randomLight_;    //!<< Random light amount
    fl::u8 fadeRate_;       //!< Fade rate
    fl::u8 baseLight_;      //!< Base light
};
