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

#include "FastLED.h"
#include <Arduino.h>
//#include "esp_log.h"
#include "ITLedMap.h"
#include "fl/ui.h"
#include "fl/fx/fx_engine.h"
#include "fl/fx/1d/particles.h"
#include "fl/fx/2d/noisepalette.h"
#include "fl/audio/audio_reactive.h"

#include "ITSparks.h"
#include "ITVuMeter.h"
#include "ITParticles.h"

using namespace fl;

//Audio objects
// Global audio source (initialized in setup)
fl::shared_ptr<fl::audio::IInput> audioSource;
//Audio reactive
fl::audio::Reactive audioReactive;


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

CRGB onBoardLed;

// #ifdef __EMSCRIPTEN__
CRGB leds[SCREEN_WIDTH * SCREEN_HEIGHT + 1];
// #else
// CRGB leds[NB_STRIP_LEDS + 1];
// #endif

XYMap xymap = XYMap::constructWithUserFunction(SCREEN_WIDTH, SCREEN_HEIGHT, IT::itUserMapFunc);

ITSparks sparks(xymap);
fl::Particles1d particles(NB_STRIP_LEDS + 1, 2, 2);
fl::NoisePalette noisePalette(xymap);
ITVuMeter ITVuMeter(xymap);
ITParticles itParticles(NB_STRIP_LEDS + 1);
fl::FxEngine fxEngine(SCREEN_WIDTH * SCREEN_HEIGHT, false); //08/04/28 : Activating interpolation results in heap crashes

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
        //Get audio samples
        if(audioSource){
            fl::audio::Sample sample = audioSource->read();
            audioReactive.processSample(sample);
        }

        EVERY_N_MILLISECONDS(200) {
            particles.spawnRandomParticle();
        }

        EVERY_N_MILLISECONDS(10000) {
            noisePalette.changeToRandomPalette();
            ITVuMeter.setRandomPalette();
        }
        float be = audioReactive.getBassEnergy();
        if(be > bassEnergy){
            bassEnergy = be;
            Serial.printf("New BE : %f\n", be);
        }
        fl::u8 level = static_cast<fl::u8>(fl::map_range_clamped(be, 0.0f, 40.0f, 0, 255));
        ITVuMeter.setLevel(level);

        bool btnState = ::digitalRead(BTN_PIN);
        if(!btnState && (btnState != lastBtn)){
            lastFxSwitchTime = ::millis();
            fxEngine.nextFx(50);
            btnUsed = true;
            Serial.printf("Press : Next FX -> %s\n", fxEngine.getFx(fxEngine.getCurrentFxId())->fxName().c_str());
        }
        lastBtn = btnState;

        fxEngine.draw(fl::millis(), leds);

        if(!btnUsed && ((::millis() - lastFxSwitchTime) >= 10000)){
            lastFxSwitchTime = ::millis();
            fxEngine.nextFx(500);
            Serial.printf("Next FX -> %s\n", fxEngine.getFx(fxEngine.getCurrentFxId())->fxName().c_str());
        }

        if(audioReactive.isBeat()){
            onBoardLed = CRGB::White;
        }else if(audioReactive.getSmoothedData().bassEnergy > (bassEnergy/2.0)){
            itParticles.spawnRandomParticle();
            float bass = audioReactive.getBass();
            bass = (bass < 0.0f) ? 0.0f : ((bass > 255.0f) ? 255.0f : bass);
            onBoardLed = ColorFromPalette(RainbowColors_p, static_cast<fl::u8>(bass));
        }else{
            onBoardLed.fadeToBlackBy(10);
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

#ifdef __EMSCRIPTEN__
    FastLED.addLeds<WS2812B, DATA_PIN_I, GRB>(leds, SCREEN_WIDTH * SCREEN_HEIGHT)
        .setCorrection(LEDColorCorrection::TypicalLEDStrip)
        .setScreenMap(xymap);
#else
    const int iCount = 2*LETTER_WIDTH + LETTER_HEIGHT;
    FastLED.addLeds<WS2812B, DATA_PIN_I, GRB>(leds, 0, iCount)
        .setCorrection(LEDColorCorrection::TypicalLEDStrip);
    FastLED.addLeds<WS2812B, DATA_PIN_T, GRB>(leds, iCount, (LETTER_HEIGHT + LETTER_WIDTH))
        .setCorrection(LEDColorCorrection::TypicalLEDStrip);
    //Internal led to show beat detection
    FastLED.addLeds<SK6812, DATA_PIN_ONBOARD, GRB>(&onBoardLed, 1);
#endif
    FastLED.setBrightness(64);
    fxEngine.addFx(sparks);
    fxEngine.addFx(particles);
    fxEngine.addFx(noisePalette);
    fxEngine.addFx(ITVuMeter);
    fxEngine.addFx(itParticles);

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
    // audioSource->setGain(2.0);
    // Start audio capture
    Serial.println("Starting audio capture...");
    audioSource->start();

    //Configure audio reactive
    fl::audio::ReactiveConfig reactConfig;
    // Enable signal conditioning (DC removal, spike filter, noise gate)
    reactConfig.enableSignalConditioning = true;
    // Enable advanced beat tracking with BPM
    reactConfig.enableMusicalBeatDetection = true;
    // Enable noise floor tracking
    reactConfig.enableNoiseFloorTracking = true;
    //Enable per-band beat detection
    reactConfig.enableMultiBandBeats = true;

    reactConfig.gain = 255;
    audioReactive.begin(reactConfig);

    xTaskCreate(fastLedTask, "FastLed", 16*1024, nullptr, 1, NULL);
}


void loop()
{
    vTaskDelete(NULL);
}