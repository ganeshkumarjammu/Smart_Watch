#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include "Adafruit_SSD1306.h"
#define OLED_RESET -1
Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);
PulseOximeter pox;
void onBeatDetected() {
  Serial.println("Beat detected!");
}
void setup() {
  Serial.begin(115200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  if (!pox.begin()) {
    Serial.println("Failed to initialize pulse oximeter");
    while (1)
      ;
  }
  pox.setOnBeatDetectedCallback(onBeatDetected);
}
void loop() {
  pox.update();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("HR:");
  display.print(" bpm");
  display.setCursor(0, 20);
  display.print("SpO2:");
  display.print(pox.getSpO2());
  display.print(" %");
  display.display();
  delay(1000);
}
