#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

const int rf = 4;

// WiFi credentials
const char* ssid = "";
const char* password = "";

// Gemini API settings
const char* gemini_api_key = "";
String gemini_endpoint = "https://generativelanguage.googleapis.com/v1/models/gemini-1.5-flash:generateContent?key=" + String(gemini_api_key);


unsigned long previousMillis = 0;
const unsigned long interval = 5000; 
bool isProcessing = false;

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.println("\nESP32-CAM Human Detection");

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera initialized successfully");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  pinMode(rf, OUTPUT);
}

String base64Encode(uint8_t* data, size_t length) {
  static const char* encoding_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t output_length = 4 * ((length + 2) / 3);
  String encoded;
  encoded.reserve(output_length);

  for (size_t i = 0; i < length; i += 3) {
    uint32_t octet_a = i < length ? data[i] : 0;
    uint32_t octet_b = i + 1 < length ? data[i + 1] : 0;
    uint32_t octet_c = i + 2 < length ? data[i + 2] : 0;

    uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

    encoded += encoding_table[(triple >> 18) & 0x3F];
    encoded += encoding_table[(triple >> 12) & 0x3F];
    encoded += encoding_table[(triple >> 6) & 0x3F];
    encoded += encoding_table[triple & 0x3F];
  }

  if (length % 3 == 1) {
    encoded[encoded.length() - 1] = '=';
    encoded[encoded.length() - 2] = '=';
  } else if (length % 3 == 2) {
    encoded[encoded.length() - 1] = '=';
  }

  return encoded;
}

String analyzeImage(camera_fb_t* fb) {
  if (WiFi.status() != WL_CONNECTED) {
    return "WiFi not connected";
  }

  HTTPClient http;
  http.begin(gemini_endpoint);
  http.addHeader("Content-Type", "application/json");

  DynamicJsonDocument doc(32768);
  String base64Image = base64Encode(fb->buf, fb->len);

  JsonArray contents = doc.createNestedArray("contents");
  JsonObject content = contents.createNestedObject();
  JsonArray parts = content.createNestedArray("parts");

  JsonObject textPart = parts.createNestedObject();
  textPart["text"] = "Does this image contain human or humans? Please answer with 'Yes' or 'No'.";

  JsonObject imagePart = parts.createNestedObject();
  JsonObject inlineData = imagePart.createNestedObject("inline_data");
  inlineData["mime_type"] = "image/jpeg";
  inlineData["data"] = base64Image;

  String jsonString;
  serializeJson(doc, jsonString);

  Serial.println("Sending request to Gemini API...");
  int httpResponseCode = http.POST(jsonString);
  String response = "No response";

  if (httpResponseCode > 0) {
    response = http.getString();
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    Serial.println("Raw response: " + response);

    DynamicJsonDocument responseDoc(16384);
    DeserializationError error = deserializeJson(responseDoc, response);

    if (!error) {
      if (responseDoc.containsKey("candidates") && 
          responseDoc["candidates"][0].containsKey("content") && 
          responseDoc["candidates"][0]["content"].containsKey("parts") &&
          responseDoc["candidates"][0]["content"]["parts"][0].containsKey("text")) {
        response = responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
      } else {
        response = "Error: " + response;
      }
    } else {
      response = "Error parsing JSON: " + String(error.c_str()) + "\nRaw: " + response;
    }
  } else {
    response = "HTTP Error: " + String(httpResponseCode) + "\n" + http.getString();
  }

  http.end();
  return response;
}

void loop() {
  unsigned long currentMillis = millis();

  if (!isProcessing && (currentMillis - previousMillis >= interval)) {
    isProcessing = true;
    Serial.println("\n--- Starting New Image Analysis ---");

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      isProcessing = false;
      previousMillis = currentMillis;
      return;
    }

    Serial.println("Image captured, analyzing...");
    String response = analyzeImage(fb);

    if (response.indexOf("Yes") != -1) {
      Serial.println("Human Detected");
      digitalWrite(rf, HIGH);
    } else {
      Serial.println("No Human Detected");
      digitalWrite(rf, LOW);
    }

    esp_camera_fb_return(fb);
    Serial.println("Analysis complete, waiting for next interval...");
    
    previousMillis = currentMillis;
    isProcessing = false;
  }
}
