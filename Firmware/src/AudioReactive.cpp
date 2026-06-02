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

    prevFFT_.resize(fftSize);
}

AudioReactive::~AudioReactive()
{
    fft_.end();
}

const AudioReactiveData* AudioReactive::process(AudioInput* input)
{
    size_t count = 0;
    AudioInput::audio_sample_t* samples = input->read(count);
    if(!samples || (count == 0)){
        //No samples
        return nullptr;
    }
    if(count != fftSample_){
        //Reset, compute some parameters
        results_[0].fftHzPerBin = input->getSampleRate() / fftSize_;
        results_[1].fftHzPerBin = results_[0].fftHzPerBin;

        envelopeFollower_.setup(input->getSampleRate(), 1.0f, 50.0f);
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

void AudioReactive::getConfiguration(JsonObject& obj, bool full, bool withSecrets) const
{
    beatDetector_.getConfiguration(obj, full, withSecrets);
}

Configurable::CFG_RESULT AudioReactive::setConfiguration(JsonObjectConst obj)
{
    ESP_LOGI(TAG, "Setting audio reactive configuration");
    return beatDetector_.setConfiguration(obj);
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

BeatDetector::BeatDetector() : lastBeat_{0}, startFreq_{30}, endFreq_{180}, threshold_{20.0f}, sensitivity_{1.4f}
{
}

BeatDetector::~BeatDetector()
{
}

#if defined(USE_BASS)
bool BeatDetector::process(const AudioInput::audio_sample_t* samples, size_t nbSamples, const AudioReactiveData* data)
{
    //Only use bass bins
    float value = getSpectralFlux(startFreq_, endFreq_, data);
    float prevMean = lastValues_;
    bool ret = false;
    if(prevMean > 0.0f){
        float diff = (value/prevMean);
        ret = ((value > threshold_) && (diff>=sensitivity_) && (lastValues_.getLatest() < threshold_));
        // ESP_LOGI(TAG, "%c %f Mean : %f \t\t Last: %f", (ret ? '+' : '-'), value, prevMean, lastValues_.getLatest());
    }else{
        ret = (value > (1.5f * threshold_));
    }

    lastValues_.add(value);
    unsigned long now = ::millis();
    if(ret){
        //250ms between each beat give use a 240 BPM max :)
        if((now - lastBeat_) < 100){
            ret = false;
        }else{
            lastBeat_ = now;
        }
    }
    return ret;
}
#elif defined(USE_ENVELOPE)
bool BeatDetector::process(const AudioInput::audio_sample_t* samples, size_t nbSamples, const AudioReactiveData* data)
{
    float avg = lastValues_.get();
    //Use filtered bass value
    float value = data->filtered;
    float silenceThd = 0.5f;
    float threshold = 1.22f;
    float diff = lastValues_ == 0.0f ? value*2.0f : value / lastValues_;
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
    ESP_LOGI(TAG, "%c %f Mean : %f Diff : %f", (ret ? '+' : '-'), value, lastValues_.get(), diff);
    return ret;
}
#else
bool BeatDetector::process(const AudioInput::audio_sample_t* samples, size_t nbSamples, const AudioReactiveData* data)
{
    //Use the signal RMS
    float value = data->signalRMS;
    float silenceThd = 0.5f;
    float threshold = 1.3f;
    float diff = lastValues_ == 0.0f ? value*2.0f : value / lastValues_;
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
    ESP_LOGI(TAG, "%c %f Mean : %f Diff : %f", (ret ? '+' : '-'), value, lastValues_.get(), diff);
    return ret;
}
#endif

float BeatDetector::getSpectralFlux(float startFreq, float endFreq, const AudioReactiveData* data)
{
    float ret = 0.0f;
    if(data->fftDataSize != prevFFT_.size()){
        prevFFT_.resize(data->fftDataSize);
    }

    int startFreqBin = startFreq/data->fftHzPerBin;
    int endFreqBin = (endFreq/data->fftHzPerBin)+1;

    for (std::size_t i = startFreqBin; i < endFreqBin; i++) {
        float diff = data->fftData[i] - prevFFT_[i];
        if (diff > 0.0f) {
            ret += diff;
        }
    }
    ret /= static_cast<float>(endFreqBin-startFreqBin);

    //Save previous FFT
    for(std::size_t i=0;i<data->fftDataSize; ++i){
        prevFFT_[i] = data->fftData[i];
    }
    return ret;
}

void BeatDetector::getConfiguration(JsonObject& obj, bool full, bool withSecrets) const
{
    createSetting(obj, full, "bd_startFreq", "Beat detect. start frequency [Hz]", startFreq_, 0.0f, 10000.0f);
    createSetting(obj, full, "bd_endFreq", "Beat detect. end frequency [Hz]", endFreq_, 0.0f, 10000.0f);
    createSetting(obj, full, "bd_threshold", "Beat detect. threshold", threshold_, 0.0f, 100.0f);
    createSetting(obj, full, "bd_sensitivity", "Beat detect. sensitivity", sensitivity_, 0.0f, 50.0f);
}

Configurable::CFG_RESULT BeatDetector::setConfiguration(JsonObjectConst obj)
{
    CFG_RESULT ret;
    ret |= setValueIfSet(obj, "bd_startFreq", startFreq_);
    ret |= setValueIfSet(obj, "bd_endFreq", endFreq_);
    ret |= setValueIfSet(obj, "bd_threshold", threshold_);
    ret |= setValueIfSet(obj, "bd_sensitivity", sensitivity_);
    return ret;
}