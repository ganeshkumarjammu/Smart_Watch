#include <Wire.h>
#include "MAX30105.h"
#include "Adafruit_SSD1306.h"
#include "MAX30105.h"
#include "heartRate.h"

// OLED display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// MAX30102 sensor settings
MAX30105 particleSensor; // Using MAX30105 class for MAX30102
#define MAX30102_I2C_ADDRESS 0x57

// Heart rate and SpO2 calculation variables
const byte RATE_SIZE = 4; // Increase this for more averaging
byte rates[RATE_SIZE];    // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;        // Time of the last beat
float beatsPerMinute;
int beatAvg;

// Temperature and presence variables
float temperature;
bool presenceDetected = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize rates array
  for (byte i = 0; i < RATE_SIZE; i++) {
    rates[i] = 0;
  }

  // Initialize I2C
  Wire.begin();
  Wire.setClock(400000); // Fast mode I2C (400kHz)

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Initializing..."));
  display.display();

  // Initialize MAX30102 sensor
  if (!particleSensor.begin(Wire, 400000, MAX30102_I2C_ADDRESS)) {
    Serial.println("MAX30102 not found. Check wiring!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("MAX30102 not found"));
    display.display();
    for (;;);
  }

  // Configure sensor
  particleSensor.setup(); // Use default settings
  particleSensor.setPulseAmplitudeRed(0x0A); // Adjust LED brightness
  particleSensor.setPulseAmplitudeGreen(0);   // Turn off green LED
}

void loop() {
  // Read sensor data
  long irValue = particleSensor.getIR();
  long redValue = particleSensor.getRed();
  
  // Read temperature
  temperature = particleSensor.readTemperature();

  // Presence detection (basic threshold-based)
  presenceDetected = (irValue > 50000); // Adjust threshold as needed

  // Heart rate detection
  if (checkForBeat(irValue) == true) { // Requires SparkFun MAX3010x library
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60000.0 / delta; // Simplified BPM calculation

    if (beatsPerMinute < 200 && beatsPerMinute > 30) { // Tighter BPM range
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      // Calculate average heart rate
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;
    }
  }

  // Reset heart rate if no beat detected for 5 seconds
  if (millis() - lastBeat > 5000) {
    beatAvg = 0;
    rateSpot = 0;
    for (byte i = 0; i < RATE_SIZE; i++) {
      rates[i] = 0;
    }
  }

  // Update OLED display
  display.clearDisplay();
  display.setCursor(0, 0);

  // Display presence
  display.print(F("Presence: "));
  display.println(presenceDetected ? F("Yes") : F("No"));

  // Display temperature
  display.print(F("Temp: "));
  display.print(temperature);
  display.println(F(" C"));

  // Display heart rate
  display.print(F("BPM: "));
  if (beatAvg > 0) {
    display.println(beatAvg);
  } else {
    display.println(F("N/A"));
  }

  // Simple heart rate plot
  static int plotY[SCREEN_WIDTH];
  static int plotIndex = 0;
  if (presenceDetected) {
    int plotValue = map(irValue, 50000, 100000, 0, SCREEN_HEIGHT / 2);
    plotY[plotIndex] = constrain(plotValue, 0, SCREEN_HEIGHT / 2 - 1);
    for (int i = 0; i < SCREEN_WIDTH - 1; i++) {
      int x0 = i;
      int x1 = i + 1;
      int y0 = SCREEN_HEIGHT - plotY[i];
      int y1 = SCREEN_HEIGHT - plotY[(i + 1) % SCREEN_WIDTH];
      display.drawLine(x0, y0, x1, y1, SSD1306_WHITE);
    }
    plotIndex = (plotIndex + 1) % (SCREEN_WIDTH - 1);
  }

  display.display();

  // Serial output for debugging
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", Temp=");
  Serial.print(temperature);
  Serial.print(", BPM=");
  Serial.print(beatAvg);
  Serial.print(", Presence=");
  Serial.println(presenceDetected ? "Yes" : "No");

  delay(20); // Small delay to prevent overwhelming the I2C bus
}