#pragma once

#include "AudioInput.hpp"
#include <driver/gpio.h>
#include <driver/i2s_std.h>

/**
 * ESP32 I2S 16 bits audio input (try to be generic)
 */
class I2SAudioInput : public AudioInput{
public:
    /**
     * Constructor
     * @param sampleRate  Sample rate in Hz
     * @param nbSamples Numberf of samples in the buffer
     * @param bclkPin BCLK pin
     * @param wsPin Word select pin
     * @param dinPin Data in pin
     * @param mClkPin Master clock pin (can be set to GPIO_NUM_NC if not used, default)
     * @param mClkMultiple Master clock multiple (384 by default)
     * @param slotBitWidth Slot bit width (32 bits by default)
     * @param slotMode Slot mode, mono : only receive the data in the first slots stereo : receive the data in all slots for rx mode
     * @param slotMask receives left, right or both slot
     */
    I2SAudioInput(uint32_t sampleRate, size_t nbSamples, gpio_num_t bclkPin, gpio_num_t wsPin, gpio_num_t dinPin,
                    gpio_num_t mClkPin = GPIO_NUM_NC, i2s_mclk_multiple_t mClkMultiple = I2S_MCLK_MULTIPLE_384,
                    i2s_slot_bit_width_t slotBitWidth = I2S_SLOT_BIT_WIDTH_32BIT, i2s_slot_mode_t slotMode = I2S_SLOT_MODE_STEREO,
                i2s_std_slot_mask_t slotMask = I2S_STD_SLOT_BOTH);
    virtual ~I2SAudioInput();
    audio_sample_t* readSamples(std::size_t& size) override;
    void start() override;
    void stop() override;
private:
    gpio_num_t mClkPin_;
    gpio_num_t bclkPin_;
    gpio_num_t wsPin_;
    gpio_num_t dinPin_;
    i2s_mclk_multiple_t mClkMultiple_;
    i2s_slot_bit_width_t slotBitWidth_;
    i2s_slot_mode_t slotMode_;
    i2s_std_slot_mask_t slotMask_;
    i2s_chan_handle_t rx_handle_;
    int16_t* rxBuffer_;
    size_t rxBufferSize_;
    audio_sample_t* buffer_;
};