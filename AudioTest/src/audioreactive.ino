// @filter: (mem is high) and ((platform is teensy) or (platform is esp32))

// Audio Reactive LEDs using FastLED.add(AudioConfig)
// Demonstrates the auto-pumped audio pipeline: mic → processor → callbacks → LEDs

#include "FastLED.h"

#define NUM_LEDS 1
#define LED_PIN 33

// I2S pins for INMP441 microphone (adjust for your board)
#define I2S_WS_PIN 37  // Word Select (LRCLK)
#define I2S_SD_PIN 35  // Serial Data (DIN)
#define I2S_CLK_PIN 36 // Serial Clock (BCLK)
#define I2S_CHANNEL fl::Left

CRGB leds[NUM_LEDS];
// Global audio source (initialized in setup)
fl::shared_ptr<fl::IAudioInput> audioSource;

// Audio processor with beat detection
fl::AudioProcessor audioProcessor;

void setup() {
    Serial.begin(115200);
    FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(128);

    // Create platform-specific audio configuration
    fl::AudioConfig config =  fl::AudioConfig::CreateInmp441(I2S_WS_PIN, I2S_SD_PIN, I2S_CLK_PIN, I2S_CHANNEL);
    config.setMicProfile(fl::MicProfile::GenericMEMS);

    // Initialize I2S Audio
    Serial.println("Initializing audio input...");
    fl::string errorMsg;
    audioSource = fl::IAudioInput::create(config, &errorMsg);

    if (!audioSource) {
        Serial.print("Failed to create audio source: ");
        Serial.println(errorMsg.c_str());
        return;
    }
    // audioSource->setGain(10.0);
    // Start audio capture
    Serial.println("Starting audio capture...");
    audioSource->start();

    // fl::SignalConditionerConfig scConfig;
    // scConfig.spikeThreshold = 15000;
    // scConfig.noiseGateOpenThreshold = 1000;
    // scConfig.noiseGateCloseThreshold = 700;
    // audioProcessor.configureSignalConditioner(scConfig);

    audioProcessor.setNoiseFloorTrackingEnabled(true);

    // Flash white on every beat
    audioProcessor.onBeat([] {
        Serial.println("On beat");
        fill_solid(leds, NUM_LEDS, CRGB::White);
    });

    // Map bass level to hue
    // audioProcessor.onBass([](float level) {
    //     if(level > 0.001f){
    //         Serial.printf("On bass : %f\n", level);
    //         fill_solid(leds, NUM_LEDS, CRGB::Red);
    //         uint8_t hue = static_cast<uint8_t>(level * 160);
    //         fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
    //     }
    // });

    audioProcessor.onPeak([](float energy) {
        if(((int)energy > 0)){
            Serial.printf("Peak : %f\n", energy);
            uint8_t hue = static_cast<uint8_t>(energy/1000.0f * 160);
            fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
        }
    });

    // Dim to black on silence
    audioProcessor.onSilenceStart([] {
        Serial.println("On silence start");
        fill_solid(leds, NUM_LEDS, CRGB::Black);
    });
}

void loop() {
        // Get audio sample
    fl::AudioSample sample = audioSource->read();

    if (sample.isValid()) {
        // Process audio through the audio processor
        audioProcessor.update(sample);
    }

    fadeToBlackBy(leds, NUM_LEDS, 20);
    FastLED.show();
}