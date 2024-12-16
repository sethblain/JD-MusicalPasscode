#include <Audio.h>
#include "DFRobotDFPlayerMini.h"
#include "mp3tf16p.h"

//IMP441 or INP441 microphone

// Define the analog pin connected to the microphone
const int micPin = A0;
const int buttonPin = 2; // Pin for the button

//For controlling DC motor
const int motorPin = 6;

//Initialize MP3Player Object
MP3Player mp3(16,17);

// Audio library components
AudioInputAnalog         adc1(micPin);  // Analog input from mic (A2)
AudioAnalyzeFFT1024      fft1024;       // FFT analyzer (1024 bins)
AudioConnection          patchCord1(adc1, fft1024);

// Note names for mapping
const char *noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); // Configure button pin
  pinMode(motorPin, OUTPUT);
  Serial.begin(9600);
  AudioMemory(10); // Allocate memory for audio processing
  fft1024.windowFunction(AudioWindowHanning1024); // Set FFT window function
  mp3.initialize(); //Initialize mp3
}

void loop() {
  // Wait for button press
  if (digitalRead(buttonPin) == LOW) {
    Serial.println("Button pressed! Listening...");
    delay(50); // Debounce delay
    String note = identifyNote(); // Call the function to identify the note
    if (note != "") {
      Serial.print("Detected Note: ");
      Serial.println(note);
    } else {
      Serial.println("No valid note detected.");
    }

    // Check if the sound is from a trumpet
    bool trumpetDetected = isTrumpet();
    if (trumpetDetected) {
      Serial.println("The sound was produced by a trumpet.");
    } else {
      Serial.println("The sound was not produced by a trumpet.");
    }
    
    delay(1000); // Avoid repeated triggers
    // //get track to play based on note
    // if (note == "C"){
    //   digitalWrite(motorPin, HIGH);
    //   mp3.playTrackNumber(1, 30, false);
    //   while(!mp3.playCompleted()){
    //       if (digitalRead(buttonPin) == LOW){
    //         digitalWrite(motorPin, LOW);
    //         mp3.player.stop();
    //         Serial.println("Song stopped");
    //         mp3.player.stop();
    //         break;
    //       }
    //   }
    // }
    // else if (note == "F"){
    //   digitalWrite(motorPin, HIGH);
    //   mp3.playTrackNumber(2, 30, false);
    //   while(!mp3.playCompleted()){
    //       if (digitalRead(buttonPin) == LOW){
    //       digitalWrite(motorPin, LOW);
    //       mp3.player.stop();
    //       Serial.println("Song stopped");
    //       break;
    //       }
    //   }
    // }
    // else{
    //   Serial.println("No song associated with this note.");
    // }
    // delay(500);
  }
}

// Function to identify the note with the highest volume
String identifyNote() {
  unsigned long startTime = millis();
  const int threshold = 600; // Minimum signal level to consider
  float fundamentalFrequency = 0;
  float validPeaks[10] = {0}; // Array to store valid peaks (up to 10 for simplicity)
  int peakCount = 0;

  while (millis() - startTime < 5000) { // Listen for up to 5 seconds
    if (fft1024.available()) {
      // Check for audio input exceeding the threshold
      int audioInput = analogRead(micPin);
      if (audioInput > threshold) {
        // Process FFT to find peaks
        float binMagnitudes[512];
        for (int i = 0; i < 512; i++) {
          binMagnitudes[i] = fft1024.read(i);
        }

        // Peak detection
        for (int i = 1; i < 511; i++) {
          if (binMagnitudes[i] > binMagnitudes[i - 1] && binMagnitudes[i] > binMagnitudes[i + 1] &&
              binMagnitudes[i] > 0.01) { // Consider only peaks with magnitude > 0.01
            float frequency = i * (44100.0 / 1024.0); // Fs / FFT size
            if (frequency > 20.0 && frequency < 5000.0) { // Valid frequency range
              validPeaks[peakCount++] = frequency;
              if (peakCount >= 10) break; // Limit to 10 peaks
            }
          }
        }

        // Harmonic analysis to identify the fundamental
        if (peakCount > 0) {
          for (int i = 0; i < peakCount; i++) {
            bool isFundamental = true;
            for (int j = 0; j < peakCount; j++) {
              if (j != i && fabs(validPeaks[j] / validPeaks[i] - round(validPeaks[j] / validPeaks[i])) < 0.1) {
                // Check if validPeaks[j] is a harmonic of validPeaks[i]
                continue;
              }
              if (j != i && validPeaks[j] < validPeaks[i]) {
                isFundamental = false; // If a smaller peak matches as a harmonic, this is not fundamental
              }
            }
            if (isFundamental) {
              fundamentalFrequency = validPeaks[i];
              break;
            }
          }
        }

        // Determine the closest note to the fundamental frequency
        if (fundamentalFrequency > 20.0 && fundamentalFrequency < 5000.0) { // Valid range for musical notes
          int noteIndex = round(12.0 * log2(fundamentalFrequency / 440.0)) + 69; // MIDI number
          int note = noteIndex % 12; // Note within the octave

          // Construct the note as a string (e.g., "A4")
          return String(noteNames[note]);
        }
      }
    }
  }

  // If no note was detected within 5 seconds
  return "";
}

bool isTrumpet() {
  unsigned long startTime = millis();
  const int threshold = 450;  // Minimum volume level to consider input valid
  float fundamentalFrequency = 0;
  float harmonicAmplitudes[10] = {0}; // Array to store amplitudes of first 10 harmonics
  int peakCount = 0;

  while (millis() - startTime < 5000) { // Listen for up to 5 seconds
    if (fft1024.available()) {
      // Check for audio input exceeding the threshold
      int audioInput = analogRead(micPin);
      if (audioInput > threshold) {
        float binMagnitudes[512];

        // Read FFT bin magnitudes
        for (int i = 0; i < 512; i++) {
          binMagnitudes[i] = fft1024.read(i);
        }

        // Find the fundamental frequency (strongest peak)
        float maxMagnitude = 0;
        int fundamentalBin = 0;
        for (int i = 1; i < 512; i++) {
          if (binMagnitudes[i] > maxMagnitude) {
            maxMagnitude = binMagnitudes[i];
            fundamentalBin = i;
          }
        }
        fundamentalFrequency = fundamentalBin * (44100.0 / 1024.0); // Convert bin to frequency

        // Harmonic analysis
        if (fundamentalFrequency > 20.0 && fundamentalFrequency < 5000.0) {
          for (int h = 1; h <= 5; h++) { // Analyze up to the 5th harmonic
            int harmonicBin = round((fundamentalFrequency * h) / (44100.0 / 1024.0));
            if (harmonicBin < 512) {
              harmonicAmplitudes[h] = binMagnitudes[harmonicBin];
            }
          }

          // Compare harmonic amplitudes to trumpet characteristics
          // Trumpet typically has strong 1st, 2nd, and 3rd harmonics
          if (harmonicAmplitudes[2] > 0.3 * harmonicAmplitudes[1] && // 2nd harmonic is significant
              harmonicAmplitudes[3] > 0.2 * harmonicAmplitudes[1] && // 3rd harmonic is smaller but still significant
              harmonicAmplitudes[4] < 0.15 * harmonicAmplitudes[1] && // Higher harmonics are weaker
              harmonicAmplitudes[5] < 0.1 * harmonicAmplitudes[1]) {
            return true; // The harmonic pattern matches a trumpet
          }
        }
      }
    }
  }
  return false; // No trumpet-like sound detected
}
