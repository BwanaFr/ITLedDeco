#include "PCM1808.hpp"
#include "fl/stl/assert.h"

#define AUDIO_BUFFER_SIZE 1024

namespace fl{

PCM1808::PCM1808(uint32_t sample_rate) : rx_handle_{nullptr}, hasError_{false}, totalSampleRead_{0}, sample_rate_{sample_rate}
{
}

PCM1808::~PCM1808()
{
    stop();
}

void PCM1808::start() FL_NOEXCEPT
{
    if(rx_handle_){
        FL_WARN("I2S channel is already initialized");
        return;
    }
    initI2S();
    totalSampleRead_ = 0;
}

void PCM1808::stop() FL_NOEXCEPT
{
    destroyI2S();
    totalSampleRead_ = 0;
}

bool PCM1808::error(fl::string *msg) FL_NOEXCEPT
{
    //TODO?
    return hasError_;
}

audio::Sample PCM1808::read() FL_NOEXCEPT
{
    if (!rx_handle_) {
        FL_WARN("I2S channel is not initialized");
        return audio::Sample();  // Invalid sample
    }
    size_t buf_size = AUDIO_BUFFER_SIZE;
    aud_sample_t buf[buf_size];
    size_t bytes_read = 0;
    size_t count = 0;
    esp_err_t result =
        i2s_channel_read(rx_handle_, buf, buf_size * sizeof(aud_sample_t), &bytes_read, 10);
    if (result == ESP_OK) {
        if (bytes_read > 0) {
            // cout << "Bytes read: " << bytes_read << endl;
            count = bytes_read / sizeof(aud_sample_t);
        }
    }

    if (count <= 0) {
        return audio::Sample();  // Invalid sample
    }
    u32 timestamp_ms = static_cast<u32>((totalSampleRead_ * 1000ULL) / sample_rate_);

    size_t monoCount = count / 2;
    //Convert to mono
    int index = 0;
    for(size_t i=0;i<count;i+=2){
        int64_t s = (buf[i] + buf[i+1])/2;
        buf[index++] = static_cast<aud_sample_t>(s);
    }

    // Update total samples counter
    totalSampleRead_ += monoCount;

    fl::span<const i16> data(buf, monoCount);

    // Create audio::Sample with pooled audio::SampleImpl (pooling handled internally)
    return audio::Sample(data, timestamp_ms);
}

void PCM1808::initI2S()
{
    const i2s_data_bit_width_t bit_width = I2S_DATA_BIT_WIDTH_16BIT;
    const i2s_slot_mode_t slot_mode = I2S_SLOT_MODE_STEREO;
    const i2s_std_slot_mask_t slot_mask = I2S_STD_SLOT_BOTH;
    const i2s_port_t i2s_port = I2S_NUM_0;

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = sample_rate_,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = I2S_MCLK_MULTIPLE_384,
            .bclk_div = 8,  //Not used in slave mode
        },
        .slot_cfg = {
            .data_bit_width = bit_width,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode = slot_mode,
            .slot_mask = slot_mask,
            .ws_width = 32,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .mclk = GPIO_NUM_1,
            .bclk = GPIO_NUM_5,
            .ws = GPIO_NUM_4,
            .dout = GPIO_NUM_NC,
            .din = GPIO_NUM_8,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false}
        }
    };
    // Create I2S channel configuration with DMA buffer settings
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        i2s_port, I2S_ROLE_MASTER);
    chan_cfg.dma_frame_num = AUDIO_BUFFER_SIZE; //dma_frame_num * slot_num * data_bit_width / 8 = dma_buffer_size <= 4092
    // Create RX channel
    esp_err_t ret = i2s_new_channel(&chan_cfg, nullptr, &rx_handle_);
    FL_ASSERT(ret == ESP_OK, "Failed to create I2S channel");

    // Initialize channel with standard mode configuration
    ret = i2s_channel_init_std_mode(rx_handle_, &std_cfg);
    FL_ASSERT(ret == ESP_OK, "Failed to initialize I2S channel");

    // Enable the channel
    ret = i2s_channel_enable(rx_handle_);
    FL_ASSERT(ret == ESP_OK, "Failed to enable I2S channel");

}

void PCM1808::destroyI2S()
{
    if(rx_handle_){
        // Disable the channel first
        i2s_channel_disable(rx_handle_);
        // Then delete the channel
        i2s_del_channel(rx_handle_);
        rx_handle_ = nullptr;
    }
}

fl::shared_ptr<fl::audio::IInput> PCM1808::createPCM1808(uint32_t sample_rate)
{
    return fl::make_shared<fl::PCM1808>(sample_rate);
}

}//namespace fl