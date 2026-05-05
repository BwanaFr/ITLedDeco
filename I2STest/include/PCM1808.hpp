#pragma once

#include <cstdint>
#include <driver/gpio.h>
#include <driver/i2s_std.h>
#include <FreeRTOS.h>

#define SAMPLES_MAX 1024*1024

class PCM1808 {
public:
    typedef int16_t audio_sample_t;
    PCM1808();
    ~PCM1808();
    void start();
    void stop();
    void read();
    const audio_sample_t* getBuffer(size_t& size);
    void reset();
private:
    void initI2S();
    void destroyI2S();
    i2s_chan_handle_t rx_handle_;
    uint32_t sample_rate_;
    uint64_t totalSampleRead_;
    SemaphoreHandle_t mutex_;
    audio_sample_t* buffer;//[SAMPLES_MAX];
};
