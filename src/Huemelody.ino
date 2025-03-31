/*
 * ColorSense: Interactive Color Learning Device
 * ACM IDC 2025 Demo Submission
 * 
 * This code implements a color recognition device with two modes:
 * 1. Color Recognition Mode: Identifies colors and provides audio feedback
 * 2. Painter Collection Mode: Gamifies color learning through painter palettes
 */

#include <Wire.h>
#include "Adafruit_TCS34725.h"
#include "FastLED.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

// =====================================================================
// 1. HARDWARE CONFIGURATION
// =====================================================================

// Pin definitions
#define DATA_PIN 2       // LED data pin
#define NUM_LEDS 12      // Number of LEDs in ring
#define SDA_PIN 8        // I2C data
#define SCL_PIN 9        // I2C clock
#define EC11_SW_PIN 10   // Rotary encoder button
#define EC11_A_PIN 11    // Rotary encoder A
#define EC11_B_PIN 3     // Rotary encoder B
#define SD_CS 5          // SD card chip select
#define SPI_MOSI 13      // SPI MOSI
#define SPI_MISO 12      // SPI MISO
#define SPI_SCK 14       // SPI clock
#define I2S_DOUT 6       // I2S data
#define I2S_BCLK 7       // I2S bit clock
#define I2S_LRC 4        // I2S word clock

// LED configuration
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define BRIGHTNESS 255
#define FRAMES_PER_SECOND 250

// Button timing constants
#define MODE_SWITCH_PRESS_DURATION 3000  // 3 seconds for mode switch
#define SLEEP_PRESS_DURATION 5000        // 5 seconds for sleep mode
#define ROTATION_DEBOUNCE 10             // Debounce for rotation
#define TRACK_NAVIGATION_DEBOUNCE 100    // Debounce for track navigation

// Audio and volume constants
#define MIN_VOLUME 0
#define MAX_VOLUME 21
#define VOLUME_STEP 1
#define VOLUME_DISPLAY_DURATION 1000

// Color recognition constants
#define COLOR_COUNT_THRESHOLD 5          // Trigger special audio after N detections
#define COLOR_FAMILY_COUNT 16            // Total number of color families

FASTLED_USING_NAMESPACE

// =====================================================================
// 2. DATA STRUCTURES
// =====================================================================

// Operation modes
enum OperationMode {
  COLOR_RECOGNITION = 1,
  PAINTER_COLLECTION = 2
};

// Persistent data across sleep cycles
RTC_DATA_ATTR int bootCount = 0;

// Color step structure - defines the sequence of tone steps for each color family
struct ColorSteps {
  const char* colorPrefix;    // Color family prefix (R, G, B, etc.)
  int steps[10];              // Array of steps (1, 2, 3, 4, 6, 8, 12, 16)
  int stepCount;              // Number of steps in this family
};

// Color threshold structure - defines RGB ranges for color detection
struct ColorThreshold {
  uint8_t rMin, rMax;         // Red channel range
  uint8_t gMin, gMax;         // Green channel range
  uint8_t bMin, bMax;         // Blue channel range
  const char* colorId;        // Color ID for display
  const char* audioFile;      // Audio filename without .mp3
};

// Painter color structure - defines RGB ranges for painter palette colors
struct PainterColor {
  uint8_t rMin, rMax;
  uint8_t gMin, gMax;
  uint8_t bMin, bMax;
  const char* painterId;      // Painter name
  uint8_t colorIndex;         // Index within painter's palette
};

// Calibration data structure
struct CalibrationData {
  float redCoef;
  float greenCoef;
  float blueCoef;
  bool isCalibrated;
};

// =====================================================================
// 3. COLOR STEP SEQUENCES - DEFINES TONAL PROGRESSION
// =====================================================================

