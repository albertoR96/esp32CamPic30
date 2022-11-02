#include <SPI.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"

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

char ssid[] = "HUAWEIxdx4";    //  your network SSID (name)
char pass[] = "iLRaGI4YHx";   // your network password

int status = WL_IDLE_STATUS;
String server = "192.168.1.80";

// Initialize the client library
WiFiClient client;

bool wfiConnect(int x = 0)
{
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  int timeoutS = 0;
  while (WiFi.status() != WL_CONNECTED && timeoutS < 10) {
    Serial.print(".");
    delay(500);
    timeoutS++;
  }
  if (timeoutS >= 20 && x < 3) {
    delay(2000);
    return wfiConnect(x + 1);
  } else if (WiFi.status() != WL_CONNECTED && x >= 3) {
    return false;
  }
  return WiFi.status() == WL_CONNECTED;
}

void loadConfigCamera()
{
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

  // init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
}

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(9600);
  Serial.println("Attempting to connect to WPA network...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  if (wfiConnect()) {
    Serial.println();
    Serial.println("Connected!!!");
    Serial.print("ESP32-CAM IP Address: ");
    Serial.println(WiFi.localIP());

    loadConfigCamera();
  } else {
    Serial.println();
    Serial.println("Could not get connection :(");
  }
  /*while (WiFi.status() != WL_CONNECTED || ) {
    Serial.print(".");
    // Connect to WPA/WPA2 network:
    // wait 10 seconds for connection:
    delay(800);
  }
  if (status != WL_CONNECTED) {
    Serial.println("Couldn't get a wifi connection");
    // don't do anything else:
    while(true);
  } else {
    Serial.println("Connected to wifi");
    Serial.println("\nStarting connection...");
    // if you get a connection, report back via serial:
    if (client.connect(server.c_str(), 80)) {
      Serial.println("connected");
      // Make a HTTP request:
      client.println("GET /index.php HTTP/1.0");
      client.println();
    }
  }*/
}

bool connectToServer(int x = 0)
{
  Serial.println("Connecting to " + server);
  int timeoutS = 0;
  while (!client.connect(server.c_str(), 80) && timeoutS < 10) {
    Serial.print(".");
    timeoutS++;
    delay(500);
  }
  Serial.println();
  if (timeoutS >= 20 && x < 3) {
    delay(2000);
    return connectToServer(x + 1);
  } else if (timeoutS >= 20 && x >= 3) {
    return false;
  }
  return true;
}

void doRequest(String method, String path)
{
  Serial.println("Doing request...");
  Serial.println(method + path + " HTTP/1.1");
  client.println(method + path + " HTTP/1.1");
  client.println("Host: " + server);
  client.println("Connection: close");
  client.println();

  delay(100);
  int f = 0;
  while (client.available()) {
    if (f == 0) {
      f++;
      Serial.println("Reading from client...");
    }
    char c = client.read();
    Serial.print(c);
  }
}

void sendHTTPRequest(int x = 0)
{
  if (connectToServer()) {
    Serial.println("Connected to server!!!");
    delay(500);
    doRequest("GET", "/index.html");
  } else {
    Serial.println("Could not connect to server :(");
  }
}

void takePictureAndSend()
{
  if (connectToServer()) {
    Serial.println("Connected to server!!!");
    delay(500);


    Serial.println("Taking picture!!!");
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      Serial.println("Restarting...");
      delay(5000);
      ESP.restart();
    } else {
      uint32_t imageLen = fb->len;
      uint8_t *fbBuf = fb->buf;
      size_t fbLen = fb->len;

      String head = "--ESP32CAMFILE\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\r\nContent-Type: image/jpeg\r\n";
      String tail = "\r\n--ESP32CAMFILE--\r\n";

      uint32_t extraLen = head.length() + tail.length();
      uint32_t totalLen = imageLen + extraLen;

      bool readResponse = client.connected();
      if (readResponse) {
        client.println("POST /save_pic.php HTTP/1.1");
        client.println("Host: " + server);
        client.println("Content-Length: " + String(totalLen));
        client.println("Content-Type: multipart/form-data; boundary=ESP32CAMFILE");
        client.println("Connection: keep-alive");
        client.println();
        client.print(head);

        Serial.println();
        Serial.println("Image size: ");
        Serial.print(fbLen);
        Serial.println();

        for (size_t n = 0; n < fbLen; n = n + 1024) {
          if (n + 1024 < fbLen) {
            client.write(fbBuf, 1024);
            fbBuf += 1024;
          } else if (fbLen % 1024 > 0) {
            size_t remainder = fbLen % 1024;
            client.write(fbBuf, remainder);
          }
        }
        client.print(tail);
      } else {
        Serial.println();
        Serial.println("disconnecting.");
        client.stop();
      }

      esp_camera_fb_return(fb);

      //read the server's response
      if (readResponse) {
        int timoutTimer = 10000;
        long startTimer = millis();
        String bodyResponse = "";
        while ((startTimer + timoutTimer) > millis()) {
          Serial.print(".");
          delay(100);
          bool printMessage = true;
          while (client.available()) {
            if (printMessage) {
              Serial.println("Reading from client...");
              printMessage = false;
            }
            char c = client.read();
            bodyResponse += String(c);
            Serial.print(c);
          }
          if (bodyResponse.length() > 0 ) { break; }
        }
        Serial.println();
      }
    }
  } else {
    Serial.println("Could not connect to server :(");
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    delay(2000);
    /*sendHTTPRequest();
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting.");
      client.stop();
    }*/

    takePictureAndSend();
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting.");
      client.stop();
      Serial.println("Restarting...");
      delay(5000);
      ESP.restart();
    }

    delay(27000);
  } else {
    Serial.println("Restarting...");
    delay(5000);
    ESP.restart();
  }
}