#pragma once

#include "fl/fx/1d/particles.h"
#include <LedFX.hpp>

class FLParticles : public LedFX
{
public:
    FLParticles(fl::u16 nbLeds, fl::u8 max_particles = 4, fl::u8 fade_rate = 2);
    virtual ~FLParticles() = default;
    void beforeDraw();
    void getConfiguration(JsonObject& obj) const override;
private:
    fl::Particles1d p1dFx_;
    fl::u32 spawnTime_;
    fl::u32 lastSpawn_;
};