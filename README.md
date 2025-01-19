
# ESP32-CAM Elephant Detection

This project utilizes an ESP32-CAM module to capture images and analyze them for the presence of elephants. The image is sent to Gemini API for analysis, and based on the response, an action is triggered.

## Features
- Captures an image using the ESP32-CAM module.
- Sends the captured image to Gemini API for analysis.
- Receives a response from the API, which answers whether the image contains an elephant.
- Controls a GPIO pin (for example, turning on a relay or other connected device) based on the API's response.

## Components
- **ESP32-CAM**: Camera module used for image capture.
- **WiFi**: Connects the ESP32-CAM to the internet for API communication.
- **Gemini API**: Used for processing the image and detecting if an elephant is present.
- **GPIO**: The output (like an LED or relay) is controlled based on the analysis result.

## Requirements
- ESP32 board (ESP32-CAM in this case).
- Camera module.
- A Wi-Fi connection to send and receive data from the Gemini API.
- Gemini API key for image analysis.

## Setup

### Wi-Fi Configuration
In the code, configure the Wi-Fi credentials:

```cpp
const char* ssid = "<your_wifi_ssid>";
const char* password = "<your_wifi_password>";
```

### Gemini API Key
Set your Gemini API key in the following variable:

```cpp
const char* gemini_api_key = "<your_gemini_api_key>";
```

### Camera Settings
The code includes ESP32-CAM specific camera pin configurations and initialization. Adjust them according to your ESP32-CAM wiring setup if necessary.

## Code Explanation

### Setup
1. The camera is initialized with specific GPIO pins for the ESP32-CAM.
2. The ESP32 connects to the Wi-Fi network.
3. An RF pin is set up for triggering actions based on the image analysis.

### Image Capture and Analysis
1. The `loop()` function captures an image from the ESP32-CAM every 5 seconds (controlled by the interval).
2. The image is encoded into Base64 and sent to the Gemini API for analysis.
3. The API response is checked to determine whether the image contains an elephant. If yes, a GPIO pin (connected to an RF device) is triggered.

### Handling Responses
- If the response contains "Yes", the system assumes an elephant is detected and performs an action (like turning on a relay or LED).
- If "No", the system does nothing or performs an alternate action.

### Error Handling
- If the ESP32 cannot connect to Wi-Fi or capture an image, appropriate error messages will be printed.
- The system also handles HTTP and JSON parsing errors from the Gemini API.

## Dependencies
- ESP32 Arduino Core
- `WiFi.h`: For Wi-Fi functionality.
- `HTTPClient.h`: To make HTTP requests to the Gemini API.
- `ArduinoJson.h`: For JSON serialization and parsing.
- `esp_camera.h`: To interface with the ESP32-CAM module.

## Wiring Diagram
You can refer to the standard ESP32-CAM pinout and ensure the GPIO pins are correctly wired according to the camera module specifications.

## License
This project is licensed under the MIT License.
