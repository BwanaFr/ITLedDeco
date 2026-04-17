#include <LedFX.hpp>

LedFX::LedFX(fl::Fx* fx, unsigned long displayPeriod, unsigned long transitionTime) :
            fx_{fx}, displayPeriod_{displayPeriod}, transitionTime_{transitionTime},
            enabled_{true}
{
}

void LedFX::getConfiguration(JsonDocument& doc) const
{
    JsonObject obj = doc[getFX()->fxName()].to<JsonObject>();
    obj["Enabled"] = isEnabled();
    createSetting<fl::u32>(obj, "transitionTime", "FX transition time [ms]", transitionTime_, 0);
    createSetting<fl::u32>(obj, "displayTime", "FX display time [ms]", displayPeriod_, 0);
    getCustomConfiguration(obj);
}

void LedFX::getCustomConfiguration(JsonObject& obj) const
{
}

bool LedFX::setCustomConfiguration(JsonObjectConst obj)
{
    return true;
}

bool LedFX::setConfiguration(const JsonDocument& doc)
{
    bool ret = true;
    const std::string fxName = getFX()->fxName().c_str();
    if(doc[fxName].is<JsonObjectConst>()){
        JsonObjectConst obj = doc[fxName].as<JsonObjectConst>();
        setValueIfSet(obj, "Enabled", enabled_);
        setValueIfSet(obj, "transitionTime", transitionTime_);
        setValueIfSet(obj, "displayTime", displayPeriod_);
        return setCustomConfiguration(obj);
    }
    return true;    //Accept setting...
}

void LedFX::audioReactive(fl::audio::Reactive& reactive)
{
}

void LedFX::beforeDraw()
{
}