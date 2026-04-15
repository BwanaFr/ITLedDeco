#include <ITParticles.hpp>
#include <ITLedMap.hpp>

ITParticles::ITParticles(fl::u16 num_leds, fl::u8 max_particles, fl::u8 fade_rate) :
            Fx1d{num_leds},
            LedFX{this},
            subStrips_{fl::Particles1d(LETTER_WIDTH, max_particles, fade_rate), fl::Particles1d(LETTER_HEIGHT, max_particles, fade_rate),
                        fl::Particles1d(LETTER_WIDTH, max_particles, fade_rate), fl::Particles1d(LETTER_HEIGHT, max_particles, fade_rate), fl::Particles1d(LETTER_WIDTH, max_particles, fade_rate)},
            nbParticules_{max_particles}
{
    for(fl::Particles1d& p : subStrips_){
        p.setCyclical(false);
    }
}

ITParticles::~ITParticles()
{
}

fl::string ITParticles::fxName() const
{
    return "ITParticules";
}

void ITParticles::setSpeed(float speed)
{
    for(fl::Particles1d& p : subStrips_){
        p.setSpeed(speed);
    }
}

void ITParticles::setLifetime(fl::u16 lifetime_ms)
{
    for(fl::Particles1d& p : subStrips_){
        p.setLifetime(lifetime_ms);
    }
}

void ITParticles::setFadeRate(fl::u8 fade_rate)
{
    for(fl::Particles1d& p : subStrips_){
        p.setFadeRate(fade_rate);
    }
}

void ITParticles::setOverdrawCount(fl::u8 count)
{
    for(fl::Particles1d& p : subStrips_){
        p.setOverdrawCount(count);
    }
}

void ITParticles::draw(DrawContext context)
{
    subStrips_[0].draw(DrawContext(context.now, context.leds.subspan(0, LETTER_WIDTH)));
    subStrips_[1].draw(DrawContext(context.now, context.leds.subspan(LETTER_WIDTH, LETTER_HEIGHT)));
    subStrips_[2].draw(DrawContext(context.now, context.leds.subspan(LETTER_WIDTH + LETTER_HEIGHT, LETTER_WIDTH)));
    subStrips_[3].draw(DrawContext(context.now, context.leds.subspan(LETTER_WIDTH * 2 + LETTER_HEIGHT, LETTER_HEIGHT)));
    subStrips_[4].draw(DrawContext(context.now, context.leds.subspan(LETTER_WIDTH * 2 + LETTER_HEIGHT *2, LETTER_WIDTH)));
}

void ITParticles::spawnRandomParticle()
{
    fl::u8 parts = /*random8(*/nbParticules_;//);
    for(int i=0;i<parts;++i){
        fl::u8 strip = random8() % subStrips_.size();
        subStrips_[strip].spawnRandomParticle();
    }
}

void ITParticles::getConfiguration(JsonObject& obj) const
{
    createSetting(obj, "reactive", "Music reactive?", true);
    createSetting<fl::u8>(obj, "maxParts", "Maximum particules", nbParticules_, 0);
}

void ITParticles::audioReactive(fl::audio::Reactive& reactive)
{
    //Spwan a random particule when a beat is detected
    if(reactive.isBeat()){
        spawnRandomParticle();
    }
}