#include <FLParticles.hpp>

FLParticles::FLParticles(fl::u16 nbLeds, fl::u8 max_particles, fl::u8 fade_rate) :
    LedFX{&p1dFx_}, p1dFx_{nbLeds, max_particles, fade_rate}, spawnTime_{200},
    lastSpawn_{0}
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
}