const ColorSteps colorStepSequences[] = {
  // Basic color families
  {"R", {1, 2, 3, 4, 6, 8, 12, 16, 0}, 8},  // Red series has 8 steps
  {"O", {1, 2, 3, 4, 6, 8, 12, 16, 0}, 8},  // Orange series
  {"Y", {1, 2, 3, 4, 6, 8, 12, 16, 0}, 8},  // Yellow series
  {"G", {1, 2, 3, 4, 6, 8, 12, 0}, 7},      // Green series
  {"B", {1, 2, 3, 4, 6, 8, 12, 16, 0}, 8},  // Blue series
  {"V", {1, 2, 3, 4, 6, 8, 12, 16, 0}, 8},  // Violet series
  
  // Mixed color families
  {"RO", {1, 2, 3, 4, 6, 8, 0}, 6},         // Red-Orange series
  {"OY", {1, 2, 3, 4, 6, 8, 12, 16, 0}, 8}, // Orange-Yellow series
  {"YG", {1, 2, 3, 4, 6, 8, 12, 0}, 7},     // Yellow-Green series
  {"GB", {1, 2, 3, 4, 6, 8, 0}, 6},         // Green-Blue series
  {"BV", {1, 2, 3, 4, 6, 8, 0}, 6},         // Blue-Violet series
  {"VR", {1, 2, 3, 4, 6, 8, 12, 16, 0}, 8}, // Violet-Red series
};

// Calibration values for optimal color detection
CalibrationData calibration = {
  1.88,  // redCoef
  1.00,  // greenCoef
  1.21,  // blueCoef
  true   // isCalibrated
};

// =====================================================================
// 4. HARDWARE INITIALIZATION
// =====================================================================

// Hardware objects
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_101MS, TCS34725_GAIN_1X);
CRGB leds[NUM_LEDS];
uint8_t gammatable[256];  // Gamma correction table
Audio audio;

// State variables
OperationMode currentMode = COLOR_RECOGNITION;
bool isColorDetectionEnabled = false;
bool isLooping = false;
char currentPlayingFile[32] = "";
int currentVolume = 19;  // Default volume

// LED display state
bool isShowingVolume = false;
unsigned long volumeDisplayStartTime = 0;
CRGB originalLedColors[NUM_LEDS];

// Button state variables
bool lastButtonState = HIGH;
unsigned long buttonPressStartTime = 0;
bool isLongPressing = false;
unsigned long sleepPressStartTime = 0;
bool isSleepPressing = false;

// Encoder tracking
bool isEncoderPressed = false;
bool volumeAdjustedWhilePressed = false;
unsigned long lastRotationTime = 0;
unsigned long lastTrackNavigationTime = 0;

// Color tracking
uint8_t colorCounters[COLOR_FAMILY_COUNT] = {0};
bool painterCollectionStatus[6][5] = {false};  // 6 painters, 5 colors each

// =====================================================================
// 5. SETUP FUNCTION
// =====================================================================

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("Starting up..."));
  
  // Initialize LED strip first for wake animation
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  
  // Log boot information
  bootCount++;
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  Serial.println("Boot count: " + String(bootCount));
  
  // Play wake-up animation regardless of wake reason
  playWakeUpAnimation();
  
  // Compute gamma correction table
  for (int i = 0; i < 256; i++) {
    float x = i;
    x /= 255;
    x = pow(x, 2.5);
    x *= 255;
    gammatable[i] = x;
  }
  
  // Configure white balance
  FastLED.setTemperature(Tungsten100W);
  
  // Set up GPIO pins
  pinMode(EC11_SW_PIN, INPUT_PULLUP);
  pinMode(EC11_A_PIN, INPUT_PULLUP);
  pinMode(EC11_B_PIN, INPUT_PULLUP);
  
  // Initialize I2C and color sensor
  Wire.begin(SDA_PIN, SCL_PIN);
  if (tcs.begin()) {
    Serial.println(F("Found color sensor"));
  } else {
    Serial.println(F("No TCS34725 found... check connections"));
    while (1); // Halt if sensor not found
  }
  
  // Initialize SD card
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  
  if (!SD.begin(SD_CS)) {
    Serial.println(F("Error accessing microSD card!"));
    // Flash red LED to indicate error
    for (int i = 0; i < 1; i++) {
      fill_solid(leds, NUM_LEDS, CRGB::Red);
      FastLED.show();
      delay(150);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      delay(150);
    }
  }
  
  // Initialize audio system
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(currentVolume);
  
  // Check for audio files
  if (!checkAudioFiles()) {
    Serial.println(F("One or more audio files are missing!"));
    // Flash yellow LED to indicate error
    for (int i = 0; i < 1; i++) {
      fill_solid(leds, NUM_LEDS, CRGB::Yellow);
      FastLED.show();
      delay(150);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      FastLED.show();
      delay(150);
    }
  }
  
  randomSeed(analogRead(0));
}

// =====================================================================
// 6. MAIN LOOP
// =====================================================================

