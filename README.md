# Head_Tilt_Racing_Game_EspCam

Welcome to the Head Tilt Racing Game! This project uses an [ESP32-CAM](https://espressif.com/en/products/socs/esp32-cam) microcontroller with camera capabilities to create an interactive tilt-controlled racing game. Players steer the racing car using the natural movement of their head, detected by the onboard camera. This project demonstrates embedded vision, playful hardware interaction, and real-time game logic – perfect for makers, educators, and enthusiasts.

## Features

- **Head-tilt controls:** Players tilt their head left/right to steer the car.
- **Real-time tracking:** Uses the ESP32-CAM camera and computer vision to detect head movement.
- **Embedded gaming:** Everything runs on the ESP32-CAM — no external PC required.
- **Simple graphics:** Racing track and car rendered using available display method (serial, TFT, etc).
- **Lightweight code:** Optimized for ESP32-CAM memory constraints.

## Getting Started

### Hardware Required

- **ESP32-CAM module**
- FTDI programmer (for initial flashing)
- Optional: TFT/LCD display for game visualization
- USB cables, breadboard, and jumper wires

### Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software)
- ESP32 board definitions installed (via Board Manager)
- Relevant libraries:
  - `esp32cam.h`
  - `Adafruit_GFX`
  - (if using display) `Adafruit_ILI9341` or similar

### Installation

1. **Clone this repo:**
   ```bash
   git clone https://github.com/jrc1203/Head_Tilt_Racing_Game_EspCam.git
   ```
2. **Open the code in Arduino IDE.**
3. **Install any missing libraries** via Arduino Library Manager.
4. **Select the ESP32-CAM board** (`AI Thinker ESP32-CAM`) in the IDE.
5. **Connect hardware** and configure your FTDI programmer for flashing.
6. **Upload the sketch** to your ESP32-CAM.

### Usage

- Position the ESP32-CAM so it has good visibility of your face.
- Power up and reset the board.
- Observe the output: The game will start, and your head tilts will steer the car on the track!
- (If using serial monitor) Watch car position updates in the console.
- (If using display) View the game on the attached screen.

## Folder Structure

```
/
├── src/                 # Source code files (.ino, .cpp, .h)
├── assets/              # Game assets, images
├── docs/                # Documentation, schematics
├── README.md
└── LICENSE
```

## How It Works

- The camera captures frames and runs a simple detection routine (may use color, movement, or lightweight ML).
- Head left/right motion is mapped to car movement on the virtual track.
- The game logic updates the car and obstacles, advancing through the race.

## Customization

- Change the race track layout in code.
- Implement other visual cues, such as "head nod" for acceleration/brake.
- Adapt the graphics renderer for different displays (TFT, serial, web).

## Troubleshooting

- Ensure good lighting for the camera.
- Face detection may require calibration or library updates for accuracy.
- For FTDI programming, connect GPIO0 to GND during flashing.

## Contributing

Pull requests, issues, and suggestions are welcome! Please fork the repo and propose your ideas.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

**Made with ESP32-CAM and a bit of tilt!**
