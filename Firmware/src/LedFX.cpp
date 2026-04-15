#include <LedFX.hpp>

LedFX::LedFX(fl::Fx* fx, unsigned long displayPeriod, unsigned long transitionTime) :
            fx_{fx}, displayPeriod_{displayPeriod}, transitionTime_{transitionTime},
            enabled_{true}
{
}

void LedFX::getConfiguration(JsonObject& obj) const
{
}

bool LedFX::setConfiguration(const JsonObject& obj)
{
}

void LedFX::audioReactive(fl::audio::Reactive& reactive)
{
}

void LedFX::beforeDraw()
{
}