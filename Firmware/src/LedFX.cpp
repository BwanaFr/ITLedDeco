#include <LedFX.hpp>

LedFX::LedFX(fl::Fx* fx, unsigned long displayPeriod, unsigned long transitionTime) :
            fx_{fx}, displayPeriod_{displayPeriod}, transitionTime_{transitionTime},
            enabled_{true}
{
}

void LedFX::getConfiguration(JsonDocument& doc) const
{
    JsonObject obj = doc[getFX()->fxName()].to<JsonObject>();
    obj["enabled"] = isEnabled();
    createSetting<fl::u32>(obj, "transitionTime", "FX transition time [ms]", transitionTime_, 0);
    createSetting<fl::u32>(obj, "displayTime", "FX display time [ms]", displayPeriod_, 0);
    getCustomConfiguration(obj);
}

void LedFX::getCustomConfiguration(JsonObject& obj) const
{
}

void LedFX::setCustomConfiguration(const JsonObject& obj)
{
}

bool LedFX::setConfiguration(const JsonDocument& obj)
{
    if(obj[getFX()->fxName()].is<JsonObject>()){
        JsonObject obj = obj[getFX()->fxName()].as<JsonObject>();
        if(obj["enabled"].is<bool>()){
            setEnabled(obj["enabled"].as<bool>());
        }
    }
}

void LedFX::audioReactive(fl::audio::Reactive& reactive)
{
}

void LedFX::beforeDraw()
{
}