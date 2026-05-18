#include "AudioReactive.hpp"
#include <Arduino.h>
#include <esp_log.h>
#include <cmath>

static const char* TAG = "AudioReactive";

AudioReactive::AudioReactive(uint32_t fftSize) : fftSize_{fftSize}, fftSample_{0}, currentResult_{0},
                                resultMutex_{nullptr}
{
    resultMutex_ = xSemaphoreCreateMutex();
    if(!resultMutex_){
        ESP_LOGE(TAG, "resultMutex_ creation failed!");
    }
}

AudioReactive::~AudioReactive()
{
    fft_.end();
}

const AudioReactiveData* AudioReactive::process(AudioInput* input)
{
    size_t count = 0;
    AudioInput::audio_sample_t* samples = input->readSamples(count);
    if(!samples || (count == 0)){
        //No samples
        return nullptr;
    }
    if(count != fftSample_){
        ESP_LOGI(TAG, "Samples count %u", count);
        //Reconfigure FFT
        fftSample_ = count;
        fft_.end();
        fft_.init(fftSize_, fftSample_, SPECTRAL_AVERAGE);
    }
    //Removes DC from buffer
    removeDC(samples, count);

    //Next result
    int nextResult = ((currentResult_ == 0) ? 1 : 0);
    AudioReactiveData* result = &results_[nextResult];
    if(result->fftData == nullptr){
        result->fftData = new float[fftSize_];
        result->fftDataSize = fftSize_;
    }
    //TODO: Not needed to do it every time...
    result->fftHzPerBin = input->getSampleRate() / fftSize_;

    //Computes signal RMS
    result->signalRMS = getRMS(samples, count);

    //Performs FFT
    fft_.compute(samples, result->fftData);

    //Performs beat detection
    result->beatDetected = beatDetector_.process(samples, count, result);

    if(xSemaphoreTake(resultMutex_, portMAX_DELAY)){
        currentResult_ = nextResult;
        xSemaphoreGive(resultMutex_);
    }
    return result;
}

void AudioReactive::removeDC(AudioInput::audio_sample_t* samples, size_t count)
{
    AudioInput::audio_sample_t mean = getMean(samples, count);
    for(size_t i=0;i<count;++i){
        samples[i] = samples[i] - mean;
    }
}

AudioInput::audio_sample_t AudioReactive::getMean(const AudioInput::audio_sample_t* buffer, size_t count)
{
    AudioInput::audio_sample_t ret = 0.0f;
    for(size_t i=0;i<count;++i){
        ret += buffer[i];
    }
    return ret/static_cast<AudioInput::audio_sample_t>(count);
}

AudioInput::audio_sample_t AudioReactive::getRMS(const AudioInput::audio_sample_t* samples, size_t count)
{
    AudioInput::audio_sample_t ret = 0.0f;
    for(size_t i=0;i<count;++i){
        ret += samples[i] * samples[i];
    }
    ret /= static_cast<AudioInput::audio_sample_t>(count);
    return std::sqrt(ret);
}

BeatDetector::BeatDetector() : lastBeat_{0}
{
}

BeatDetector::~BeatDetector()
{
}


bool BeatDetector::process(const AudioInput::audio_sample_t* samples, size_t nbSamples, const AudioReactiveData* data)
{
    float avg = lastValues_.get();
    float diff = data->signalRMS / lastValues_;
    bool ret = (diff > 1.6f);
    // ESP_LOGI(TAG, "[%u]\t%f\t\t%f", lastValues_.size(), data->signalRMS, avg);
    ESP_LOGI(TAG, "%c %f Mean : %f Diff : %f", (ret ? '+' : '-'), data->signalRMS, lastValues_.get(), diff);
    lastValues_.add(data->signalRMS);
    return ret;
}