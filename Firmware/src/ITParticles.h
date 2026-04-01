#pragma once

#include <array>
#include <fl/fx/1d/particles.h>

class ITParticles : public fl::Fx1d {
public:
    ITParticles(fl::u16 num_leds);
    virtual ~ITParticles();
    fl::string fxName() const override;
    void draw(DrawContext context) override;
    void spawnRandomParticle();
private:
    std::array<fl::Particles1d, 5> subStrips_;
};

#include <ITParticles.cpp.hpp>