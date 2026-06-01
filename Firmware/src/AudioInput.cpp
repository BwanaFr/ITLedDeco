#include "AudioInput.hpp"

AudioInput::AudioInput(uint32_t sampleRate) : sampleRate_{sampleRate}, gain_{1.0f}
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
    audio_sample_t* ret = readSamples(size);
    for(std::size_t i=0; i<size; ++i){
        ret[i] *= gain_;
    }
    return ret;
}