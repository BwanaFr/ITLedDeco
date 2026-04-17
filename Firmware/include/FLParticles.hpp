#pragma once

#include "FastLED.h"
#include "fl/fx/1d/particles.h"
#include <LedFX.hpp>

class FLParticles : public LedFX
{
public:
    FLParticles(fl::u16 nbLeds, fl::u8 max_particles = 4);
    virtual ~FLParticles() = default;
    void beforeDraw() override;
    void getCustomConfiguration(JsonObject& obj) const override;
    bool setCustomConfiguration(JsonObjectConst obj) override;
private:
    fl::Particles1d p1dFx_;
    fl::u32 spawnTime_;
    fl::u32 lastSpawn_;
    fl::u16 lifetime_;
    int speed_;
    fl::u8 fadeRate_;
};