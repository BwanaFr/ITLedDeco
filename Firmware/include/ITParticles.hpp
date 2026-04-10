#pragma once

#include <array>
#include <fl/fx/1d/particles.h>

class ITParticles : public fl::Fx1d {
public:
    ITParticles(fl::u16 num_leds, fl::u8 max_particles = 3, fl::u8 fade_rate = 1);
    virtual ~ITParticles();
    fl::string fxName() const override;
    void draw(DrawContext context) override;
    /**
     * @see fl::Particles1d::spawnRandomParticle()
     */
    void spawnRandomParticle();

    /**
     * @see fl::Particles1d::setSpeed(float)
     */
    void setSpeed(float speed);

    /**
     * @see fl::Particles1d::setLifetime(fl::u16)
     */
    void setLifetime(fl::u16 lifetime_ms);

    /**
     * @see fl::Particles1d::setFadeRate(fl::u8)
     */
    void setFadeRate(fl::u8 fade_rate);

    /**
     * @see fl::Particles1d::setOverdrawCount(fl::u8)
     */
    void setOverdrawCount(fl::u8 count);

    /**
     * Sets the number of paritcules to spawn
     * @param spawnNumber Number of particules to spawn
     */
    inline void setNbParticules(fl::u8 spawnNumber){ nbParticules_ = spawnNumber; }
    inline fl::u8 getNbParticules() const { return nbParticules_; }
private:
    std::array<fl::Particles1d, 5> subStrips_;
    fl::u8 nbParticules_;
};
