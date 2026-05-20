#include "AudioReactive.hpp"
#include <Arduino.h>
#include <esp_log.h>
#include <cmath>

static const char* TAG = "AudioReactive";

AudioReactive::AudioReactive(uint32_t fftSize) : fftSize_{fftSize}, fftSample_{0}, currentResult_{0},
                                audioAvailable_{false}, resultMutex_{nullptr}
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
        //Reset, compute some parameters
        results_[0].fftHzPerBin = input->getSampleRate() / fftSize_;
        results_[1].fftHzPerBin = results_[0].fftHzPerBin;

        envelopeFollower_.setup(input->getSampleRate(), 0.1f, 50.0f);
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

    //Computes signal RMS
    result->signalRMS = getRMS(samples, count);

    //Envelope filter
    result->filtered = envelopeFollower_.process(samples, count);

    //Performs FFT
    fft_.compute(samples, result->fftData);

    //Performs beat detection
    result->beatDetected = beatDetector_.process(samples, count, result);

    if(xSemaphoreTake(resultMutex_, portMAX_DELAY)){
        currentResult_ = nextResult;
        audioAvailable_ = true;
        xSemaphoreGive(resultMutex_);
    }
    return result;
}

const AudioReactiveData* AudioReactive::getData()
{
    const AudioReactiveData* ret = nullptr;
    if(audioAvailable_){
        if(xSemaphoreTake(resultMutex_, portMAX_DELAY)){
            ret = &results_[currentResult_];
            audioAvailable_ = false;
            xSemaphoreGive(resultMutex_);
        }
    }
    return ret;
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

// #define USE_ENVELOPE
// #define USE_BA SS
bool BeatDetector::process(const AudioInput::audio_sample_t* samples, size_t nbSamples, const AudioReactiveData* data)
{
    float avg = lastValues_.get();
#ifdef USE_ENVELOPE
    //Use filtered bass value
    float value = data->filtered;
    float silenceThd = 6000.0f;
    float threshold = 1.3f;
#elif defined(USE_BASS)
    //Only use bass bins
    float value = 0.0f;
    int startFreqBin = 80/data->fftHzPerBin;
    int endFreqBin = (600/data->fftHzPerBin)+1;
    float freqAmplitude = 0;
    for(int i=startFreqBin;i<endFreqBin;++i){
        freqAmplitude += data->fftData[i];
    }
    freqAmplitude /= (endFreqBin-startFreqBin);
    value = std::sqrt(freqAmplitude);
    float silenceThd = 400.0f;
    float threshold = 1.4f;
#else
    //Use the signal RMS
    float value = data->signalRMS;
    float silenceThd = 6000.0f;
    float threshold = 1.3f;
#endif
    float diff = value / lastValues_;
    bool ret = (diff > threshold) && (value > silenceThd);
    lastValues_.add(value);
    unsigned long now = ::millis();
    if(ret){
        if((now - lastBeat_) < 100){
            ret = false;
        }
        lastBeat_ = now;
    }
    int diffPc = (diff-1.0f)*100;
    // ESP_LOGI(TAG, "%c %f Mean : %f Diff : %d", (ret ? '+' : '-'), value, lastValues_.get(), diffPc);
    return ret;
}