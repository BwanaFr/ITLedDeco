#pragma once

#include "fl/fx/fx2d.h"



class ITFill : public fl::Fx2d {
public:
    ITFill(const fl::XYMap &xymap);
    inline void setRate(const fl::u16 rate) { rate_ = rate; }
    void draw(fl::Fx::DrawContext context) override;
    virtual fl::string fxName() const override;
    inline void setLevel(fl::u8 level) { level_ = level; }
private:
    fl::u16 rate_;      //!<< Sparks generation rate [ms]
    fl::u32 lastGen_;   //!<< Last time sparks was generated
    fl::u8 level_;      //!<< Current step
    fl::u16 width_;
    fl::u16 height_;
};

#include "ITFill.cpp.hpp"
