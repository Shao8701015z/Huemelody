# Huemelody: An Interactive Device for Children Combining Colors and Music


## 🌈 Project Overview

Huemelody is a heptagonal interactive device that transforms colors into musical melodies, enhancing children's multi-sensory experience through color-to-music conversion. The device offers two interaction modes, allowing users to experience how colors from artworks, environments, and creative combinations can be converted into unique musical melodies using AI technology and color sensing.

<div align="center">
  <img src="assets/images/artwork_sensing.jpg" alt="Huemelody sensing artwork cards" width="600"/>
  <p><i>Figure 1: Interactive device Huemelody sensing artwork cards.</i></p>
</div>

Through hands-on interaction with Huemelody, users can explore the potential relationships between colors and sounds, fostering creativity, environmental awareness, and multi-sensory learning in children, providing enriching experiences in both individual and collaborative environments.

[Watch Huemelody in action demonstration video](https://www.youtube.com/watch?v=GMl0PmqDHKo)

## ✨ Features and Benefits

- **Accommodates Diverse Interaction Preferences**: Offers two interaction methods - observation of structured art cards and exploration of environmental colors
- **Multi-sensory Experience**: Simultaneously stimulates visual, auditory, and tactile senses, naturally enhancing focus and engagement
- **Self-directed Exploratory Learning**: Encourages children to discover connections between visual and auditory elements through firsthand exploration
- **Color-to-Music Mapping**: Creates musical melodies based on HSV color characteristics (hue, saturation, brightness)
- **Integration of Art and Technology**: Combines art appreciation with music creation, promoting STEAM education

<div align="center">
  <img src="assets/images/field_testing1.jpg" alt="Children field testing" width="233"/>
  <img src="assets/images/field_testing2.jpg" alt="Children field testing" width="251"/>
  <p><i>Figure 2, 3: The children were listening to the colors of music.</i></p>
</div>

## 🔧 Technical Implementation

Huemelody creates a seamless color-to-sound experience by combining sensing technology with algorithmic composition:

1. **Color Sensing**: Utilizes TCS34725 RGB color sensor to capture color data
2. **Data Processing**: Performs normalization, color space conversion, and classification
3. **Musical Mapping**: Maps colors to musical elements based on the HSV model
   - **Hue**: Determines instruments and scale modes
   - **Saturation**: Influences rhythmic complexity and note density
   - **Value (Brightness)**: Controls pitch range and melodic contour

<div align="center">
  <img src="assets/images/color_to_music_process.png" alt="Color to Music Process" width="600"/>
  <p><i>Figure 6: Interactive device internal process.</i></p>
</div>

### HSV Color Model to Music Mapping

The HSV color model generates music through three components:

**Hue** determines the instrument and mode:
- Red and Orange Hues (0°-60°): Strings/percussion, major modes, bright and majestic
- Yellow and Green Hues (60°-180°): Woodwinds, pentatonic scales, warm and smooth
- Blue and Purple Hues (180°-360°): Brass, minor modes, calm and melancholic

**Saturation** affects rhythmic complexity and note density:
- High Saturation: Dense notes, complex rhythms, fast-paced/jazz
- Low Saturation: Sparse notes, simple rhythms, lyrical/background music

**Value** impacts pitch range and melody contour:
- High Value (Bright): High pitch, ascending melodies, energetic/joyful
- Low Value (Dark): Low pitch, descending melodies, calm/mysterious

## 📐 Physical Design

Huemelody adopts a heptagonal structure, symbolizing Newton's theory of seven primary colors. The device dimensions (46mm height, 68mm diameter) are suitable for single-handed operation by children, with a color sensor located at the bottom.

## 🎨 Interaction Modes

Huemelody offers two primary interaction modes (toggled by long-pressing the top knob):

### 1. Basic Detection Mode

Displays and plays sounds for individual colors, and provides special sound effects for color families based on children's associations (e.g., alarm sounds for reds, dolphin sounds for blues). These special effects are triggered when five different colors from the same color family are consecutively detected.

<div align="center">
  <img src="assets/images/color_cards1.png" alt="Basic Color Cards" width="240"/>
  <img src="assets/images/color_cards2.png" alt="Basic Color Cards" width="235"/>
  <p><i>Figure 10, 11: Basic color scheme cards.</i></p>
</div>

### 2. Art Learning Mode

Collects five key colors from artwork and automatically plays the unique musical melody of that artwork, enhancing the art appreciation experience.

<div align="center">
  <img src="assets/images/artwork_cards1.png" alt="Artwork Cards" width="230"/>
  <img src="assets/images/artwork_cards2.png" alt="Artwork Cards" width="230"/>
  <p><i>Figure 12, 13: Artwork cards introduction in Mandarin.</i></p>
</div>

## 💡 Use Cases

Huemelody has been field-tested in different environments, creating engaging multi-sensory experiences:

- **Classroom Setting**: Children were fascinated by discovering what artworks "sound like," eagerly scanning Munch's paintings and competing to find the key colors that trigger the complete musical piece
- **Outdoor Setting**: The device transformed farm items into musical instruments, with children comparing the sounds of flowers, fruits, and natural materials

## 🔌 Hardware Configuration

- **Microcontroller**: ESP32s3zero (supporting WiFi and Bluetooth connectivity)
- **Color Sensor**: TCS34725 RGB color sensor
- **LED Display**: 12 addressable RGB LEDs (WS2812B)
- **Audio Output**: I2S digital audio
- **User Input**: Rotary encoder (with button)
- **Storage**: MicroSD card (for storing audio files)

## 🛠️ Setup and Operation

### Required Hardware
- ESP32s3zero development board
- TCS34725 RGB color sensor
- WS2812B RGB LED ring (12 LEDs)
- Rotary encoder (EC11 type)
- MAX98357A I2S audio amplifier
- MicroSD card module
- Small speaker
- Lithium battery and charging circuit

### Connection Diagram
Detailed wiring diagrams can be found in the `hardware` folder.

### Software Installation
1. Install Arduino IDE and ESP32s3zero board support
2. Install required libraries:
   - Adafruit_TCS34725
   - FastLED
   - ESP32s3zero-audioI2S
   - SD
3. Upload `src/Huemelody.ino` to ESP32s3zero

### Operation Instructions
- **Short press button**: Start/stop color detection
- **Long press 3 seconds**: Switch operation modes
- **Long press 5 seconds**: Enter sleep mode
- **Rotate encoder**:
  - Rotate while pressing: Adjust volume
  - Rotate without pressing: Switch tracks in audio playback mode

## 🧩 Future Enhancement Plans

- **Hardware Upgrades**: Improve portability, durability, color detection accuracy, and battery life
- **Collaborative Mode**: Wireless communication between multiple devices, allowing group music creation
- **Custom Sound Mapping**: Simple interface for educators to create color-sound relationships targeting specific learning objectives
- **Accessibility Features**: Make Huemelody suitable for children of various abilities, including tactile feedback options and amplified sound settings

## 👥 Team Members

- **Jie-Yu Ching**
  - Department of Creative Technology and Product Design, National Taipei University of Business
  - joycejing1014@gmail.com

- **Shao-Cheng Chen**
  - Department of Creative Technology and Product Design, National Taipei University of Business
  - axs8701015z@gmail.com

- **Prof. Chenwei Chiang**
  - Department of Creative Technology and Product Design, National Taipei University of Business
  - Chenwei@ntub.edu.tw

- **Yi-Qiao Wang**
  - Department of Creative Technology and Product Design, National Taipei University of Business
  - Jasica920812@gmail.com

## 📚 Publication

Jie-Yu Ching, Shao-Cheng Chen, Chenwei Chiang, and Yi-Qiao Wang. 2025. Huemelody: An interactive device for children combining colors and music. Proceedings of the 24th Interaction Design and Children. Association for Computing Machinery, New York, NY, USA, 757–761. https://doi.org/10.1145/3713043.3731477

## Reference
- https://shop.mirotek.com.tw/iot/esp32-start-33/
- https://github.com/adafruit/pianoglove
- https://github.com/helenbang/ColorPiano
- https://www.makerguides.com/interfacing-esp32-and-tcs34725-rgb-color-sensor/#RGBC_Interrupt_Threshold_Registers
- https://www.luisllamas.es/en/arduino-rgb-color-sensor-tcs34725/
- https://www.circuits-diy.com/interfacing-ws2812b-led-ring-with-esp32/
- https://github.com/pschatzmann/ESP32-A2DP
- https://sites.google.com/cies.tn.edu.tw/codingrobot/%E5%85%B6%E4%BB%96%E6%9D%B1%E6%9D%B1/esp32%E9%9F%B3%E9%A0%BB%E5%88%86%E6%9E%90%E6%9D%BF
- https://blog.csdn.net/weixin_42880082/article/details/128639016
- https://dronebotworkshop.com/esp32-i2s/
---

<div align="center">
  <p>© 2025 Shao-Cheng Chen, Huemelody Team. National Taipei University of Business.</p>
</div>


