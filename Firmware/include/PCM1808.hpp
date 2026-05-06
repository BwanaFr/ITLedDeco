#pragma once

#include <driver/gpio.h>
#include <driver/i2s_std.h>

#include <fl/system/log.h>
#include <fl/audio/audio_input.h>

namespace fl {

class PCM1808 : public audio::IInput {
public:
    PCM1808(uint32_t sample_rate);
    ~PCM1808();
    void start() FL_NOEXCEPT override;
    void stop() FL_NOEXCEPT override;
    bool error(fl::string *msg = nullptr) FL_NOEXCEPT override;
    audio::Sample read() FL_NOEXCEPT override;
    static fl::shared_ptr<fl::audio::IInput> createPCM1808(uint32_t sample_rate);
private:
    void initI2S();
    void destroyI2S();
    i2s_chan_handle_t rx_handle_;
    bool hasError_;
    fl::string errMsg_;
    u64 totalSampleRead_;
    uint32_t sample_rate_;
    typedef i16 aud_sample_t;
};

} //namespace fl