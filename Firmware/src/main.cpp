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

#include "fl/audio/audio_reactive.h"

#include "FLNoisePalette.hpp"
#include "FLParticles.hpp"
#include "ITSparks.hpp"
#include "ITVuMeter.hpp"
#include "ITParticles.hpp"
#include "ITVuMeter.hpp"

#include "NetworkConfigurator.hpp"
#include "Configuration.hpp"

#include "AudioInput.hpp"
#include "I2SAudioInput.hpp"
#include "AudioReactive.hpp"

using namespace fl;


#ifdef S3_CAM                   // Build for Aliexpress S3 CAM board
#define DATA_PIN_I 45           // I Led data pin
#define DATA_PIN_T 46           // T Led data pin
#define DATA_PIN_ONBOARD 33     // Onboard LED

#define BTN_PIN 38              // Onboard button
#define LED_PIN 34              // Onboard led

// I2S pins for INMP441 microphone (adjust for your board)
#define MIC_I2S_WS_PIN GPIO_NUM_37          // Word Select (LRCLK)
#define MIC_I2S_SD_PIN GPIO_NUM_35          // Serial Data (DIN)
#define MIC_I2S_CLK_PIN GPIO_NUM_36         // Serial Clock (BCLK)

//External ADC
#define EXT_I2S_BCLK_PIN GPIO_NUM_6         // Serial Clock (BCLK)
#define EXT_I2S_WS_PIN GPIO_NUM_4           // Word Select (LRCLK)
#define EXT_I2S_SD_PIN GPIO_NUM_5           // Serial Data (DIN)
#define EXT_I2S_MCLK_PIN GPIO_NUM_NC        // Masterc lock (MCLK)

#elif defined(T7_S3)
#define DATA_PIN_I 10           // I Led data pin
#define DATA_PIN_T 11           // T Led data pin

#define BTN_PIN 0               // Onboard button
#define LED_PIN 17              // Onboard led

// I2S pins for INMP441 microphone (adjust for your board)
#define MIC_I2S_WS_PIN GPIO_NUM_37  // Word Select (LRCLK)
#define MIC_I2S_SD_PIN GPIO_NUM_35  // Serial Data (DIN)
#define MIC_I2S_CLK_PIN GPIO_NUM_36 // Serial Clock (BCLK)

//External ADC
#define EXT_I2S_BCLK_PIN GPIO_NUM_5     // Serial Clock (BCLK)
#define EXT_I2S_WS_PIN GPIO_NUM_4       // Word Select (LRCLK)
#define EXT_I2S_SD_PIN GPIO_NUM_8       // Serial Data (DIN)
#define EXT_I2S_MCLK_PIN GPIO_NUM_1     // Masterc lock (MCLK)
#endif

static const char* TAG = "Main";

//Audio objects for sound-reactive tentative
AudioReactive audioReactive{256};
AudioInput* audioInput = nullptr;
int currentAudioInput = -1;
bool audioConfigChanged = false;

CRGB onBoardLed;

CRGB leds[SCREEN_WIDTH * SCREEN_HEIGHT + 1];    //Do we need to allocate all 2D LEDs????

XYMap xymap = XYMap::constructWithUserFunction(SCREEN_WIDTH, SCREEN_HEIGHT, IT::itUserMapFunc);

ITSparks sparks(xymap);
FLParticles particles(NB_STRIP_LEDS, 2);
FLNoisePalette noisePalette(xymap);
ITVuMeter itVuMeter(xymap);
ITParticles itParticles(NB_STRIP_LEDS + 1, 5);

LedFXManager fxManager{SCREEN_WIDTH * SCREEN_HEIGHT};

void configureAudioInput(){
    int input;
    bool autoGainEnabled;
    float audioGain;
    configuration.getAudioConfiguration(input, autoGainEnabled, audioGain);

    bool micInput = (input == 0);
    ESP_LOGI(TAG, "Configuring audio -> Gain : %f, Auto-gain : %u, Input : %s", audioGain, autoGainEnabled, (micInput ? "Mic" : "Line"));

    if(input != currentAudioInput){
        //Destroy previously used audio objects
        if(audioInput){
            delete audioInput;
            audioInput = nullptr;
        }

        // Initialize I2S Audio
        if(input == 1){
            //PCM1808 input
            audioInput = new I2SAudioInput(22050, 512, EXT_I2S_BCLK_PIN, EXT_I2S_WS_PIN, EXT_I2S_SD_PIN, EXT_I2S_MCLK_PIN);
        }

        // Start audio capture
        if(audioInput){
            audioInput->start();
        }
        currentAudioInput = input;
    }
}

void configurationChanged(DeviceConfiguration::Parameter param)
{
    if(param == DeviceConfiguration::Parameter::AUDIO){
        audioConfigChanged = true;
    }
}

/**
 * Audio reactive task
 */
void audioReactiveTask(void* param){
    delay(5000);
    //Enable audio
    configureAudioInput();
    while(true){
        //Reconfigure audio if needed
        if(audioConfigChanged){
            configureAudioInput();
            audioConfigChanged = false;
        }
        if(audioInput){
            const AudioReactiveData* data = audioReactive.process(audioInput);
            if(data){
                // ESP_LOGI("Audio", "RMS: %f", data->signalRMS);
#ifdef LED_PIN
                ::digitalWrite(LED_PIN, data->beatDetected);
#endif
            }
        }
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

    while(true){
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
        fxManager.draw(leds, audioReactive.getData());  //TODO: Change the audioreactive part

        FastLED.show();
        delay(10);
    }
}

void setup()
{
    //Starts serial
    Serial.begin(115200);

    //Button pin
    ::pinMode(BTN_PIN, INPUT_PULLUP);

    //LED
#ifdef LED_PIN
    ::pinMode(LED_PIN, OUTPUT);
#endif

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
#ifdef DATA_PIN_ONBOARD
    //Internal led to show beat detection
    FastLED.addLeds<SK6812, DATA_PIN_ONBOARD, GRB>(&onBoardLed, 1);
#endif
    FastLED.setBrightness(64);

    fxManager.registerFx(&noisePalette);
    fxManager.registerFx(&particles);
    fxManager.registerFx(&sparks);
    fxManager.registerFx(&itParticles);
    fxManager.registerFx(&itVuMeter);

    //Loads FX configuration
    JsonDocument doc;
    configuration.loadFXConfiguration(doc);
    fxManager.setFXConfigurations(doc, false);

    //Removes logs from FXManager
    esp_log_level_set("LedFXManager", ESP_LOG_WARN);

    //Create the FreeRTOS OS with a stack of 16KB
    xTaskCreateUniversal(fastLedTask, "FastLed", 16*1024, nullptr, 1, NULL, 1);

    //Create a task for processing audio input
    xTaskCreate(audioReactiveTask, "Audio", 4096, nullptr, 1, NULL);
}

void loop()
{
    vTaskDelete(NULL);
}

float getAudioGain(){
    // if(autoGainEnabled){
    //     return autoGain.getGain();
    // }else{
    //     return audioGain;
    // }
    return 0.0f;
}