void loop() {
  // Handle user input
  handleButton();
  handleEncoder();
  
  // Check if volume display timeout has expired
  if (isShowingVolume && (millis() - volumeDisplayStartTime > VOLUME_DISPLAY_DURATION)) {
    restoreOriginalLeds();
  }
  
  if (isColorDetectionEnabled) {
    // Read color sensor and update LEDs
    readColor();
    calculateLeds();
    adjustBrightness();
    FastLED.show();
    printColorInfo();
    
    // Process color based on current mode
    if (currentMode == COLOR_RECOGNITION) {
      checkColor(colorThresholds, sizeof(colorThresholds) / sizeof(ColorThreshold));
    } else { // PAINTER_COLLECTION mode
      // Check for painter colors
      checkPainterColor(daliColors, sizeof(daliColors) / sizeof(PainterColor));
      checkPainterColor(goghColors, sizeof(goghColors) / sizeof(PainterColor));
      checkPainterColor(munchColors, sizeof(munchColors) / sizeof(PainterColor));
      checkPainterColor(picassoColors, sizeof(picassoColors) / sizeof(PainterColor));
      checkPainterColor(seuratColors, sizeof(seuratColors) / sizeof(PainterColor));
      checkPainterColor(vinciColors, sizeof(vinciColors) / sizeof(PainterColor));
      
      // Check if any painter collection is complete
      checkPainterCompletion();
    }
  } else {
    // Handle audio playback when not sensing
    if (isLooping) {
      if (!audio.isRunning()) {
        if (currentPlayingFile[0] != '\0') {
          if (!audio.connecttoFS(SD, currentPlayingFile)) {
            Serial.printf("Failed to restart: %s\n", currentPlayingFile);
            isLooping = false;
          }
        }
      } else {
        audio.loop();
      }
    }
  }
  
  // Ensure audio system is serviced
  if (audio.isRunning()) {
    audio.loop();
  }
}

// =====================================================================
// 7. COLOR SENSING FUNCTIONS
// =====================================================================

// Read color from sensor and apply calibration
void readColor() {
  uint16_t red, green, blue;
  tcs.setInterrupt(false);  // Turn on LED
  delay(20);                // Integration time
  tcs.getRawData(&red, &green, &blue, &clear);
  tcs.setInterrupt(true);   // Turn off LED
  
  uint32_t sum = clear;
  
  // Apply calibration coefficients
  r = (red / (float)sum) * calibration.redCoef * 256;
  g = (green / (float)sum) * calibration.greenCoef * 256;
  b = (blue / (float)sum) * calibration.blueCoef * 256;
  
  lux = tcs.calculateLux(red, green, blue);
}

// Set LEDs to detected color
void calculateLeds() {
  fill_solid(leds, NUM_LEDS, CRGB(gammatable[(int)r], gammatable[(int)g], gammatable[(int)b]));
}

// Adjust brightness based on ambient light
void adjustBrightness() {
  uint8_t brightness = map(lux, 0, 4000, 10, BRIGHTNESS);
  brightness = constrain(brightness, 10, BRIGHTNESS);
  FastLED.setBrightness(brightness);
}

// =====================================================================
// 8. COLOR DETECTION FUNCTIONS
// =====================================================================

// Check if current color matches any defined color threshold
bool checkColor(const ColorThreshold* thresholds, int count) {
  for (int i = 0; i < count; i++) {
    const ColorThreshold& t = thresholds[i];
    
    // Skip undefined color ranges
    if (t.rMin == 0 && t.rMax == 0) continue;
    
    if (r >= t.rMin && r <= t.rMax && 
        g >= t.gMin && g <= t.gMax && 
        b >= t.bMin && b <= t.bMax) {
      Serial.print(t.colorId);
      
      // Play audio for the detected color
      playAudio(t.audioFile);
      return true;
    }
  }
  return false;
}

