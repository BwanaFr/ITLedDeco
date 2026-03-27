#pragma once

#include "fl/fx/fx2d.h"
#include "fl/fx/1d/particles.h"

class ITParticle : public fl::Fx2d {
public:
    ITParticle(const fl::XYMap &xymap);
    virtual void draw(fl::Fx::DrawContext context) override;
    virtual fl::string fxName() const override;

private:
    fl::Particles1d iBottom_;
    fl::Particles1d iBar_;
};

#include "ITFill.cpp.hpp"
