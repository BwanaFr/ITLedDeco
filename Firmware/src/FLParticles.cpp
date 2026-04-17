#include <FLParticles.hpp>

FLParticles::FLParticles(fl::u16 nbLeds, fl::u8 max_particles) :
    LedFX{&p1dFx_}, p1dFx_{nbLeds, max_particles, 2}, spawnTime_{200},
    lastSpawn_{0}, lifetime_{4000}, speed_{100}, fadeRate_{2}
{
}

void FLParticles::beforeDraw()
{
    if((millis() - lastSpawn_) >= spawnTime_){
        p1dFx_.spawnRandomParticle();
        lastSpawn_ = millis();
    }
}

void FLParticles::getCustomConfiguration(JsonObject& obj) const
{
    createSetting<fl::u32>(obj, "spawnTime", "Spawn every [ms]", spawnTime_, 0);
    createSetting<fl::u16>(obj, "lifeTime", "Particule life time [ms]", lifetime_, 0);
    createSetting(obj, "speed", "Speed multiplier", speed_, 0, 200);
    createSetting<fl::u8>(obj, "fadeRate", "Fade rate", fadeRate_, 0, 255);
}

bool FLParticles::setCustomConfiguration(JsonObjectConst obj)
{
    setValueIfSet(obj, "spawnTime", spawnTime_);
    if(setValueIfSet(obj, "lifeTime", lifetime_)){
        p1dFx_.setLifetime(lifetime_);
    }
    if(setValueIfSet(obj, "speed", speed_)){
        float fSpeed = speed_/100.0f;
        p1dFx_.setSpeed(fSpeed);
    }
    if(setValueIfSet(obj, "fadeRate", fadeRate_)){
        p1dFx_.setFadeRate(fadeRate_);
    }
    return true;
}