// Check if current color matches a painter's palette color
bool checkPainterColor(const PainterColor* thresholds, int count) {
  for (int i = 0; i < count; i++) {
    const PainterColor& t = thresholds[i];
    if (r >= t.rMin && r <= t.rMax && 
        g >= t.gMin && g <= t.gMax && 
        b >= t.bMin && b <= t.bMax) {
      
      // Map painter name to index
      int painterIndex = -1;
      if (strcmp(t.painterId, "Dali") == 0) painterIndex = 0;
      else if (strcmp(t.painterId, "Gogh") == 0) painterIndex = 1;
      else if (strcmp(t.painterId, "Munch") == 0) painterIndex = 2;
      else if (strcmp(t.painterId, "Picasso") == 0) painterIndex = 3;
      else if (strcmp(t.painterId, "Seurat") == 0) painterIndex = 4;
      else if (strcmp(t.painterId, "Vinci") == 0) painterIndex = 5;
      
      if (painterIndex >= 0) {
        // Mark this color as collected
        painterCollectionStatus[painterIndex][t.colorIndex - 1] = true;
        Serial.printf(" Collected %s%d", t.painterId, t.colorIndex);
        
        // Optional: play a collection sound
        return true;
      }
    }
  }
  return false;
}

// Check if any painter's collection is complete
void checkPainterCompletion() {
  // Check each painter's collection status
  for (int painter = 0; painter < 6; painter++) {
    bool complete = true;
    for (int color = 0; color < 5; color++) {
      if (!painterCollectionStatus[painter][color]) {
        complete = false;
        break;
      }
    }
    
    if (complete) {
      // Painter collection complete - visual feedback
      blinkLEDs(2);
      
      // Play the painter's audio information
      const char* painterNames[] = {"P_Dali", "P_Gogh", "P_Munch", "P_Picasso", "P_Seurat", "P_Vinci"};
      Serial.printf("%s collection complete! Playing %s\n", painterNames[painter] + 2, painterNames[painter]);
      
      playAudio(painterNames[painter]);
      isColorDetectionEnabled = false;
      
      // Reset this painter's collection status
      for (int color = 0; color < 5; color++) {
        painterCollectionStatus[painter][color] = false;
      }
    }
  }
}

// =====================================================================
// 9. AUDIO PLAYBACK FUNCTIONS
// =====================================================================

// Play audio for a detected color
void playAudio(const char* colorName) {
  char baseColor[10] = {0};
  getBaseColorName(colorName, baseColor, sizeof(baseColor));
  
  int colorIndex = getColorFamilyIndex(colorName);
  
  // Handle special audio in Color Recognition mode
  if (currentMode == COLOR_RECOGNITION && colorIndex >= 0) {
    colorCounters[colorIndex]++;
    
    if (colorCounters[colorIndex] >= COLOR_COUNT_THRESHOLD) {
      // Play special "X" audio after multiple detections
      colorCounters[colorIndex] = 0;
      snprintf(currentPlayingFile, sizeof(currentPlayingFile), "/%sX.mp3", baseColor);
    } else {
      // Play normal color audio
      snprintf(currentPlayingFile, sizeof(currentPlayingFile), "/%s.mp3", colorName);
    }
  } else {
    // Direct playback or painter audio
    snprintf(currentPlayingFile, sizeof(currentPlayingFile), "/%s.mp3", colorName);
  }
  
  playTrack(currentPlayingFile);
}

// Play a specific audio track
void playTrack(const char* filename) {
  if (!SD.exists(filename)) {
    Serial.print(F("File does not exist: "));
    Serial.println(filename);
    return;
  }
  
  // Stop current audio
  audio.stopSong();
  
  // Save new track name
  strncpy(currentPlayingFile, filename, sizeof(currentPlayingFile) - 1);
  currentPlayingFile[sizeof(currentPlayingFile) - 1] = '\0';
  
  if (!audio.connecttoFS(SD, currentPlayingFile)) {
    Serial.print(F("Failed to open "));
    Serial.println(currentPlayingFile);
    isLooping = false;
  } else {
    isLooping = true;
    Serial.print(F("Playing: "));
    Serial.println(currentPlayingFile);
    
    // Reset collection status in painter mode
    if (currentMode == PAINTER_COLLECTION) {
      resetAllPainterCollections();
    }
  }
}

// =====================================================================
// 10. USER INTERFACE FUNCTIONS
// =====================================================================

