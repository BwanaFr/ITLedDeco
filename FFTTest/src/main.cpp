
#include <Arduino.h>
#include "esp_log.h"

#include "PCM1808.hpp"
#include "esp32s3_dsp.h"
#include "esp_heap_caps.h"

#define FFT_SIZE  512
#define FFT_SAMPLES 512
#define SAMPLE_RATE 22050

ESP32S3_FFT fft;
static const char* TAG = "Main";

PCM1808 pcm{SAMPLE_RATE};

void setup()
{
    //Starts serial
    Serial.begin(115200);

    // get info from fft to set up buffers
    fft_table_t *fft_table = fft.init(FFT_SIZE, FFT_SAMPLES, SPECTRAL_AVERAGE);


    pcm.start();
}

unsigned long bCount = 0;
bool wasBeat = false;
void loop()
{
    if(pcm.read()){
        size_t size = 0;
        const float* buffer = pcm.getBuffer(size);
        float output_bufr[FFT_SAMPLES];
        unsigned long start = millis();

        fft.compute(buffer, output_bufr);
        unsigned long fftTime = millis() - start;
        int count = 0;
        float freqPerBin = SAMPLE_RATE/FFT_SAMPLES;
        int maxFreq = 800/freqPerBin;
        for(int i=0;i<maxFreq;++i){
            if(output_bufr[i] >= 90000.0f){
               ++count;
            }
        }
        if(count >= maxFreq/3){
            if(!wasBeat){
                Serial.printf("[%u] BEAT\n", ++bCount);
                wasBeat = true;
            }
        }else{
            wasBeat = false;
        }
        // if(prev > 1000.f){
        //     float freqPerBin = SAMPLE_RATE/FFT_SAMPLES;
        //     Serial.printf("Max at %fHz (%f) in %ums\n", (max*freqPerBin), prev, fftTime);
        // }
    }
}
