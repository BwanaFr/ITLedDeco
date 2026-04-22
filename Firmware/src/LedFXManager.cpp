#include <LedFXManager.hpp>
#include <Arduino.h>
#include "esp_log.h"
#include <Configuration.hpp>

const char* TAG = "LedFXManager";

LedFXManager::LedFXManager(fl::u16 nbLeds) :
        fxEngine_{nbLeds, false}, //Remove the interpolation as it results to crashes...
        lastChange_{0}, autoChange_{true}
{
}

LedFXManager::~LedFXManager()
{
}

void LedFXManager::draw(fl::span<CRGB> outputBuffer, fl::audio::Reactive* audioreactive)
{
    int currId = fxEngine_.getCurrentFxId();
    LedFX* currLedFX = fxMap_[currId];
    if(audioreactive){
        currLedFX->audioReactive(*audioreactive);
        //Feed audio for requesting FX
        for(FXMap::iterator it=fxMap_.begin();it!=fxMap_.end();++it){
            if(it->second != currLedFX){
                if(it->second->needAudio()){
                    it->second->audioReactive(*audioreactive);
                }
            }
        }
    }
    currLedFX->beforeDraw();

    fxEngine_.draw(fl::millis(), outputBuffer);

    //Maybe it's time to switch
    if(autoChange_ && (millis() >= (lastChange_ + currLedFX->getDisplayPeriod()))){
        nextFX();
    }
}

void LedFXManager::registerFx(LedFX* fx)
{
    if(!fx){
        ESP_LOGE(TAG, "Can't register null LedFX!");
        return;
    }
    fl::Fx* flFx = fx->getFX();
    if(!flFx){
        ESP_LOGE(TAG, "Can't register LedFx with null FX!");
        return;
    }
    int index = fxEngine_.addFx(*flFx);
    if(index > -1){
        fxMap_[index] = fx;
    }
}

void LedFXManager::nextFX()
{
    if(fxMap_.size() > 1){
        int curId = fxEngine_.getCurrentFxId();
        while(true){
            ++curId;
            if(curId >= fxMap_.size()){
                //Loop
                curId = 0;
            }
            if(curId == fxEngine_.getCurrentFxId()){
                ESP_LOGI(TAG, "No next FX to display.");
                lastChange_ = millis();
                break;
            }
            LedFX* nextFx = fxMap_[curId];
            if(nextFx->isEnabled()){
                ESP_LOGI(TAG, "NextFX -> %s", nextFx->getFX()->fxName().c_str());
                fxEngine_.setNextFx(curId, nextFx->getTransisitonTime());
                lastChange_ = millis();
                break;
            }
        }
    }
}

void LedFXManager::getFXConfigurations(JsonDocument& doc) const
{
    for(FXMap::const_iterator it=fxMap_.begin(); it != fxMap_.end(); ++it){
        LedFX* fx = it->second;
        fx->getConfiguration(doc);
    }
}

bool LedFXManager::setFXConfigurations(const JsonDocument& doc, bool save)
{
    bool changed = false;
    for(FXMap::iterator it=fxMap_.begin(); it != fxMap_.end(); ++it){
        LedFX* fx = it->second;
        changed |= fx->setConfiguration(doc);
    }
    if(changed && save){
        configuration.saveFXConfiguration();
    }
    //Switch FX if actual is disabled (by config)
    int curId = fxEngine_.getCurrentFxId();
    if(!fxMap_[curId]->isEnabled()){
        nextFX();
    }
    return changed;
}

const LedFX* LedFXManager::getCurrentFX() const
{
    const int curId = fxEngine_.getCurrentFxId();
    FXMap::const_iterator it = fxMap_.find(curId);
    if(it != fxMap_.end()){
        return it->second;
    }
    return nullptr;
}

bool LedFXManager::getFXInfos(std::string& actualName, unsigned long& nextInMs, unsigned long& dispUntilMs) const
{
    unsigned long now = millis();
    const LedFX* currLedFX = getCurrentFX();
    if(currLedFX){
        actualName = currLedFX->getFX()->fxName().c_str();
        dispUntilMs = now - lastChange_;
        if(isAutoChange()){
            nextInMs = currLedFX->getDisplayPeriod() - dispUntilMs;
        }else{
            nextInMs = 0;
        }
        return true;
    }
    return false;
}