#include "AudioEngine.h"
#include "DACOutput.h"
#include "SineWave.h"
#include "SawWave.h"
#include "SquareWave.h"
#include "Limiter.h"
#include "ADSR.h"

// Add this LED_BUILTIN definition for ESP32 boards
#ifndef LED_BUILTIN
#define LED_BUILTIN 2  // GPIO2 is commonly used for onboard LED
#endif

DACOutput dacOutput;
AudioEngine audioEngine(dacOutput);
SineWave sineWave;
SawWave sawWave;
SquareWave squareWave;
Limiter limiter(0.95f, 0.005f);
ADSR ampEnv;

AudioSource* currentWave = &sineWave;

void setup() {
    // Initialize all buffers to zero
    memset(&sineWave, 0, sizeof(sineWave));
    memset(&sawWave, 0, sizeof(sawWave));
    memset(&squareWave, 0, sizeof(squareWave));

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  Serial.begin(115200);
  Serial.println("ESP32 Professional Audio Engine");
  Serial.println("Commands: start, stop, freq <f>[i], amp <a>[i], wave <sine|saw|square>, noteon, noteoff, peak, rms, stats");

  // Setup ADSR
  ampEnv.setAttack(0.01f);
  ampEnv.setDecay(0.2f);
  ampEnv.setSustain(0.5f);
  ampEnv.setRelease(1.0f);
  ampEnv.setSampleRate(44100.0f);

  // Setup waveforms - all start with 0 amplitude by default

  // Setup modulation
  ModulationEngine& modEngine = audioEngine.getModulationEngine();
  modEngine.addSource(&ampEnv);
  modEngine.addRoute(&ampEnv, sineWave.getAmplitudePtr(), 1.0f);
  modEngine.addRoute(&ampEnv, sawWave.getAmplitudePtr(), 1.0f);
  modEngine.addRoute(&ampEnv, squareWave.getAmplitudePtr(), 1.0f);
  
  // Add sources and effects to audio engine
  audioEngine.addSource(&sineWave);
  audioEngine.addSource(&sawWave);
  audioEngine.addSource(&squareWave);
  audioEngine.addEffect(&limiter);

  // Start with sine wave active, and others inactive
  ampEnv.noteOff(); // Ensure envelope is off
  sineWave.setAmplitude(1.0f);


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
    bool immediate = command.endsWith("i");
    String valueStr = command.substring(5, immediate ? command.length() - 1 : command.length());
    float freq = valueStr.toFloat();
    if (freq >= 20.0f && freq <= 20000.0f) {
      sineWave.setFrequency(freq, immediate);
      sawWave.setFrequency(freq, immediate);
      squareWave.setFrequency(freq, immediate);
      Serial.print("Frequency set to: ");
      Serial.print(freq);
      Serial.println(immediate ? " (immediate)" : " (smoothed)");
    } else {
      Serial.println("Invalid frequency (20-20000Hz)");
    }
  } else if (command.startsWith("amp") || command.startsWith("a ")) {
    String valueStr = command.substring(command.indexOf(' ') + 1);
    float amp = valueStr.toFloat();
    if (amp >= 0.0f && amp <= 1.0f) {
      sineWave.setAmplitude(amp);
      sawWave.setAmplitude(amp);
      squareWave.setAmplitude(amp);
      Serial.print("Base amplitude set to: ");
      Serial.println(amp);
    } else {
      Serial.println("Invalid amplitude (0.0-1.0)");
    }
  } else if (command.startsWith("wave")) {
    String waveType = command.substring(5);
    waveType.trim();

    if (waveType == "sine") {
      currentWave = &sineWave;
      sineWave.setAmplitude(1.0f);
      sawWave.setAmplitude(0.0f);
      squareWave.setAmplitude(0.0f);
      Serial.println("Waveform set to Sine");
    } else if (waveType == "saw") {
      currentWave = &sawWave;
      sineWave.setAmplitude(0.0f);
      sawWave.setAmplitude(1.0f);
      squareWave.setAmplitude(0.0f);
      Serial.println("Waveform set to Saw");
    } else if (waveType == "square") {
      currentWave = &squareWave;
      sineWave.setAmplitude(0.0f);
      sawWave.setAmplitude(0.0f);
      squareWave.setAmplitude(1.0f);
      Serial.println("Waveform set to Square");
    } else {
      Serial.println("Unknown waveform. Use sine, saw, or square.");
    }
  } else if (command == "noteon") {
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