#include "AudioEngine.h"
#include "DACOutput.h"
#include "SineWave.h"
#include "Limiter.h"
#include "ADSR.h"

// Add this LED_BUILTIN definition for ESP32 boards
#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // GPIO2 is commonly used for onboard LED
#endif

DACOutput dacOutput;
AudioEngine audioEngine(dacOutput);
SineWave sineWave(440.0f, 1.0f); // Start with 0 amplitude
Limiter limiter(0.95f, 0.005f);
ADSR ampEnv;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.begin(115200);
  Serial.println("ESP32 Professional Audio Engine");
  Serial.println("Commands: start, stop, freq <f>[i], amp <a>[i], noteon, noteoff, peak, rms, stats");

  // Setup ADSR - this is correct
  ampEnv.setAttack(0.01f);
  ampEnv.setDecay(0.2f);
  ampEnv.setSustain(0.5f);
  ampEnv.setRelease(1.0f);
  ampEnv.setSampleRate(44100.0f);
  // ampEnv starts in IDLE state by default, which is correct

  // Setup sine wave
  sineWave.setAmplitude(1.0f, true); // Set base amplitude

  // Setup modulation - this routes ADSR envelope to control amplitude
  ModulationEngine& modEngine = audioEngine.getModulationEngine();
  modEngine.addSource(&ampEnv);
  modEngine.addRoute(&ampEnv, sineWave.getAmplitudePtr(), 1.0f);
  
  // Add to audio engine
  audioEngine.addSource(&sineWave);
  audioEngine.addEffect(&limiter);
  audioEngine.start();
}


void processCommand(const String& command) {
  if (command == "start") {
    audioEngine.start();
    Serial.println("Engine started");
  }
  else if (command == "stop") {
    audioEngine.stop();
    Serial.println("Engine stopped");
  }
  else if (command.startsWith("freq")) {
    bool immediate = command.endsWith("i");
    String valueStr = command.substring(5, immediate ? command.length()-1 : command.length());
    float freq = valueStr.toFloat();
    
    if (freq >= 20.0f && freq <= 20000.0f) {
      sineWave.setFrequency(freq, immediate);
      Serial.print("Frequency set to: ");
      Serial.print(freq);
      Serial.println(immediate ? " (immediate)" : " (smoothed)");
    } else {
      Serial.println("Invalid frequency (20-20000Hz)");
    }
  }
  else if (command.startsWith("amp") || command.startsWith("a ")) {
        String valueStr = command.substring(command.indexOf(' ') + 1);
        float amp = valueStr.toFloat();
        if (amp >= 0.0f && amp <= 1.0f) {
            sineWave.setAmplitude(amp); // This now controls base amplitude
            Serial.print("Base amplitude set to: ");
            Serial.println(amp);
        } else {
            Serial.println("Invalid amplitude (0.0-1.0)");
        }
    }

  else if (command == "noteon") {
    ampEnv.noteOn();
    Serial.println("Note On");
  }
  else if (command == "noteoff") {
    ampEnv.noteOff();
    Serial.println("Note Off");
  }
  else if (command == "peak") {
    Serial.print("Peak level: ");
    Serial.println(audioEngine.getPeakLevel(), 4);
  }
  else if (command == "rms") {
    Serial.print("RMS level: ");
    Serial.println(audioEngine.getRMSLevel(), 4);
  }
  else if (command == "stats") {
    Serial.print("Peak: ");
    Serial.print(audioEngine.getPeakLevel(), 4);
    Serial.print(" | RMS: ");
    Serial.println(audioEngine.getRMSLevel(), 4);
  }
}

void loop() {
  static String commandBuffer = "";

  // Process serial commands
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      commandBuffer.trim();
      if (commandBuffer.length() > 0) {
        processCommand(commandBuffer);
      }
      commandBuffer = "";
    } 
    else if (c != '\r') {
      commandBuffer += c;
    }
  }

  // Render audio
  if (audioEngine.getPeakLevel() > 0.98f) {
    // Visual clip indicator
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 100) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      lastBlink = millis();
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  audioEngine.renderBlock();
}