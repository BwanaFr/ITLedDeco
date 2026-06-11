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
#define LED_PIN_INVERTED true
// I2S pins for INMP441 microphone (adjust for your board)
#define MIC_I2S_WS_PIN GPIO_NUM_37          // Word Select (LRCLK)
#define MIC_I2S_SD_PIN GPIO_NUM_35          // Serial Data (DIN)
#define MIC_I2S_CLK_PIN GPIO_NUM_36         // Serial Clock (BCLK)

//External ADC
// #define EXT_I2S_BCLK_PIN GPIO_NUM_6         // Serial Clock (BCLK)
// #define EXT_I2S_WS_PIN GPIO_NUM_4           // Word Select (LRCLK)
// #define EXT_I2S_SD_PIN GPIO_NUM_5           // Serial Data (DIN)
// #define EXT_I2S_MCLK_PIN GPIO_NUM_NC        // Masterc lock (MCLK)

#elif defined(T7_S3)
#define DATA_PIN_I 10           // I Led data pin
#define DATA_PIN_T 11           // T Led data pin

#define BTN_PIN 0               // Onboard button
#define LED_PIN 17              // Onboard led

// // I2S pins for INMP441 microphone (adjust for your board)
// #define MIC_I2S_WS_PIN GPIO_NUM_37  // Word Select (LRCLK)
// #define MIC_I2S_SD_PIN GPIO_NUM_35  // Serial Data (DIN)
// #define MIC_I2S_CLK_PIN GPIO_NUM_36 // Serial Clock (BCLK)

//External ADC
#define EXT_I2S_BCLK_PIN GPIO_NUM_5     // Serial Clock (BCLK)
#define EXT_I2S_WS_PIN GPIO_NUM_4       // Word Select (LRCLK)
#define EXT_I2S_SD_PIN GPIO_NUM_8       // Serial Data (DIN)
#define EXT_I2S_MCLK_PIN GPIO_NUM_1     // Masterc lock (MCLK)
#endif

//Updates LED at 50Hz
#define LED_UPDATE_PERIOD_MS 1

static const char* TAG = "Main";

//Audio objects for sound-reactive tentative
AudioReactive audioReactive{256};
AudioInput* audioInput = nullptr;
int currentAudioInput = -1;
bool audioConfigChanged = false;

#ifdef DATA_PIN_ONBOARD
//Optionnal onboard LED
CRGB onBoardLed;
#endif

//Leds values (+1 to have a default trash LED for mapping)
//We are (don't know why, to be seen in FastLed code) obliged to allocate the full 2D array
CRGB leds[NB_STRIP_LEDS + 1]; //SCREEN_WIDTH * SCREEN_HEIGHT + 1];

XYMap xymap = XYMap::constructWithUserFunction(SCREEN_WIDTH, SCREEN_HEIGHT, IT::itUserMapFunc);

//Effects used by this
ITSparks sparks(xymap);
FLParticles particles(NB_STRIP_LEDS, 2);
FLNoisePalette noisePalette(xymap);
ITVuMeter itVuMeter(xymap);
ITParticles itParticles(NB_STRIP_LEDS, 5);
//Our FX manager to switch between effects
LedFXManager fxManager{NB_STRIP_LEDS + 1}; //SCREEN_WIDTH * SCREEN_HEIGHT};

void configureAudioInput(){
    int input;
    bool autoGainEnabled;
    float audioGain;
    configuration.getAudioConfiguration(input, audioGain);

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
#ifdef EXT_I2S_BCLK_PIN
            audioInput = new I2SAudioInput(22050, 512, EXT_I2S_BCLK_PIN, EXT_I2S_WS_PIN, EXT_I2S_SD_PIN, EXT_I2S_MCLK_PIN);
#endif
        }else if(input == 0){
            //Mic input
#ifdef MIC_I2S_WS_PIN
            audioInput = new I2SAudioInput(22050, 512, MIC_I2S_CLK_PIN, MIC_I2S_WS_PIN, MIC_I2S_SD_PIN);
#endif
        }

        // Start audio capture
        if(audioInput){
            audioInput->start();
        }
        currentAudioInput = input;
    }
    if(audioInput){
        audioInput->setGain(audioGain);
    }
}

void configurationChanged(DeviceConfiguration::Parameter param)
{
    if(param == DeviceConfiguration::Parameter::AUDIO){
        audioConfigChanged = true;
    }else if(param == DeviceConfiguration::Parameter::LED_BRIGHTNESS){
        ESP_LOGI(TAG, "Setting brightness to %u", configuration.getLEDBrightness());
        FastLED.setBrightness(configuration.getLEDBrightness());
    }
}

/**
 * Audio reactive task
 */
void audioReactiveTask(void* param){
    //Enable audio
    configureAudioInput();
    while(true){
        //Reconfigure audio if needed
        if(audioConfigChanged){
            configureAudioInput();
            audioConfigChanged = false;
        }
        if(audioInput){
            //Calling process will wait for samples to be ready on ADC
            const AudioReactiveData* data = audioReactive.process(audioInput);
            if(data){
                // ESP_LOGI("Audio", "RMS: %f", data->signalRMS);
#ifdef LED_PIN
#ifdef LED_PIN_INVERTED
                ::digitalWrite(LED_PIN, !data->beatDetected);
#else
                ::digitalWrite(LED_PIN, data->beatDetected);
#endif
#endif
#ifdef DATA_PIN_ONBOARD
                if(data->beatDetected){
                    onBoardLed = CRGB::White;
                }else{
                    onBoardLed.fadeToBlackBy(80);
                }
#endif
            }
        }else{
            //Do nothing, yield the CPU
            delay(5);
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

    unsigned long lastSample = ::millis();

    const TickType_t updateRateMs = LED_UPDATE_PERIOD_MS/portTICK_PERIOD_MS;
    while(true){

        TickType_t lastWakeTime = xTaskGetTickCount();
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
        lastSample = ::millis();
        fxManager.draw(leds, audioReactive.getData());  //This draw takes 70ms????
        FastLED.show();                                 //Takes 11 ms for full configuration

        //Try to keep a 50Hz update rate
        xTaskDelayUntil(&lastWakeTime, updateRateMs);
    }
}

void setup()
{
    //Starts serial
    Serial.begin(115200);
    ::delay(5000);

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

    //Adds LEDs to FastLED
    const int iCount = 2*LETTER_WIDTH + LETTER_HEIGHT;
    FastLED.addLeds<WS2812B, DATA_PIN_I, GRB>(leds, 0, iCount)
        .setCorrection(LEDColorCorrection::TypicalLEDStrip);
    FastLED.addLeds<WS2812B, DATA_PIN_T, GRB>(leds, iCount, (LETTER_HEIGHT + LETTER_WIDTH))
        .setCorrection(LEDColorCorrection::TypicalLEDStrip);
#ifdef DATA_PIN_ONBOARD
    //Internal led to show beat detection
    FastLED.addLeds<SK6812, DATA_PIN_ONBOARD, GRB>(&onBoardLed, 1);
#endif
    FastLED.setBrightness(configuration.getLEDBrightness());

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

    //Create the FreeRTOS OS with a stack of 4KB (seems to fit)
    xTaskCreateUniversal(fastLedTask, "FastLed", 4096, nullptr, 1, NULL, 1);

    //Create a task for processing audio input
    xTaskCreate(audioReactiveTask, "Audio", 4096, nullptr, 1, NULL);
}

void loop()
{
    vTaskDelete(NULL);
}