// Handle button presses
void handleButton() {
  bool currentButtonState = digitalRead(EC11_SW_PIN);
  
  // Button press handling
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    buttonPressStartTime = millis();
    isLongPressing = true;
    sleepPressStartTime = millis();
    isSleepPressing = true;
    isEncoderPressed = true;
    volumeAdjustedWhilePressed = false;
  }
  
  // Check for sleep mode trigger
  if (isSleepPressing && currentButtonState == LOW) {
    if (millis() - sleepPressStartTime >= SLEEP_PRESS_DURATION) {
      Serial.println(F("Entering deep sleep mode..."));
      prepareForSleep();
      enterDeepSleep();
    }
  }
  
  // Long press detection for mode switching
  if (isLongPressing && currentButtonState == LOW) {
    if (millis() - buttonPressStartTime >= MODE_SWITCH_PRESS_DURATION) {
      // Only switch mode if no volume adjustment happened
      if (!volumeAdjustedWhilePressed) {
        switchMode();
      }
      isLongPressing = false;
    }
  }
  
  // Button release handling
  if (currentButtonState == HIGH && lastButtonState == LOW) {
    isEncoderPressed = false;
    
    // Short press handling
    if (millis() - buttonPressStartTime < MODE_SWITCH_PRESS_DURATION && !volumeAdjustedWhilePressed) {
      isColorDetectionEnabled = !isColorDetectionEnabled;
      
      // Reset collections in painter mode
      if (currentMode == PAINTER_COLLECTION) {
        resetAllPainterCollections();
        Serial.println(F("Reset all painter collections in Mode 2"));
      }
      
      if (!isColorDetectionEnabled) {
        // Update LEDs with current color
        for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB(gammatable[(int)r], gammatable[(int)g], gammatable[(int)b]);
        }
        FastLED.show();
        
        if (currentMode == COLOR_RECOGNITION) {
          checkColorAndPlayAudio();
        }
      } else {
        isLooping = false;
        audio.stopSong();
      }
    }
    
    // Reset flags
    volumeAdjustedWhilePressed = false;
    isLongPressing = false;
    isSleepPressing = false;
  }
  
  lastButtonState = currentButtonState;
}

// Handle rotary encoder
void handleEncoder() {
  static uint8_t lastEncoded = 0;
  static int lastDir = 0;
  static int validRotations = 0;
  
  // Read encoder state
  uint8_t MSB = digitalRead(EC11_A_PIN);
  uint8_t LSB = digitalRead(EC11_B_PIN);
  
  // Combine into one value
  uint8_t encoded = (MSB << 1) | LSB;
  
  // Detect rotation direction
  uint8_t sum = (lastEncoded << 2) | encoded;
  int dir = 0;
  
  // Determine direction using state table
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    dir = 1;  // Clockwise
  } else if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    dir = -1; // Counter-clockwise
  }
  
  // Save encoder state
  lastEncoded = encoded;
  
  // Direction confirmation logic
  if(dir != 0) {
    // Increment counter for consistent rotation
    if(dir == lastDir) {
      validRotations++;
      
      // When sufficient rotations are detected
      if(validRotations >= 1) {
        unsigned long currentTime = millis();
        
        if(isEncoderPressed) {
          // Volume control when encoder is pressed
          if(currentTime - lastRotationTime > ROTATION_DEBOUNCE) {
            if(dir > 0) {
              // Decrease volume (clockwise)
              if(currentVolume > MIN_VOLUME) {
                currentVolume = currentVolume - VOLUME_STEP;
                audio.setVolume(currentVolume);
                Serial.println(currentVolume);
                displayVolumeOnLeds();
                volumeAdjustedWhilePressed = true;
              }
            } else {
              // Increase volume (counter-clockwise)
              if(currentVolume < MAX_VOLUME) {
                currentVolume = currentVolume + VOLUME_STEP;
                audio.setVolume(currentVolume);
                Serial.println(currentVolume);
                displayVolumeOnLeds();
                volumeAdjustedWhilePressed = true;
              }
            }
            validRotations = 0;
            lastRotationTime = currentTime;
          }
        } else {
          // Track navigation when encoder is not pressed
          if(currentTime - lastTrackNavigationTime > TRACK_NAVIGATION_DEBOUNCE && !isColorDetectionEnabled) {
            if(dir > 0) {
              // Next track (clockwise)
              playNextTrack();
            } else {
              // Previous track (counter-clockwise)
              playPreviousTrack();
            }
            validRotations = 0;
            lastTrackNavigationTime = currentTime;
          }
        }
      }
    } else {
      // Direction changed, reset counter and record new direction
      lastDir = dir;
      validRotations = 2;
    }
  }
}

// =====================================================================
// 11. POWER MANAGEMENT
// =====================================================================

