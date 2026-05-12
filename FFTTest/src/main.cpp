
#include <Arduino.h>
#include <cmath>
#include "esp_log.h"

#include "PCM1808.hpp"
#include "esp32s3_dsp.h"
#include "esp_heap_caps.h"

#define LED_BUILTIN 17
#define FFT_SIZE  256
#define FFT_SAMPLES 512
#define SAMPLE_RATE 22050

ESP32S3_FFT fft;
static const char* TAG = "Main";

PCM1808 pcm{SAMPLE_RATE};

/**
 * Computes the RMS value of the signal
 */
float rms(const float* buffer, size_t bufSize)
{
    float ret = 0.0f;
    for(size_t i=0;i<bufSize;++i){
        ret += buffer[i] * buffer[i];
    }
    return sqrt(ret/static_cast<float>(bufSize));
}

float energy(float rms, size_t bufSize)
{
    return (rms*rms)*(2*bufSize+1);
}

void getMinMax(float& min, float& max, const float* buffer, size_t bufSize)
{
    min = buffer[0];
    max = buffer[0];
    for(size_t i=1;i<bufSize;++i){
        if(buffer[i] < min){
            min = buffer[i];
        }
        if(buffer[i] > max){
            max = buffer[i];
        }
    }
}

float getMean(const float* buffer, size_t count)
{
    float ret = 0.0f;
    for(size_t i=0;i<count;++i){
        ret += buffer[i];
    }
    return ret/static_cast<float>(count);
}

float absMean(const float* buffer, size_t count)
{
    float ret = 0.0f;
    for(size_t i=0;i<count;++i){
        ret += std::abs(buffer[i]);
    }
    return ret/static_cast<float>(count);
}

void removeDc(float* buffer, size_t count){
    float mean = getMean(buffer, count);
    for(size_t i=0;i<count;++i){
        buffer[i] = buffer[i] - mean;
    }
}

void computePower(float* buffer, size_t count)
{
    for(size_t i=0;i<count;++i){
        buffer[i] = std::pow(buffer[i], 2)/(2.0*(SAMPLE_RATE/FFT_SIZE));
    }
}

void setup()
{
    //Starts serial
    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);

    // get info from fft to set up buffers
    fft_table_t *fft_table = fft.init(FFT_SIZE, FFT_SAMPLES, SPECTRAL_AVERAGE);

    pcm.start();
}

unsigned long bCount = 0;
bool wasBeat = false;

#define RMS_HISTORY 10
float prevRMS[RMS_HISTORY] = {1.0f};

#define DIFF_HISTORY 1
float prevDiff[DIFF_HISTORY] = {1.0f};
unsigned long lastBeat = 0;

void loop()
{
    if(pcm.read()){
        size_t size = 0;
        float* buffer = pcm.getBuffer(size);
        removeDc(buffer, size);

        float output_bufr[FFT_SAMPLES];
        unsigned long start = millis();

        fft.compute(buffer, output_bufr);
        //Transform to spectrum power

        float freqPerBin = SAMPLE_RATE/FFT_SIZE;
        int startFreqBin = 50/freqPerBin;
        int endFreqBin = 600/freqPerBin;
        size_t count = (endFreqBin-startFreqBin)+1;
        computePower(&output_bufr[startFreqBin], count);
        float rmsVal = getMean(&output_bufr[startFreqBin], count);
        float sigRMS = rms(buffer, size);
        // float rmsVal = energy(rms(buffer, size), size);
        float prevRMSMean = getMean(prevRMS, RMS_HISTORY);
        float diff = (rmsVal/prevRMSMean);
        if(std::isnan(diff) || (diff == 0.0f)){
            diff = 1.0f;
        }
        float prevDiffMean = getMean(prevDiff, DIFF_HISTORY);
        if(std::isnan(prevDiffMean) || (prevDiffMean == 0.0f)){
            prevDiffMean = 1.0f;
        }
        char beat = '-';

        if(diff > 2.0f && (sigRMS > 3000.0f)){
            unsigned long now = millis();
            // if(wasBeat){
                if((now - lastBeat)>150){
                    digitalWrite(LED_BUILTIN, 1);
                    lastBeat = now;
                    beat = '+';
                }
            // }
            wasBeat = true;
        }else{
            digitalWrite(LED_BUILTIN, 0);
        }
        Serial.printf("Ampl[%c] : %f \t\t(%f / %f) from %d to %d\n", beat, sigRMS, diff, prevDiffMean,startFreqBin,endFreqBin);


        for(int i=1;i<RMS_HISTORY;++i){
            prevRMS[i-1] = prevRMS[i];
        }
        prevRMS[RMS_HISTORY -1] = rmsVal;

        for(int i=1;i<DIFF_HISTORY;++i){
            prevDiff[i-1] = prevDiff[i];
        }
        prevDiff[DIFF_HISTORY -1] = diff;
    }
}
