#pragma once

#include "AudioInput.hpp"
#include "esp32s3_dsp.h"
#include "FreeRTOS.h"
#include <array>
#include <deque>

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Configuration.hpp"

//Beat detection algo selection
// #define USE_ENVELOPE
#define USE_BASS

/**
 * This structure represents a audio-reactive data results
 */
struct AudioReactiveData
{
    float* fftData;         //!< FFT data bins
    size_t fftDataSize;     //!< Number of bins in fftData
    float fftHzPerBin;      //!< Frequency (Hz) between bins
    float signalRMS;        //!< RMS value of the signal
    float filtered;         //!< Envelope filtered data
    bool beatDetected;      //!< True if beat is detected

    AudioReactiveData() : fftData{nullptr}, fftDataSize{0}, fftHzPerBin{0}, signalRMS{0}, filtered{0}, beatDetected{false}
    {
    }

    AudioReactiveData(float* data, size_t fftSize) : fftData{data}, fftDataSize{fftSize}, fftHzPerBin{0}, signalRMS{0}, filtered{0}, beatDetected{false}
    {
    }

    ~AudioReactiveData(){
        if(fftData){
            delete[] fftData;
        }
    }
};

template <typename T, std::size_t N>
class MovingStats{
public:
    MovingStats() : count_{0}, index_{0}
    {
    };

    ~MovingStats() = default;

    T getMean() const {
        T ret = T();
        for(std::size_t i=0;i<count_;++i){
            ret += buffer_[i];
        }
        return ret/count_;
    };

    void add(T val){
        if(count_ < N){
            ++count_;
        }
        if(index_ >= N){
            index_ = 0;
        }
        buffer_[index_++] = val;
    }

    std::size_t size() const {
        return count_;
    }

    T getLatest() const {
        std::size_t index = index_ == 0 ? N-1 : index_-1;
        return buffer_[index];
    }

    T getMin() const {
        T ret = T();
        if(count_>0){
            ret = buffer_[0];
        }
        for(std::size_t i=1;i<count_;++i){
            if(buffer_[i] < ret){
                ret = buffer_[i];
            }
        }
        return ret;
    }

    T getMax() const {
        T ret = T();
        if(count_>0){
            ret = buffer_[0];
        }
        for(std::size_t i=1;i<count_;++i){
            if(buffer_[i] > ret){
                ret = buffer_[i];
            }
        }
        return ret;
    }

private:
    std::array<T, N> buffer_;
    std::size_t count_;
    std::size_t index_;
};

template <typename T>
class EnvelopeFollower
{
public:
  EnvelopeFollower() : m_env{}
  {
  }

  operator T() const
  {
    return m_env;
  }


  void setup (uint32_t sampleRate, float attackMs, float releaseMs)
  {
    m_a = pow (0.01f, 1.0f / (attackMs * sampleRate * 0.001f));
    m_r = pow (0.01f, 1.0f / (releaseMs * sampleRate * 0.001f));
  }

  T process (const T* src, size_t samples)
  {
    T e = m_env;
    for (int n = samples; n; n--)
    {
    T v = std::abs(*src++);
    if (v > e)
        e = m_a * (e - v) + v;
    else
        e = m_r * (e - v) + v;
    }
    m_env = e;
    return m_env;
  }

  T m_env;
protected:
  T m_a;
  T m_r;
};

/**
 * Class for detecting beat
 */
class BeatDetector : public Configurable{
public:
    BeatDetector();
    virtual ~BeatDetector();

    /**
     * Process data
     * @param samples Samples from ADC
     * @param nbSamples Number of samples from ADC
     * @param data AudioReactiveData pointer
     * @return true if a beat is detected
     */
    bool process(const AudioInput::audio_sample_t* samples, size_t nbSamples, const AudioReactiveData* data);

    void getConfiguration(JsonObject& obj, bool full=true, bool withSecrets=true) const override;
    CFG_RESULT setConfiguration(JsonObjectConst obj) override;

    /**
     * Gets the maximum flux over last 50 computations
     * @return the maximum flux over last 50 computations
     */
    inline AudioInput::audio_sample_t getMaxValueHistory() const { return last50Values_.getMax(); };

    /**
     * Gets the maximum difference over last 50 computations
     * @return the maximum difference over last 50 computations
     */
    inline AudioInput::audio_sample_t getMaxChangeHistory() const { return last50Diff_.getMax(); };

private:
    unsigned long lastBeat_;                                    //!< Last beat detection timestamp
    float startFreq_;                                           //!< Bass analysis start frequency
    float endFreq_;                                             //!< Bass analysis end frequency
    float threshold_;                                           //!< Flux threshold
    float sensitivity_;                                         //!< Flux sensitivity
#if defined(USE_BASS)
    MovingStats<AudioInput::audio_sample_t, 10> lastValues_;    //!< Last values
#else
    MovingStats<AudioInput::audio_sample_t, 1> lastValues_;     //!< Last values
#endif
    MovingStats<AudioInput::audio_sample_t, 50> last50Values_;  //!< Last 50 values
    MovingStats<AudioInput::audio_sample_t, 50> last50Diff_;    //!< Last 50 diffs
    std::deque<float> prevFFT_;                                 //!< Previous FFT bins
    /**
     * Computes spectral flux
     */
    float getSpectralFlux(float startFreq, float endFreq, const AudioReactiveData* data);
};

/**
 * Tentative of implementing a small (and smart) audioreactive
 */
class AudioReactive : public Configurable
{
public:
    AudioReactive(uint32_t fftSize=512);
    virtual ~AudioReactive();

    /**
     * Process audio input
     * @param input Audio input to read samples from
     * @return Computing data or nullptr if no samples are available
     */
    const AudioReactiveData* process(AudioInput* input);

    /**
     * Gets audio data
     */
    const AudioReactiveData* getData();

    /**
     * Gets audio reactive statistics
     */
    void getStats(JsonObject& obj);

    void getConfiguration(JsonObject& obj, bool full=true, bool withSecrets=true) const override;
    CFG_RESULT setConfiguration(JsonObjectConst obj) override;

private:
    ESP32S3_FFT fft_;                                               //!< Object to compute FFT
    uint32_t fftSize_;                                              //!< Number of bins in FFT results
    uint32_t fftSample_;                                            //!< Number of audio samples reads
    int currentResult_;                                             //!< Current result index number
    bool audioAvailable_;                                           //!< Audio data available
    SemaphoreHandle_t resultMutex_;                                 //!< Mutex to protect result index (should be atomic, not really needed)
    AudioReactiveData results_[2];                                  //!< Result double-buffer
    BeatDetector beatDetector_;                                     //!< Beat detector
    EnvelopeFollower<AudioInput::audio_sample_t> envelopeFollower_; //!< Envelope follower filter
    std::deque<float> prevFFT_;                                     //!< Previous FFT bins

    /**
     * Removes DC offset from samples
     */
    void removeDC(AudioInput::audio_sample_t* samples, size_t count);

    /**
     * Computes RMS value of the signal
     */
    static AudioInput::audio_sample_t getRMS(const AudioInput::audio_sample_t* samples, size_t count);

    /**
     * Gets mean of the signal
     */
    static AudioInput::audio_sample_t getMean(const AudioInput::audio_sample_t* buffer, size_t count);

};
