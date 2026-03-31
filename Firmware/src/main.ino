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
#include "ITFill.h"

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

// I2S pins for INMP441 microphone (adjust for your board)
#define I2S_WS_PIN 37  // Word Select (LRCLK)
#define I2S_SD_PIN 35  // Serial Data (DIN)
#define I2S_CLK_PIN 36 // Serial Clock (BCLK)
#define I2S_CHANNEL fl::audio::AudioChannel::Left

static const char* TAG = "Main";

CRGB leds[SCREEN_WIDTH * SCREEN_HEIGHT];

XYMap xymap = XYMap::constructWithUserFunction(SCREEN_WIDTH, SCREEN_HEIGHT, IT::itUserMapFunc);

ITSparks sparks(xymap);
fl::Particles1d particles(SCREEN_WIDTH * SCREEN_HEIGHT, 1, 2);
fl::NoisePalette noisePalette(xymap);
ITFill itFill(xymap);
fl::FxEngine fxEngine(SCREEN_WIDTH * SCREEN_HEIGHT);

void setup()
{
    //Starts serial
    Serial.begin(115200);
#ifdef __EMSCRIPTEN__
    FastLED.addLeds<WS2812B, DATA_PIN_I, GRB>(leds, SCREEN_WIDTH * SCREEN_HEIGHT)
        .setCorrection(LEDColorCorrection::TypicalLEDStrip)
        .setScreenMap(xymap);
#else
    const int iCount = 2*LETTER_WIDTH + LETTER_HEIGHT;
    FastLED.addLeds<WS2812B, DATA_PIN_I, GRB>(leds + 1, iCount)
        .setCorrection(LEDColorCorrection::TypicalLEDStrip)
        .setScreenMap(xymap);
    FastLED.addLeds<WS2812B, DATA_PIN_T, GRB>(leds + iCount + 1, (LETTER_HEIGHT + LETTER_WIDTH))
        .setCorrection(LEDColorCorrection::TypicalLEDStrip)
      .setScreenMap(xymap);
#endif
    FastLED.setBrightness(127);
    fxEngine.addFx(sparks);
    fxEngine.addFx(particles);
    fxEngine.addFx(noisePalette);
    fxEngine.addFx(itFill);

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
    // audioSource->setGain(10.0);
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
    audioReactive.begin(reactConfig);

}

fl::u8 level = 0;
fl::u32 lastSpawnTime = 0;
void loop()
{
    //Get audio samples
    fl::audio::Sample sample = audioSource->read();
    audioReactive.processSample(sample);

    // fl::u32 now = fl::millis();
    // if((now - lastSpawnTime) > 1000){
    //     particles.spawnRandomParticle();
    // }
    // EVERY_N_MILLISECONDS(100) { Serial.println("Hello world!"); }
    // itFill.draw(fl::Fx::DrawContext(fl::millis(), leds));
    // particles.draw(fl::Fx::DrawContext(fl::millis(), leds));
    // noisePalette.setSpeed(5);
    // noisePalette.setScale(5);
    EVERY_N_MILLISECONDS(10000) { noisePalette.changeToRandomPalette();
            particles.spawnRandomParticle();
        }
    EVERY_N_MILLISECONDS(100) { itFill.setLevel(level += 15); }
    // noisePalette.draw(fl::Fx::DrawContext(fl::millis(), leds));
    // FastLED.showColor(CRGB::BlueViolet);
    EVERY_N_SECONDS(5) {
        fxEngine.nextFx(500);
    }
    fxEngine.draw(fl::millis(), leds);
    FastLED.show();
}