#include <ITParticles.hpp>
#include <ITLedMap.hpp>

ITParticles::ITParticles(fl::u16 num_leds, fl::u8 max_particles, fl::u8 fade_rate) :
            Fx1d{num_leds},
            LedFX{this},
            subStrips_{fl::Particles1d(LETTER_WIDTH, max_particles, fade_rate), fl::Particles1d(LETTER_HEIGHT, max_particles, fade_rate),
                        fl::Particles1d(LETTER_WIDTH, max_particles, fade_rate), fl::Particles1d(LETTER_HEIGHT, max_particles, fade_rate), fl::Particles1d(LETTER_WIDTH, max_particles, fade_rate)},
            nbParticules_{max_particles}, audioReactive_{true}, spawnTime_{200}, lastSpawn_{0}, lifetime_{4000}, speed_{100}, fadeRate_{2}
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

void ITParticles::getCustomConfiguration(JsonObject& obj) const
{
    createSetting(obj, "reactive", "Music reactive?", audioReactive_);
    createSetting<fl::u8>(obj, "maxParts", "Maximum particules", nbParticules_, 0);
    createSetting<fl::u32>(obj, "spawnTime", "Spawn every (if not reactive) [ms]", spawnTime_, 0);
    createSetting<fl::u16>(obj, "lifeTime", "Particule life time [ms]", lifetime_, 0);
    createSetting(obj, "speed", "Speed multiplier", speed_, 0, 200);
    createSetting<fl::u8>(obj, "fadeRate", "Fade rate", fadeRate_, 0, 255);
}

bool ITParticles::setCustomConfiguration(JsonObjectConst obj)
{
    setValueIfSet(obj, "reactive", audioReactive_);
    setValueIfSet(obj, "maxParts", nbParticules_);
    setValueIfSet(obj, "spawnTime", spawnTime_);
    if(setValueIfSet(obj, "lifeTime", lifetime_)){
        setLifetime(lifetime_);
    }
    if(setValueIfSet(obj, "speed", speed_)){
        float fSpeed = speed_/100.0f;
        setSpeed(fSpeed);
    }
    if(setValueIfSet(obj, "fadeRate", fadeRate_)){
        setFadeRate(fadeRate_);
    }
    return true;
}

void ITParticles::audioReactive(fl::audio::Reactive& reactive)
{
    if(audioReactive_){
        //Spwan a random particule when a beat is detected
        if(reactive.isBeat()){
            spawnRandomParticle();
        }
    }
}

void ITParticles::beforeDraw()
{
    if(!audioReactive_ && (millis() - lastSpawn_) >= spawnTime_){
        spawnRandomParticle();
        lastSpawn_ = millis();
    }
}