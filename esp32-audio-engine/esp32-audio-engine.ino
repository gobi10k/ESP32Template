#include "AudioEngine.h"
#include "DACOutput.h"
#include "WavetableSynth.h"
#include "Limiter.h"
#include "ADSR.h"

// Add this LED_BUILTIN definition for ESP32 boards
#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // GPIO2 is commonly used for onboard LED
#endif

DACOutput dacOutput;
AudioEngine audioEngine(dacOutput);
WavetableSynth wtSynth;
Limiter limiter(0.95f, 0.005f);
ADSR ampEnv;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.begin(115200);
  Serial.println("ESP32 Wavetable Synth Engine");
  Serial.println("Commands: start, stop, freq <f>, pos <p>, morph <m>, noteon, noteoff, peak, rms, stats");

  // Setup ADSR
  ampEnv.setAttack(0.01f);
  ampEnv.setDecay(0.2f);
  ampEnv.setSustain(0.5f);
  ampEnv.setRelease(1.0f);
  ampEnv.setSampleRate(44100.0f);

  // Setup Wavetable Synth
  wtSynth.generateSineWavetable();      // Position 0
  wtSynth.generateTriangleWavetable();  // Position 1
  wtSynth.generateSquareWavetable();    // Position 2
  wtSynth.generateSawWavetable();       // Position 3
  wtSynth.generateComplexWaveform(0b0000010101); // Position 4 (organ-like)
  wtSynth.generateComplexWaveform(0b1111111111); // Position 5 (rich harmonics)

  // Setup modulation
  ModulationEngine& modEngine = audioEngine.getModulationEngine();
  modEngine.addSource(&ampEnv);
  modEngine.addRoute(&ampEnv, wtSynth.getGainPtr(), 1.0f);

  // Add sources and effects to audio engine
  audioEngine.addSource(&wtSynth);
  audioEngine.addEffect(&limiter);

  ampEnv.noteOff(); // Ensure envelope is off
  wtSynth.setFrequency(440.0f);

  audioEngine.start();
}

void processCommand(const String& command) {
  if (command == "start") {
    audioEngine.start();
    Serial.println("Engine started");
  } else if (command == "stop") {
    audioEngine.stop();
    Serial.println("Engine stopped");
  } else if (command.startsWith("freq")) {
    String valueStr = command.substring(5);
    float freq = valueStr.toFloat();
    if (freq >= 20.0f && freq <= 20000.0f) {
      wtSynth.setFrequency(freq);
      Serial.print("Frequency set to: ");
      Serial.println(freq);
    } else {
      Serial.println("Invalid frequency (20-20000Hz)");
    }
  } else if (command.startsWith("pos")) {
    String valueStr = command.substring(4);
    float pos = valueStr.toFloat();
    if (pos >= 0.0f && pos <= 5.0f) { // 6 wavetables
      wtSynth.setPosition(pos);
      Serial.print("Position set to: ");
      Serial.println(pos);
    } else {
      Serial.println("Invalid position (0.0-5.0)");
    }
  } else if (command.startsWith("morph")) {
    String valueStr = command.substring(6);
    float morph = valueStr.toFloat();
    if (morph >= 0.0f && morph <= 1.0f) {
      wtSynth.setMorph(morph);
      Serial.print("Morph set to: ");
      Serial.println(morph);
    } else {
      Serial.println("Invalid morph (0.0-1.0)");
    }
  }
  else if (command == "noteon") {
    ampEnv.noteOn();
    Serial.println("Note On");
  } else if (command == "noteoff") {
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