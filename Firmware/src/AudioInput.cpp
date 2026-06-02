#include "AudioInput.hpp"
#include <cmath>

AudioInput::AudioInput(uint32_t sampleRate) : sampleRate_{sampleRate}, gain_{1.0f}, meanValue_{0.0f}
{
}

void AudioInput::start()
{
}

void AudioInput::stop()
{
}

AudioInput::audio_sample_t* AudioInput::read(std::size_t& size)
{
    audio_sample_t mean = 0.0f;
    audio_sample_t* ret = readSamples(size);
    for(std::size_t i=0; i<size; ++i){
        ret[i] *= gain_;
        mean += std::abs(ret[i]);
    }
    if(size > 0){
        meanValue_ = mean/static_cast<audio_sample_t>(size);
    }
    return ret;
}