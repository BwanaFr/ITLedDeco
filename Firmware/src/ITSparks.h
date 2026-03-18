#pragma once

#include "fl/fx/fx2d.h"



class ITSparks : public fl::Fx2d {
public:
    ITSparks(const fl::XYMap &xymap);
    inline void setNbSparks(const fl::u16 count) { nbSparks_ = count; }
    inline void setRate(const fl::u16 rate) { rate_ = rate; }
    virtual void draw(fl::Fx::DrawContext context) override;
    virtual fl::string fxName() const override;

private:

    fl::u16 nbSparks_;  //!<< Number of sparks
    fl::u16 rate_;      //!<< Sparks generation rate [ms]
    fl::u32 lastGen_;   //!<< Last time sparks was generated
};

#include "ITSparks.cpp.hpp"