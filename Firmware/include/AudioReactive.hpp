#pragma once

#include "AudioInput.hpp"
#include "esp32s3_dsp.h"
#include "FreeRTOS.h"
#include <array>

#include <Arduino.h>
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
class MovingAverage{
public:
    MovingAverage() : count_{0}, index_{0}
    {
    };

    ~MovingAverage() = default;

    operator T(){
        return get();
    }

    T get(){
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

    std::size_t size(){
        return count_;
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
class BeatDetector{
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
private:
    unsigned long lastBeat_;                                    //!< Last beat detection
    MovingAverage<AudioInput::audio_sample_t, 1> lastValues_;   //!< Last values

};

/**
 * Tentative of implementing a small (and smart) audioreactive
 */
class AudioReactive
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
