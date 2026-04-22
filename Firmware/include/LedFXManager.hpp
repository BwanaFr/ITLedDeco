#pragma once

#include <map>
#include <FastLED.h>
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
     * @return true if configuration is changed
     */
    bool setFXConfigurations(const JsonDocument& doc, bool save = true);

    /**
     * Gets current Fx pointer
     * @return Const pointer to actual LedFX or nullptr if no FX?
     */
    const LedFX* getCurrentFX() const;

    /**
     * Get actual FX timing
     * @param nextInMs Milliseconds to next FX change
     * @param dispUntilMs Milliseconds since display of the actual FX
     */
    bool getFXInfos(std::string& actualName, unsigned long& nextInMs, unsigned long& dispUntilMs) const;

private:
    fl::FxEngine fxEngine_;
    typedef std::map<int, LedFX*> FXMap;
    FXMap fxMap_;
    unsigned long lastChange_;
    bool autoChange_;
};