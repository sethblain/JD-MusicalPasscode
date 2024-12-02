// Define the analog pin connected to the microphone
const int micPin = A0;

//Audio library components
AudioInputAnalog         adc1(micPin);  // Analog input from mic (A2)
AudioAnalyzeFFT1024      fft1024;   // FFT analyzer (1024 bins)
AudioConnection          patchCord1(adc1, fft1024);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(micPin, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  int audioInput = analogRead(micPin); // Read audio signal
  Serial.println(audioInput); // Print the value to Serial Monitor
  delay(10); // Small delay for readability
}
