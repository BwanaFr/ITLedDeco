#include "PCM1808.hpp"
#include <Arduino.h>
#include "esp_log.h"

#define AUDIO_BUFFER_SIZE 240

static const char* TAG = "PCM1808";

PCM1808::PCM1808() : rx_handle_{nullptr}, sample_rate_{44100}, totalSampleRead_{0}
{
    mutex_ = xSemaphoreCreateMutex();
    if(mutex_ == NULL){
        ESP_LOGE(TAG, "Unable to create mutex");
    }
    buffer = (audio_sample_t*)malloc(SAMPLES_MAX * sizeof(audio_sample_t));
    if(!buffer){
        ESP_LOGE(TAG, "Unable to allocate buffer!");
    }
}

PCM1808::~PCM1808()
{
    stop();
    if(buffer){
        free(buffer);
    }
}

void PCM1808::start()
{
    if(rx_handle_){
        ESP_LOGW(TAG, "I2S channel is already initialized");
        return;
    }
    initI2S();
    reset();
}

void PCM1808::stop()
{
    destroyI2S();
}


void PCM1808::read()
{
    if (!rx_handle_) {
        ESP_LOGE(TAG, "I2S channel is not initialized");
        return;
    }
    audio_sample_t buf[AUDIO_BUFFER_SIZE];
    size_t bytes_read = 0;
    size_t count = 0;
    esp_err_t result =
        i2s_channel_read(rx_handle_, buf, sizeof(buf), &bytes_read, 0);
    if (result == ESP_OK) {
        if (bytes_read > 0) {
            // ESP_LOGI(TAG, "Bytes reads : %u", bytes_read);
            // cout << "Bytes read: " << bytes_read << endl;
            count = bytes_read / sizeof(audio_sample_t);
        }
    }

    if (count <= 0) {
        // ESP_LOGW(TAG, "No sample read!");
        return;  // Invalid sample
    }

    if(totalSampleRead_ >= SAMPLES_MAX){
        // reset();
        return;
    }

    if(xSemaphoreTake(mutex_, portMAX_DELAY)){
        if(buffer){
            for(size_t i=0;i<count;i+=2){
                //Convert stereo to mono
                int64_t sample = (buf[i] + buf[i+1])/2;
                buffer[totalSampleRead_++] = sample;
                if(totalSampleRead_ >= SAMPLES_MAX){
                    break;
                }
            }
        }else{
            ESP_LOGE(TAG, "BUFFER is NULL!!");
        }
        xSemaphoreGive(mutex_);
    }
}

const PCM1808::audio_sample_t* PCM1808::getBuffer(size_t& size)
{
    const audio_sample_t * ret = buffer;
    if(!ret){
        ESP_LOGE(TAG, "Unable to allocate buffer!");
        size = 0;
        return ret;
    }
    if(xSemaphoreTake(mutex_, portMAX_DELAY)){
        size = totalSampleRead_;
        xSemaphoreGive(mutex_);
    }
    return ret;
}

void PCM1808::reset()
{
    if(xSemaphoreTake(mutex_, portMAX_DELAY)){
        totalSampleRead_ = 0;
        xSemaphoreGive(mutex_);
    }
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
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_24BIT, //The PCM1808 needs a 32 bit slow width (chapt 7.3.5.1.2 of the data sheet "The PCM1808 device accepts 64-BCK/frame or 48-BCK/frame format (only for a 384-fS system clock)" !
            .slot_mode = slot_mode,
            .slot_mask = slot_mask,
            .ws_width = 24,
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
    // chan_cfg.dma_frame_num = AUDIO_BUFFER_SIZE;

    // Create RX channel
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, nullptr, &rx_handle_));


    // Initialize channel with standard mode configuration
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));

    // Enable the channel
    ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));
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
