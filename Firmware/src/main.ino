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
#include "fl/gfx/xymap.h"
#include "fl/fx/1d/particles.h"
#include "fl/fx/2d/noisepalette.h"

#include "ITSparks.h"
#include "ITFill.h"

using namespace fl;

//Flag to know if we are in simulation mode
// #ifdef __EMSCRIPTEN__
//#define FS_WASM 1
// #endif

#define DATA_PIN_I 45
#define DATA_PIN_T 46
#define BLUR_AMOUNT 1

static const char* TAG = "Main";

CRGB leds[SCREEN_WIDTH * SCREEN_HEIGHT];

XYMap xymap = XYMap::constructWithUserFunction(SCREEN_WIDTH, SCREEN_HEIGHT, IT::itUserMapFunc);


/**
 * NoisePalette noisePalette1(xyMap);
NoisePalette noisePalette2(xyMap);
FxEngine fxEngine(NUM_LEDS);
 */


void setup()
{
    //Starts serial
    Serial.begin(115200);
    const int iCount = 2*LETTER_WIDTH + LETTER_HEIGHT;
    FastLED.addLeds<WS2812B, DATA_PIN_I, GRB>(leds, iCount)
        .setCorrection(LEDColorCorrection::TypicalLEDStrip)
        .setScreenMap(xymap);
    FastLED.addLeds<WS2812B, DATA_PIN_T, GRB>(leds + iCount, (LETTER_HEIGHT + LETTER_WIDTH))
        .setCorrection(LEDColorCorrection::TypicalLEDStrip)
      .setScreenMap(xymap);
    FastLED.setBrightness(127);
    FastLED.clear(true);
    delay(2000);
    Serial.printf("LEDS size : %u (%u)\n", sizeof(leds)/sizeof(CRGB), SCREEN_WIDTH * SCREEN_HEIGHT);
}

ITSparks sparks(xymap);
fl::Particles1d particles(NB_STRIP_LEDS, 1, 2);
fl::NoisePalette noisePalette(xymap);
ITFill itFill(xymap);

fl::u8 level = 0;
fl::u32 lastSpawnTime = 0;
void loop()
{
    // fl::u32 now = fl::millis();
    // if((now - lastSpawnTime) > 1000){
    //     particles.spawnRandomParticle();
    // }
    // EVERY_N_MILLISECONDS(100) { Serial.println("Hello world!"); }
    itFill.draw(fl::Fx::DrawContext(fl::millis(), leds));
    // particles.draw(fl::Fx::DrawContext(fl::millis(), leds));
    // noisePalette.setSpeed(5);
    // noisePalette.setScale(5);
    // EVERY_N_MILLISECONDS(10000) { noisePalette.changeToRandomPalette(); }
    EVERY_N_MILLISECONDS(100) { itFill.setLevel(level += 10); }
    // noisePalette.draw(fl::Fx::DrawContext(fl::millis(), leds));
    // FastLED.showColor(CRGB::BlueViolet);
    FastLED.show();
}