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
    Serial.println("Button pressed! Listening for 5 seconds...");
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
  float maxVolume = 0; // Track maximum volume
  float fundamentalFrequency = 0; // Track frequency corresponding to max volume
  const int threshold = 450; // Minimum signal level to consider
  String detectedNote = ""; // Store the detected note

  while (millis() - startTime < 5000) { // Listen for 5 seconds
    if (fft1024.available()) {
      float maxMagnitude = 0;
      int maxBin = 0;

      // Find the bin with the highest magnitude
      for (int i = 0; i < 512; i++) { // FFT1024 gives 512 bins (Nyquist limit)
        float magnitude = fft1024.read(i);
        if (magnitude > maxMagnitude) {
          maxMagnitude = magnitude;
          maxBin = i;
        }
      }

      // Calculate the frequency corresponding to the max bin
      float frequency = maxBin * (44100.0 / 1024.0); // Fs / FFT size

      // Update maxVolume and fundamentalFrequency if this reading has a higher volume
      int audioInput = analogRead(micPin);
      if (audioInput > threshold && maxMagnitude > maxVolume) {
        maxVolume = maxMagnitude;
        fundamentalFrequency = frequency;
      }
    }
  }

  // Determine the closest note to the dominant frequency
  if (fundamentalFrequency > 20.0 && fundamentalFrequency < 5000.0) { // Valid range for musical notes
    int noteIndex = round(12.0 * log2(fundamentalFrequency / 440.0)) + 69; // MIDI number
    int octave = (noteIndex / 12) - 1; // Calculate octave
    int note = noteIndex % 12; // Note within the octave

    // Construct the note as a string (e.g., "A4")
    detectedNote = String(noteNames[note]) + String(octave);
  }

  return detectedNote; // Return the detected note
}
