/**
 * IT speaker LED strip controller
 * Description : This firmware controls WS2812b from Aliexpress (aliexpress.com/item/1005007587121741.html)
 * The LED strip is arranged to form IT letters. Here is the LED strip layout
 *
 * ExxxxxxxxxxF       IxxxxxxxxxxJ
 *      D                  H
 *      x                  x
 *      x                  x
 *      C                  G
 * AxxxxxxxxxxB
 *
 * Where B-A == F-E == J-I  (called LETTER_WIDTH)
 * and D-C == H-G           (called LETTER_HEIGHT)
 *
 */

#include <WiFi.h>
#include "FastLED.h"
#include <Arduino.h>
#include "esp_log.h"

#include "ITLedMap.hpp"
#include <LedFXManager.hpp>

#include "fl/audio/auto_gain.h"
#include "fl/audio/audio_reactive.h"

#include "FLNoisePalette.hpp"
#include "FLParticles.hpp"
#include "ITSparks.hpp"
#include "ITVuMeter.hpp"
#include "ITParticles.hpp"
#include "ITVuMeter.hpp"

#include "NetworkConfigurator.hpp"
#include "Configuration.hpp"
#include "PCM1808.hpp"

using namespace fl;

#define DATA_PIN_I 45
#define DATA_PIN_T 46
#define DATA_PIN_ONBOARD 33

#define BTN_PIN 38

// I2S pins for INMP441 microphone (adjust for your board)
#define MIC_I2S_WS_PIN 37  // Word Select (LRCLK)
#define MIC_I2S_SD_PIN 35  // Serial Data (DIN)
#define MIC_I2S_CLK_PIN 36 // Serial Clock (BCLK)
#define MIC_I2S_CHANNEL fl::audio::AudioChannel::Left
#define MIC_SAMPLE_RATE 48000

#define EXT_I2S_WS_PIN 4  // Word Select (LRCLK)
#define EXT_I2S_SD_PIN 8  // Serial Data (DIN)
#define EXT_I2S_CLK_PIN 5 // Serial Clock (BCLK)
#define EXT_I2S_CHANNEL fl::audio::AudioChannel::Both
#define EXT_SAMPLE_RATE 44100

#define EXT_MCLK 1


static const char* TAG = "Main";

//Audio objects
// Global audio source (initialized in setup)
fl::shared_ptr<fl::audio::IInput> audioSource;
//Signal conditionner
fl::audio::SignalConditioner sConditioner;
//AutoGain
fl::audio::AutoGain autoGain;
//Audio reactive
fl::audio::Reactive audioReactive;
bool autoGainEnabled = false;
float audioGain = 1.0f;
bool audioConfigChanged = false;
int audioInput = -1;

CRGB onBoardLed;

CRGB leds[SCREEN_WIDTH * SCREEN_HEIGHT + 1];    //Do we need to allocate all 2D LEDs????

XYMap xymap = XYMap::constructWithUserFunction(SCREEN_WIDTH, SCREEN_HEIGHT, IT::itUserMapFunc);

ITSparks sparks(xymap);
FLParticles particles(NB_STRIP_LEDS, 2);
FLNoisePalette noisePalette(xymap);
ITVuMeter itVuMeter(xymap);
ITParticles itParticles(NB_STRIP_LEDS + 1, 5);

LedFXManager fxManager{SCREEN_WIDTH * SCREEN_HEIGHT};
// fl::FxEngine fxEngine(SCREEN_WIDTH * SCREEN_HEIGHT, false); //08/04/28 : Activating interpolation results in heap crashes

fl::audio::Config createMicAudioConfig(){
    fl::audio::ConfigI2S config(MIC_I2S_WS_PIN, MIC_I2S_SD_PIN, MIC_I2S_CLK_PIN, 0, MIC_I2S_CHANNEL, MIC_SAMPLE_RATE,
                         16, fl::audio::I2SCommFormat::Philips);
    fl::audio::Config out(config);
    out.setMicProfile(fl::audio::MicProfile::GenericMEMS);
    FastLED.add(out);
    return out;
}

void configureAudioInput(){
    int input;
    configuration.getAudioConfiguration(input, autoGainEnabled, audioGain);

    bool micInput = (input == 0);
    ESP_LOGI(TAG, "Configuring audio -> Gain : %f, Auto-gain : %u, Input : %s", audioGain, autoGainEnabled, (micInput ? "Mic" : "Line"));

    if(input != audioInput){


        //Destroy previously used audio objects
        audioSource.reset();

        // Initialize I2S Audio
        Serial.println("Initializing audio input...");
        fl::string errorMsg;
        if(micInput){
            // Create platform-specific audio configuration
            fl::audio::Config config =  createMicAudioConfig();
            audioSource = fl::audio::IInput::create(config, &errorMsg);
        }else{
            audioSource = fl::PCM1808::createPCM1808(EXT_SAMPLE_RATE);
        }

        if (!audioSource) {
            ESP_LOGE(TAG, "Failed to create audio source: %s", errorMsg.c_str());
            return;
        }

        // Start audio capture
        ESP_LOGI(TAG, "Starting audio capture...");
        audioSource->start();
        audioInput = input;

        //fl::audio::AudioManager::instance().add(fl::move(input));
    }

    //Configure signal conditionner
    fl::audio::SignalConditionerConfig scConfig;
    // scConfig.enableDCRemoval = config.enableSignalConditioning;
    // scConfig.enableSpikeFilter = config.enableSignalConditioning;
    // scConfig.enableNoiseGate = config.noiseGate && config.enableSignalConditioning;
    sConditioner.configure(scConfig);
    sConditioner.reset();

    //Configure AutoGain
    fl::audio::AutoGainConfig agcConfig;
    // agcConfig.preset = fl::audio::AGCPreset::AGCPreset_Lazy;
    autoGain.configure(agcConfig);
    autoGain.reset();

    //Configure audio reactive
    fl::audio::ReactiveConfig reactConfig;
    if(audioInput == 0){
        reactConfig.sampleRate = MIC_SAMPLE_RATE;
    }else{
        reactConfig.sampleRate = EXT_SAMPLE_RATE;
    }
    // Enable signal conditioning (DC removal, spike filter, noise gate)
    reactConfig.enableSignalConditioning = true;
    // Enable advanced beat tracking with BPM
    reactConfig.enableMusicalBeatDetection = false;
    // Enable noise floor tracking
    reactConfig.enableNoiseFloorTracking = true;
    //Enable per-band beat detection
    reactConfig.enableMultiBandBeats = true;
    reactConfig.enableMultiBand = true;

    audioReactive.begin(reactConfig);
}

