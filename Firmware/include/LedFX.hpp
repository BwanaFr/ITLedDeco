#pragma once

#include <FastLED.h>
#include <fl/fx/fx.h>
#include <ArduinoJson.h>
#include <fl/audio/audio_reactive.h>

class LedFX {
public:
    /**
     * Constructor
     * @param fx Reference to the Fx object
     * @param displayPeriod Time (ms) to show FX
     * @param transitionTime Time to transition to previous FX
     */
    LedFX(fl::Fx* fx, unsigned long displayPeriod = 10000, unsigned long transitionTime = 500);
    LedFX(const LedFX&) = delete;
    virtual ~LedFX() = default;

    /**
     * Gets configuration for the FX
     */
    void getConfiguration(JsonDocument& doc) const;

    /**
     * Sets specific configuration for this FX
     */
    bool setConfiguration(const JsonDocument& doc);

    /**
     * Sets time to show the FX
     * @param period Time in ms to display the FX
     */
    inline void setDisplayPeriod(unsigned long period) { displayPeriod_ = period; }

    /**
     * Gets the time to show this FX
     * @return Time to show this FX
     */
    inline unsigned long getDisplayPeriod() const { return displayPeriod_; }

    /**
     * Sets transition time for this FX
     * @param time Time in ms to perform the transtion
     */
    inline void setTransitionTime(unsigned long time) { transitionTime_ = time; }

    /**
     * Gets the transition time of this FX
     * @return Time in ms to transition to this FX
     */
    inline unsigned long getTransisitonTime() const { return transitionTime_; }

    /**
     * Gets pointer to the underlined FastLED FX
     */
    inline fl::Fx* getFX() const { return fx_; };

    /**
     * Sets if this FX is enabled
     * @param enable True to enable this FX
     */
    inline void setEnabled(bool enable){ enabled_ = enable; }

    /**
     * Gets if this FX is enabled
     * @return true if this FX is enabled
     */
    inline bool isEnabled() const { return enabled_; }

    /**
     * Called when a new audio sample is received (and Fx is active)
     */
    virtual void audioReactive(fl::audio::Reactive& reactive);

    /**
     * Just called before the draw method of the Fx
     */
    virtual void beforeDraw();

    /**
     * Gets if the FX needs audio
     * @return true if the FX needs audio
     */
    virtual bool needAudio();
protected:

    virtual void getCustomConfiguration(JsonObject& obj) const;

    /**
     * Sets configuration for the FX
     * @return true when setting accepted
     */
    virtual bool setCustomConfiguration(JsonObjectConst obj);

//Several helpers to create settings in the JSON object
    /**
     * Creates a generic setting
     */
    template<typename T>
    static JsonObject createSetting(JsonObject& obj, const char* name, const char* desc, T val){
        JsonObject ret = obj[name].to<JsonObject>();
        ret["desc"] = desc;
        ret["val"] = val;
        return ret;
    }

    /**
     * Creates a slider
     */
    template<typename T>
    static JsonObject createSetting(JsonObject& obj, const char* name, const char* desc, T val, T min){
        JsonObject o = createSetting<T>(obj, name, desc, val);
        o["min"] = min;
        return o;
    }

    /**
     * Creates a slider
     */
    template<typename T>
    static JsonObject createSetting(JsonObject& obj, const char* name, const char* desc, T val, T min, T max){
        JsonObject o = createSetting<T>(obj, name, desc, val, min);
        o["max"] = max;
        return o;
    }

    template<typename T>
    static bool setValueIfSet(JsonObjectConst obj, const char* name, T& val){
        JsonObjectConst v = obj[name];
        if(v){
            if(v["val"].is<T>()){
                val = v["val"].as<T>();
                return true;
            }
        }
        return false;
    }

private:
    fl::Fx* fx_;
    unsigned long displayPeriod_;
    unsigned long transitionTime_;
    bool enabled_;
};
