#include "I2SAudioInput.hpp"
#include <Arduino.h>
#include "esp_log.h"

static const char* TAG = "I2SAudioInput";

#ifndef USE_PSRAM
#define USE_PSRAM true
#endif

I2SAudioInput::I2SAudioInput(uint32_t sampleRate, size_t nbSamples, gpio_num_t bclkPin, gpio_num_t wsPin, gpio_num_t dinPin,
                    gpio_num_t mClkPin, i2s_mclk_multiple_t mClkMultiple,
                    i2s_slot_bit_width_t slotBitWidth, i2s_slot_mode_t slotMode,
                i2s_std_slot_mask_t slotMask) :
                AudioInput{sampleRate}, bclkPin_{bclkPin}, wsPin_{wsPin}, dinPin_{dinPin},
                mClkPin_{mClkPin}, mClkMultiple_{mClkMultiple}, slotBitWidth_{slotBitWidth}, slotMode_{slotMode}, slotMask_{slotMask},
                rx_handle_{nullptr}, rxBuffer_{nullptr}, rxBufferSize_{0}, buffer_{nullptr}
{
    ESP_LOGI(TAG, "Initializing I2S audio input...");
    //Allocate buffers
    rxBufferSize_ = ((slotMask == I2S_STD_SLOT_BOTH) ? nbSamples * 2 : nbSamples) * sizeof(int16_t);
    if(USE_PSRAM){
        ESP_LOGI(TAG, "Allocating buffers in PSRAM (%u bytes, %u samples)", rxBufferSize_, nbSamples);
        rxBuffer_ = static_cast<int16_t*>(heap_caps_malloc(rxBufferSize_, MALLOC_CAP_SPIRAM));
        buffer_ = static_cast<audio_sample_t*>(heap_caps_malloc(nbSamples * sizeof(audio_sample_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT));
    }else{
        rxBuffer_ = static_cast<int16_t*>(malloc(rxBufferSize_ * sizeof(int16_t)));
        buffer_ = static_cast<audio_sample_t*>(malloc(nbSamples * sizeof(audio_sample_t)));
    }
    if(!rxBuffer_){
        ESP_LOGE(TAG, "RX buffer allocation failed!");
    }
    if(!buffer_){
        ESP_LOGE(TAG, "Audio buffer allocation failed!");
    }
}

I2SAudioInput::~I2SAudioInput()
{
    stop();
    if(rxBuffer_){
        free(rxBuffer_);
    }
    if(buffer_){
        free(buffer_);
    }
}

void I2SAudioInput::start()
{
    if(rx_handle_){
        ESP_LOGW(TAG, "I2SAudioInput already started!");
        return;
    }

    ESP_LOGI(TAG, "Starting audio capture...");
    const i2s_data_bit_width_t bit_width = I2S_DATA_BIT_WIDTH_16BIT;
    const i2s_port_t i2s_port = I2S_NUM_0;

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = getSampleRate(),
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = mClkMultiple_,
            .bclk_div = 8,  //Not used in slave mode
        },
        .slot_cfg = {
            .data_bit_width = bit_width,
            .slot_bit_width = slotBitWidth_, //The PCM1808 needs a 32 bit slow width (chapt 7.3.5.1.2 of the data sheet "The PCM1808 device accepts 64-BCK/frame or 48-BCK/frame format (only for a 384-fS system clock)" !
            .slot_mode = slotMode_,
            .slot_mask = slotMask_,
            .ws_width = static_cast<uint32_t>(slotBitWidth_),
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .mclk = mClkPin_,
            .bclk = bclkPin_,
            .ws = wsPin_,
            .dout = GPIO_NUM_NC,
            .din = dinPin_,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false}
        }
    };
    // Create I2S channel configuration with DMA buffer settings
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(
        i2s_port, I2S_ROLE_MASTER);
    chan_cfg.dma_frame_num = rxBufferSize_;

    // Create RX channel
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, nullptr, &rx_handle_));

    // Initialize channel with standard mode configuration
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));

    // Enable the channel
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));
}

void I2SAudioInput::stop()
{
    if(rx_handle_){
        // Disable the channel first
        i2s_channel_disable(rx_handle_);
        // Then delete the channel
        i2s_del_channel(rx_handle_);
        rx_handle_ = nullptr;
    }
}

AudioInput::audio_sample_t* I2SAudioInput::readSamples(std::size_t& size)
{
    if (!rx_handle_) {
        ESP_LOGE(TAG, "I2S channel is not initialized");
        size = 0;
        return nullptr;
    }
    if(!buffer_ | !rxBuffer_){
        ESP_LOGE(TAG, "Buffers not allocated!");
        size = 0;
        return nullptr;
    }
    size_t bytes_read = 0;
    size_t count = 0;
    esp_err_t result = i2s_channel_read(rx_handle_, rxBuffer_, rxBufferSize_, &bytes_read, 20);
    if (result == ESP_OK) {
        if (bytes_read > 0) {
            count = bytes_read / sizeof(int16_t);
        }
    }

    if (count <= 0) {
        size = 0;
        return nullptr;  // Invalid sample
    }

    size_t nbSamples = 0;
    if(slotMask_ == I2S_STD_SLOT_BOTH){
        //Convert stereo to mono
        for(size_t i=0;i<count;i+=2){
            buffer_[nbSamples++] = (static_cast<float>(rxBuffer_[i]) + static_cast<float>(rxBuffer_[i+1]))/2.0f;
        }
    }else{
        for(size_t i=0;i<count;++i){
            buffer_[nbSamples++] = static_cast<float>(rxBuffer_[i]);
        }
    }
    size = nbSamples;
    return buffer_;
}


