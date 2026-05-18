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
     * Reads audio samples from hardware
     * @param size Number of samples set in buffer
     * @return Buffer containing samples. This buffer is held by this class and must not be destroyed!
     */
    virtual audio_sample_t* readSamples(std::size_t& size) = 0;

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
private:
    uint32_t sampleRate_;
};