#pragma once

#include <cstdint>

/**
 * A mono audio input
 */
class AudioInput {
public:
    /**
     * Constructor
     * @param sampleRate Sample rate of the audio stream
     */
    AudioInput(uint32_t sampleRate);
    virtual ~AudioInput() = default;
    typedef float audio_sample_t;

    /**
     * Reads samples from HW and apply the gain
     */
    virtual audio_sample_t* read(std::size_t& size);

    /**
     * Gets audio sample rate
     * @return the sample rate
     */
    inline uint32_t getSampleRate() { return sampleRate_; }

    /**
     * Starts the input
     */
    virtual void start();

    /**
     * Stops the input
     */
    virtual void stop();

    /**
     * Sets signal gain
     * @param gain
     */
    inline void setGain(float gain) { gain_ = gain; }

    /**
     * Gets signal gain
     * @return signal gain
     */
    inline float getGain() const { return gain_; }
protected:
    /**
     * Reads audio samples from hardware
     * @param size Number of samples set in buffer
     * @return Buffer containing samples. This buffer is held by this class and must not be destroyed!
     */
    virtual audio_sample_t* readSamples(std::size_t& size) = 0;
private:
    uint32_t sampleRate_;
    float gain_;
};