// Prepare for sleep mode
void prepareForSleep() {
  // Step 1: Make all LEDs white
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.setBrightness(120);
  FastLED.show();
  delay(200);
  
  // Step 2: Gradually decrease brightness
  for (int brightness = 120; brightness >= 0; brightness -= 5) {
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(30);
  }
  
  // Step 3: Ensure all LEDs are off
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  
  // Stop audio playback
  audio.stopSong();
  
  // Disable color sensor
  tcs.disable();
  
  // Wait for serial output to complete
  Serial.println(F("Going to sleep now"));
  Serial.flush();
}

// Enter deep sleep mode
void enterDeepSleep() {
  // Set GPIO10 as wake-up source (low level trigger)
  gpio_pullup_en(GPIO_NUM_10);
  gpio_pulldown_dis(GPIO_NUM_10);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_10, 0);
  
  // Short delay to ensure button fully released
  delay(500);
  
  // Enter deep sleep
  esp_deep_sleep_start();
}

// =====================================================================
// 12. VISUAL FEEDBACK FUNCTIONS
// =====================================================================

// Wake-up animation
void playWakeUpAnimation() {
  // Start with all LEDs white but brightness 0
  fill_solid(leds, NUM_LEDS, CRGB::White);
  FastLED.setBrightness(0);
  FastLED.show();
  
  // Gradually increase brightness
  for (int brightness = 0; brightness <= 120; brightness += 5) {
    FastLED.setBrightness(brightness);
    FastLED.show();
    delay(50);
  }
  
  // Flash twice to indicate wake-up complete
  for (int i = 0; i < 2; i++) {
    FastLED.setBrightness(0);
    FastLED.show();
    delay(150);
    FastLED.setBrightness(100);
    FastLED.show();
    delay(150);
  }
}

// Display volume level on LED ring
void displayVolumeOnLeds() {
  // Save original LED colors
  if (!isShowingVolume) {
    memcpy(originalLedColors, leds, sizeof(CRGB) * NUM_LEDS);
    isShowingVolume = true;
  }
  
  // Calculate number of LEDs to light based on volume
  int ledsToLight = map(currentVolume, MIN_VOLUME, MAX_VOLUME, 0, NUM_LEDS);
  
  // Clear all LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Light LEDs for volume indication (white)
  for (int i = 0; i < ledsToLight; i++) {
    leds[i] = CRGB::White;
    FastLED.setBrightness(100);
  }
  
  // Update display
  FastLED.show();
  
  // Record display start time
  volumeDisplayStartTime = millis();
}

// Restore original LED display after volume indication
void restoreOriginalLeds() {
  if (isShowingVolume) {
    // Restore original colors
    memcpy(leds, originalLedColors, sizeof(CRGB) * NUM_LEDS);
    FastLED.show();
    isShowingVolume = false;
  }
}

// Mode switch indicator - non-blocking LED blink function
void blinkLEDs(int count) {
  // Save original colors
  CRGB originalColors[NUM_LEDS];
  memcpy(originalColors, leds, sizeof(CRGB) * NUM_LEDS);
  
  // Blink specified number of times
  for (int j = 0; j < count; j++) {
    // On
    fill_solid(leds, NUM_LEDS, CRGB::White);
    FastLED.show();
    delay(150);
    
    // Off
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
    delay(150);
  }
  
  // Restore original colors
  memcpy(leds, originalColors, sizeof(CRGB) * NUM_LEDS);
  FastLED.show();
}

// =====================================================================
// 13. MODE MANAGEMENT
// =====================================================================

// Switch between operation modes
void switchMode() {
  // Stop current audio
  audio.stopSong();
  isLooping = false;
  currentPlayingFile[0] = '\0';
  
  // Toggle mode
  currentMode = (currentMode == COLOR_RECOGNITION) ? PAINTER_COLLECTION : COLOR_RECOGNITION;
  
  // Reset all counters
  memset(colorCounters, 0, sizeof(colorCounters));
  
  // Visual feedback - blink once for mode 1, twice for mode 2
  blinkLEDs((currentMode == COLOR_RECOGNITION) ? 1 : 2);
  
  // Reset collection status in painter mode
  if (currentMode == PAINTER_COLLECTION) {
    resetAllPainterCollections();
  }
  
  Serial.printf("Switched to Mode %d\n", currentMode);
}

// Reset all painter collection statuses
void resetAllPainterCollections() {
  memset(painterCollectionStatus, 0, sizeof(painterCollectionStatus));
}