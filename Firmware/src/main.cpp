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

using namespace fl;


//Flag to know if we are in simulation mode
// #ifdef __EMSCRIPTEN__
//#define FS_WASM 1
// #endif

#define DATA_PIN_I 45
#define DATA_PIN_T 46
#define DATA_PIN_ONBOARD 33

#define BTN_PIN 38

// I2S pins for INMP441 microphone (adjust for your board)
#define I2S_WS_PIN 37  // Word Select (LRCLK)
#define I2S_SD_PIN 35  // Serial Data (DIN)
#define I2S_CLK_PIN 36 // Serial Clock (BCLK)
#define I2S_CHANNEL fl::audio::AudioChannel::Left

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

/**
 * Runs in a FreeRTOS task to avoid stack overflow
 */
void fastLedTask(void* param){
    fl::u8 level = 0;
    fl::u32 lastFxSwitchTime = 0;
    float bassEnergy = 0.0f;
    bool lastBtn = true;
    bool btnUsed = false;

    while(true){
        fl::audio::Sample sample;
        //Get audio samples
        if(audioSource){
            sample = audioSource->read();
            //Conditionner
            sample = sConditioner.processSample(sample);
            //Autogain
            sample = autoGain.process(sample);
            //Audio reactive
            audioReactive.processSample(sample);
        }

        // EVERY_N_MILLISECONDS(10000) {
        //     noisePalette.changeToRandomPalette();
        //     ITVuMeter.setRandomPalette();
        // }


        bool btnState = ::digitalRead(BTN_PIN);
        if(!btnState && (btnState != lastBtn)){
            if(!fxManager.isAutoChange()){
                fxManager.nextFX();
            }
            fxManager.setAutoChange(!fxManager.isAutoChange());
            if(fxManager.isAutoChange()){
                ESP_LOGI(TAG, "Automatic FX switch");
            }else{
                ESP_LOGI(TAG, "No-automatic FX switch");
            }
        }
        lastBtn = btnState;
        fxManager.draw(leds, (sample.isValid() ? &audioReactive : nullptr));

        if(audioReactive.isBeat()){
            onBoardLed = CRGB::White;
        }else{
            onBoardLed.fadeToBlackBy(50);
        }

        FastLED.show();
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
    // Create platform-specific audio configuration
    fl::audio::Config config =  fl::audio::Config::CreateInmp441(I2S_WS_PIN, I2S_SD_PIN, I2S_CLK_PIN, I2S_CHANNEL);
    config.setMicProfile(fl::audio::MicProfile::GenericMEMS);

    // Initialize I2S Audio
    Serial.println("Initializing audio input...");
    fl::string errorMsg;
    audioSource = fl::audio::IInput::create(config, &errorMsg);

    if (!audioSource) {
        Serial.print("Failed to create audio source: ");
        Serial.println(errorMsg.c_str());
        return;
    }
    audioSource->setGain(2.0);
    // Start audio capture
    Serial.println("Starting audio capture...");
    audioSource->start();

    //Configure signal conditionner
    fl::audio::SignalConditionerConfig scConfig;
    // scConfig.enableDCRemoval = config.enableSignalConditioning;
    // scConfig.enableSpikeFilter = config.enableSignalConditioning;
    // scConfig.enableNoiseGate = config.noiseGate && config.enableSignalConditioning;
    sConditioner.configure(scConfig);
    sConditioner.reset();

    //Configure AutoGain
    fl::audio::AutoGainConfig agcConfig;
    agcConfig.preset = fl::audio::AGCPreset::AGCPreset_Lazy;
    autoGain.configure(agcConfig);
    autoGain.reset();

    //Configure audio reactive
    fl::audio::ReactiveConfig reactConfig;
    // Enable signal conditioning (DC removal, spike filter, noise gate)
    reactConfig.enableSignalConditioning = false;
    // Enable advanced beat tracking with BPM
    reactConfig.enableMusicalBeatDetection = true;
    // Enable noise floor tracking
    reactConfig.enableNoiseFloorTracking = false;
    //Enable per-band beat detection
    reactConfig.enableMultiBandBeats = true;

    audioReactive.begin(reactConfig);

    //Create the FreeRTOS OS with a stack of 16KB
    xTaskCreate(fastLedTask, "FastLed", 16*1024, nullptr, 1, NULL);
}


void loop()
{
    vTaskDelete(NULL);
}