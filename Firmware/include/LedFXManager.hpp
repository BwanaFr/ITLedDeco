#pragma once

#include <map>
#include <LedFX.hpp>
#include <fl/fx/fx_engine.h>
#include <fl/audio/audio_reactive.h>
#include <ArduinoJson.h>

/**
 * This class manage all FX aspects by using a fl::FxEngine underneath
 */
class LedFXManager{
public:
    LedFXManager(fl::u16 nbLeds);
    virtual ~LedFXManager();
    void draw(fl::span<CRGB> outputBuffer, fl::audio::Reactive* audioreactive);
    void registerFx(LedFX* fx);

    /**
     * Sets if the next FX should be automatically changed
     * @param autoChange True to change the FX automatically
     */
    inline void setAutoChange(bool autoChange) { autoChange_ = autoChange; }

    /**
     * Gets if FX should change automatically
     * @return True if FX is automatically changed
     */
    inline bool isAutoChange() const { return autoChange_; }

    /**
     * Move to next FX
     */
    void nextFX();

    /**
     * Gets FX configuration
     */
    void getFXConfigurations(JsonDocument& doc) const;

    /**
     * Sets FX configuration(s)
     */
    void setFXConfigurations(const JsonDocument& doc);

private:
    fl::FxEngine fxEngine_;
    typedef std::map<int, LedFX*> FXMap;
    FXMap fxMap_;
    unsigned long lastChange_;
    bool autoChange_;
};