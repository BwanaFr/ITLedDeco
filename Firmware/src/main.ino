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

#include "ITSparks.h"

using namespace fl;

//Flag to know if we are in simulation mode
// #ifdef __EMSCRIPTEN__
#define FS_WASM 1
// #endif

#ifdef FS_WASM
#define TOTAL_LEDS (SCREEN_WIDTH * (SCREEN_HEIGHT + 2))
#else
#define TOTAL_LEDS (3*LETTER_WIDTH + 2*LETTER_HEIGHT)
#endif

#define DATA_PIN 48
#define BLUR_AMOUNT 1

static const char* TAG = "Main";

CRGB leds[TOTAL_LEDS];

#ifdef FS_WASM
fl::UINumberField x_cur("Current X", 0, 0, 65535);
fl::UINumberField y_cur("Current Y", 0, 0, 65535);
fl::UINumberField i_cur("Current index", 0, 0, 65535);
XYMap xymap(SCREEN_WIDTH, SCREEN_HEIGHT, false);
XYMap itXYMap = XYMap::constructWithUserFunction(SCREEN_WIDTH, SCREEN_HEIGHT, IT::itUserMapFunc);
#else
XYMap xymap = XYMap::constructWithUserFunction(SCREEN_WIDTH, SCREEN_HEIGHT, IT::itUserMapFunc);
#endif //FS_WASM


/**
 * NoisePalette noisePalette1(xyMap);
NoisePalette noisePalette2(xyMap);
FxEngine fxEngine(NUM_LEDS);
 */

void setup()
{
    //Starts serial
    Serial.begin(115200);
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, TOTAL_LEDS)
      .setScreenMap(xymap);
    FastLED.setBrightness(127);
}

ITSparks sparks(xymap);

void loop()
{
//     static int x = 0;
//     static int y = 0;
//     static CRGB c = CRGB(0, 0, 0);
//     blur2d(leds, SCREEN_WIDTH, SCREEN_HEIGHT, BLUR_AMOUNT, xymap);
//     EVERY_N_MILLISECONDS(50) {
//         uint8_t r = random(255);
//         uint8_t g = random(255);
//         uint8_t b = random(255);
//         c = CRGB(r, g, b);
// #ifdef FS_WASM
//         u16 realIndex = xymap(x, y);
//         if(realIndex == NO_LED){
//             c = CRGB(0,0,0);
//         }
// #endif
//         u16 index = xymap(x, y);
//         if(index != NO_LED){
//             leds[index] = c;
//         }
//         if(++x >= LETTER_WIDTH*2){
//             x = 0;
//             if(++y >= LETTER_HEIGHT){
//                 y = 0;
//             }
//         }
// #ifdef FS_WASM
//         x_cur = x;
//         y_cur = y;
//         i_cur = index;
// #endif
//     }
    sparks.draw(fl::Fx::DrawContext(fl::millis(), leds));
#ifdef FS_WASM
    //Clear non-mapped LEDS
    for(int x=0;x<SCREEN_WIDTH;++x){
        for(int y=0;y<SCREEN_HEIGHT;++y){
            u16 index = xymap(x,y);
            u16 ledIndex = itXYMap(x,y);
            if(ledIndex == NO_LED){
                leds[index] = CRGB(0,0,0);
            }
        }
    }
#endif
    FastLED.show();
}