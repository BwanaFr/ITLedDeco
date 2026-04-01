#pragma once
#include <ITParticles.h>
#include <ITLedMap.h>

ITParticles::ITParticles(fl::u16 num_leds) : Fx1d(num_leds),
            subStrips_{fl::Particles1d(LETTER_WIDTH, 1), fl::Particles1d(LETTER_HEIGHT, 1),
                        fl::Particles1d(LETTER_WIDTH, 1), fl::Particles1d(LETTER_HEIGHT, 1), fl::Particles1d(LETTER_WIDTH, 1)}
{
}

ITParticles::~ITParticles()
{
}

fl::string ITParticles::fxName() const
{
    return "ITParticules";
}

void ITParticles::draw(DrawContext context)
{
    subStrips_[0].draw(DrawContext(context.now, context.leds.subspan(1, LETTER_WIDTH)));
    subStrips_[1].draw(DrawContext(context.now, context.leds.subspan(LETTER_WIDTH + 1, LETTER_HEIGHT)));
}

void ITParticles::spawnRandomParticle()
{
    //TODO: Generate only one particle for one object
    subStrips_[0].spawnRandomParticle();
    subStrips_[1].spawnRandomParticle();
}