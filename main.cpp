#include <Audio.h>

// Define the analog pin connected to the microphone
const int micPin = A0;
const int buttonPin = 2; // Pin for the button

// Audio library components
AudioInputAnalog         adc1(micPin);  // Analog input from mic (A2)
AudioAnalyzeFFT1024      fft1024;       // FFT analyzer (1024 bins)
AudioConnection          patchCord1(adc1, fft1024);

// Note names for mapping
const char *noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); // Configure button pin
  Serial.begin(115200);
  AudioMemory(10); // Allocate memory for audio processing
  fft1024.windowFunction(AudioWindowHanning1024); // Set FFT window function
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
    delay(1000); // Avoid repeated triggers
  }
}

// Function to identify the note with the highest volume
String identifyNote() {
  unsigned long startTime = millis();
  const int threshold = 550; // Minimum signal level to consider
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
          int octave = (noteIndex / 12) - 1; // Calculate octave
          int note = noteIndex % 12; // Note within the octave

          // Construct the note as a string (e.g., "A4")
          return String(noteNames[note]) + String(octave);
        }
      }
    }
  }

  // If no note was detected within 5 seconds
  return "";
}