void configurationChanged(DeviceConfiguration::Parameter param)
{
    if(param == DeviceConfiguration::Parameter::AUDIO){
        audioConfigChanged = true;
    }
}


/**
 * Runs in a FreeRTOS task to avoid stack overflow
 */
void fastLedTask(void* param){
    fl::u8 level = 0;
    fl::u32 lastFxSwitchTime = 0;
    float bassEnergy = 0.0f;
    bool lastBtn = true;
    bool btnUsed = false;

    unsigned long lastSample = ::micros();

    ::pinMode(17, OUTPUT);
    while(true){
        //Reconfigure audio if needed
        if(audioConfigChanged){
            configureAudioInput();
            audioConfigChanged = false;
        }

        fl::audio::Sample sample;
        //Get audio samples
        if(audioSource){
            sample = audioSource->read();
            if(sample){
                //Conditionner
                sample = sConditioner.processSample(sample);
                //Autogain
                if(autoGainEnabled){
                    sample = autoGain.process(sample);
                }else{
                    sample.applyGain(audioGain);
                }
                //Audio reactive
                audioReactive.processSample(sample);
                if(audioReactive.getSpectralFlux() > 1000.0f){ //audioReactive.isBeat()){
                    // Serial.println("Beat");
                    ::digitalWrite(17, 1);
                }else{
                    ::digitalWrite(17, 0);
                }
            }
        }

        // bool btnState = ::digitalRead(BTN_PIN);
        // if(!btnState && (btnState != lastBtn)){
        //     if(!fxManager.isAutoChange()){
        //         fxManager.nextFX();
        //     }
        //     fxManager.setAutoChange(!fxManager.isAutoChange());
        //     if(fxManager.isAutoChange()){
        //         ESP_LOGI(TAG, "Automatic FX switch");
        //     }else{
        //         ESP_LOGI(TAG, "No-automatic FX switch");
        //     }
        // }
        // lastBtn = btnState;
        // fxManager.draw(leds, (sample.isValid() ? &audioReactive : nullptr));

        // if(audioReactive.isBeat()){
        //     onBoardLed = CRGB::White;
        // }else{
        //     onBoardLed.fadeToBlackBy(50);
        // }

        // FastLED.show();
        delay(1);
    }
}

void setup()
{
    //Starts serial
    Serial.begin(115200);

    //Button pin
    ::pinMode(BTN_PIN, INPUT_PULLUP);

    //Loads configuration
    configuration.begin();

    //register the configuration change for audio settings
    configuration.registerListener(::configurationChanged);

    //Setup network
    NetworkConfigurator::startNetwork();

    const int iCount = 2*LETTER_WIDTH + LETTER_HEIGHT;
    FastLED.addLeds<WS2812B, DATA_PIN_I, GRB>(leds, 0, iCount)
        .setCorrection(LEDColorCorrection::TypicalLEDStrip);
    FastLED.addLeds<WS2812B, DATA_PIN_T, GRB>(leds, iCount, (LETTER_HEIGHT + LETTER_WIDTH))
        .setCorrection(LEDColorCorrection::TypicalLEDStrip);
    //Internal led to show beat detection
    FastLED.addLeds<SK6812, DATA_PIN_ONBOARD, GRB>(&onBoardLed, 1);

    FastLED.setBrightness(64);

    fxManager.registerFx(&noisePalette);
    fxManager.registerFx(&particles);
    fxManager.registerFx(&sparks);
    fxManager.registerFx(&itParticles);
    fxManager.registerFx(&itVuMeter);

    //Enable audio
    configureAudioInput();

    //Loads FX configuration
    JsonDocument doc;
    configuration.loadFXConfiguration(doc);
    fxManager.setFXConfigurations(doc, false);

    //Create the FreeRTOS OS with a stack of 16KB
    xTaskCreateUniversal(fastLedTask, "FastLed", 16*1024, nullptr, 1, NULL, 1);
    // xTaskCreate(fastLedTask, "FastLed", 16*1024, nullptr, 1, NULL);
}


void loop()
{
    vTaskDelete(NULL);
}

float getAudioGain(){
    if(autoGainEnabled){
        return autoGain.getGain();
    }else{
        return audioGain;
    }
}