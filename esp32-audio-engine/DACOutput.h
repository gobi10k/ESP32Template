// DACOutput.h
#ifndef DAC_OUTPUT_H
#define DAC_OUTPUT_H

#include "AudioEngine.h"
#include "driver/dac.h"
#include "driver/i2s.h"

#define I2S_NUM I2S_NUM_0

class DACOutput : public AudioOutput {
public:
  DACOutput() : dmaBuffer(nullptr) {
    i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
      .sample_rate = 44100,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_MSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = BLOCK_SIZE
    };
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN); // Enable both channels
  }

  ~DACOutput() {
    stop();
  }

  void start() override {
    i2s_start(I2S_NUM);
  }

  void stop() override {
    i2s_stop(I2S_NUM);
  }

  void writeBlock(float* buffer) override {
    // Use IRAM_ATTR for critical audio path
    // Apply dithering for 8-bit DAC
    int16_t pcmBuffer[BLOCK_SIZE];
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        float dithered = buffer[i] + (rand()/(float)RAND_MAX - 0.5f)/128.0f;
        pcmBuffer[i] = static_cast<int16_t>(dithered * 32767.0f);
    }

    // Consider using DMA double buffering
    i2s_write(I2S_NUM, pcmBuffer, sizeof(pcmBuffer), nullptr, portMAX_DELAY);
  }

private:
  uint8_t* dmaBuffer;
};

#endif