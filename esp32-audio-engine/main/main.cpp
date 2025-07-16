#include <stdio.h>
#include <string>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "AudioEngine.h"
#include "DACOutput.h"
#include "SineWave.h"
#include "SawWave.h"
#include "SquareWave.h"
#include "Limiter.h"
#include "ADSR.h"

#define LED_BUILTIN GPIO_NUM_2
#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

static const char* TAG = "AudioEngine";

DACOutput dacOutput;
AudioEngine audioEngine(dacOutput);
SineWave sineWave;
SawWave sawWave;
SquareWave squareWave;
Limiter limiter(0.95f, 0.005f);
ADSR ampEnv;

AudioSource* currentWave = &sineWave;

void processCommand(const std::string& command);

void audio_task(void* arg) {
    uint64_t lastBlink = 0;
    bool led_state = false;
    while (1) {
        if (audioEngine.getPeakLevel() > 0.98f) {
            uint64_t now = esp_timer_get_time() / 1000;
            if (now - lastBlink > 100) {
                led_state = !led_state;
                gpio_set_level(LED_BUILTIN, led_state);
                lastBlink = now;
            }
        } else {
            if (led_state) {
                led_state = false;
                gpio_set_level(LED_BUILTIN, led_state);
            }
        }
        audioEngine.renderBlock();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void uart_task(void *pvParameters)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &uart_config);

    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    std::string commandBuffer = "";

    while (1) {
        int len = uart_read_bytes(UART_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        if (len) {
            data[len] = '\0';
            char* token = strtok((char*)data, "\n");
            while (token != NULL) {
                std::string command(token);
                command.erase(command.find_last_not_of(" \t\n\r") + 1);
                if (command.length() > 0) {
                    processCommand(command);
                }
                token = strtok(NULL, "\n");
            }
        }
    }
}

extern "C" void app_main(void)
{
    gpio_reset_pin(LED_BUILTIN);
    gpio_set_direction(LED_BUILTIN, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "ESP32 Professional Audio Engine");
    ESP_LOGI(TAG, "Commands: start, stop, freq <f>[i], amp <a>[i], wave <sine|saw|square>, noteon, noteoff, peak, rms, stats");

    ampEnv.setAttack(0.01f);
    ampEnv.setDecay(0.2f);
    ampEnv.setSustain(0.5f);
    ampEnv.setRelease(1.0f);
    ampEnv.setSampleRate(44100.0f);

    ModulationEngine& modEngine = audioEngine.getModulationEngine();
    modEngine.addSource(&ampEnv);
    modEngine.addRoute(&ampEnv, sineWave.getAmplitudePtr(), 1.0f);
    modEngine.addRoute(&ampEnv, sawWave.getAmplitudePtr(), 1.0f);
    modEngine.addRoute(&ampEnv, squareWave.getAmplitudePtr(), 1.0f);

    audioEngine.addSource(&sineWave);
    audioEngine.addSource(&sawWave);
    audioEngine.addSource(&squareWave);
    audioEngine.addEffect(&limiter);

    ampEnv.noteOff();
    sineWave.setAmplitude(1.0f);

    audioEngine.start();

    xTaskCreate(audio_task, "audio_task", 4096, NULL, 5, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);
}

void processCommand(const std::string& command) {
    if (command == "start") {
        audioEngine.start();
        ESP_LOGI(TAG, "Engine started");
    } else if (command == "stop") {
        audioEngine.stop();
        ESP_LOGI(TAG, "Engine stopped");
    } else if (command.rfind("freq", 0) == 0) {
        bool immediate = command.back() == 'i';
        size_t start = 5;
        size_t end = immediate ? command.length() - 1 : command.length();
        try {
            float freq = std::stof(command.substr(start, end - start));
            if (freq >= 20.0f && freq <= 20000.0f) {
                sineWave.setFrequency(freq, immediate);
                sawWave.setFrequency(freq, immediate);
                squareWave.setFrequency(freq, immediate);
                ESP_LOGI(TAG, "Frequency set to: %f %s", freq, immediate ? "(immediate)" : "(smoothed)");
            } else {
                ESP_LOGE(TAG, "Invalid frequency (20-20000Hz)");
            }
        } catch (const std::invalid_argument& ia) {
            ESP_LOGE(TAG, "Invalid frequency format");
        }
    } else if (command.rfind("amp", 0) == 0 || command.rfind("a ", 0) == 0) {
        size_t start = command.find(' ') + 1;
        try {
            float amp = std::stof(command.substr(start));
            if (amp >= 0.0f && amp <= 1.0f) {
                sineWave.setAmplitude(amp);
                sawWave.setAmplitude(amp);
                squareWave.setAmplitude(amp);
                ESP_LOGI(TAG, "Base amplitude set to: %f", amp);
            } else {
                ESP_LOGE(TAG, "Invalid amplitude (0.0-1.0)");
            }
        } catch (const std::invalid_argument& ia) {
            ESP_LOGE(TAG, "Invalid amplitude format");
        }
    } else if (command.rfind("wave", 0) == 0) {
        std::string waveType = command.substr(5);
        waveType.erase(0, waveType.find_first_not_of(" \t\n\r"));
        waveType.erase(waveType.find_last_not_of(" \t\n\r") + 1);

        if (waveType == "sine") {
            currentWave = &sineWave;
            sineWave.setAmplitude(1.0f);
            sawWave.setAmplitude(0.0f);
            squareWave.setAmplitude(0.0f);
            ESP_LOGI(TAG, "Waveform set to Sine");
        } else if (waveType == "saw") {
            currentWave = &sawWave;
            sineWave.setAmplitude(0.0f);
            sawWave.setAmplitude(1.0f);
            squareWave.setAmplitude(0.0f);
            ESP_LOGI(TAG, "Waveform set to Saw");
        } else if (waveType == "square") {
            currentWave = &squareWave;
            sineWave.setAmplitude(0.0f);
            sawWave.setAmplitude(0.0f);
            squareWave.setAmplitude(1.0f);
            ESP_LOGI(TAG, "Waveform set to Square");
        } else {
            ESP_LOGE(TAG, "Unknown waveform. Use sine, saw, or square.");
        }
    } else if (command == "noteon") {
        ampEnv.noteOn();
        ESP_LOGI(TAG, "Note On");
    } else if (command == "noteoff") {
        ampEnv.noteOff();
        ESP_LOGI(TAG, "Note Off");
    } else if (command == "peak") {
        ESP_LOGI(TAG, "Peak level: %f", audioEngine.getPeakLevel());
    } else if (command == "rms") {
        ESP_LOGI(TAG, "RMS level: %f", audioEngine.getRMSLevel());
    } else if (command == "stats") {
        ESP_LOGI(TAG, "Peak: %f | RMS: %f", audioEngine.getPeakLevel(), audioEngine.getRMSLevel());
    } else {
        ESP_LOGW(TAG, "Unknown command: %s", command.c_str());
    }
}