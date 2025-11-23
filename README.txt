HEAD-TILT RACING GAME (ESP32-CAM)
===================================
A browser-based racing game controlled by head movements, powered by ESP32-CAM.

FILES INCLUDED
--------------
1. esp32cam_capture.ino  - The main code to upload to the ESP32.
2. web_index.h           - The game website (embedded inside the ESP32).
3. index.html            - A copy of the game code (for viewing/editing).

QUICK START GUIDE
-----------------
1. PREPARE WIFI
   - Create a Mobile Hotspot on your laptop (or use a classroom WiFi).
   - Note down the SSID (Name) and Password.

2. EDIT CODE
   - Open `esp32cam_capture.ino` in Arduino IDE.
   - Find lines 20-21:
     const char* ssid = "YOUR_WIFI_NAME";
     const char* password = "YOUR_WIFI_PASSWORD";
   - Replace with your actual Hotspot details.

3. UPLOAD
   - Board: "AI Thinker ESP32-CAM"
   - Connect ESP32-CAM to PC (using FTDI programmer).
   - Press Upload. (If it fails, hold the RST button on the ESP32 when "Connecting..." appears).

4. PLAY
   - Open the Serial Monitor (Baud Rate: 115200).
   - Press the RST button on the ESP32.
   - Wait for "WiFi connected".
   - It will print an IP address (e.g., http://192.168.137.145).
   - Open that link in Chrome/Edge on your laptop.
   - Allow Camera permissions and start racing!

HOW TO PLAY
-----------
- The game uses the camera to track your head.
- Tilt your head LEFT to move to the Left Lane.
- Tilt your head RIGHT to move to the Right Lane.
- Keep your head STRAIGHT to stay in the Center.
- Avoid the red obstacles!

TROUBLESHOOTING
---------------
1. "Camera Init Failed": 
   - Check your wiring. Ensure the ESP32 has enough power (5V recommended).
   - Press the RST button on the module.

2. Game loads but "Waiting for Camera...":
   - The browser might be blocking the insecure content (http).
   - Check the Serial Monitor to see if the ESP32 is still connected to WiFi.

3. Face not detected:
   - Ensure good lighting on your face.
   - Don't sit too far back (40-60cm is best).
   - If the game is laggy, try making the browser window smaller.

FOR TEACHERS
------------
- This project demonstrates: IoT (ESP32), Computer Vision (Face Mesh), and Web Development.
- Challenge for students: 
  - Change the car colors in `web_index.h` (search for "#00d2ff").
  - Change the speed or sensitivity in the HTML code.
  - Note: If you edit `index.html`, you must copy the code into `web_index.h` to see changes on the ESP32.

LIMITATIONS
-----------
- Frame rate depends on WiFi signal quality.
- The AI runs on the laptop, so a decent CPU is required.
