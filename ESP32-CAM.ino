#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>

#define LIGHT_PIN 4

#define FORWARD 1
#define BACKWARD 2
#define LEFT 3
#define RIGHT 4
#define STOP 0

#define FWD 1
#define BWD -1

const int PWMFreq = 1000;
const int PWMResolution = 8;
const int PWMSpeedChannel = 2;
const int PWMLightChannel = 3;

//Camera related constants
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

AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsCarInput("/CarInput");
uint32_t cameraClientId = 0;

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta char="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
  <title>FPV Car Control</title>
  <link rel="stylesheet" href="">
    <style>
    @import url('https://fonts.googleapis.com/css2?family=Lexend:wght@100..900&display=swap');

      body {
        font-family: 'Lexend', sans-serif;
        background-color: #f4f4f9;
        margin: 0;
        padding: 0;
      }

      h2 {
        font-family: 'Lexend';
        color: #003366;
        text-align: center;
        margin-bottom: 5px;
        font-size: 20px;
        text-shadow: 0px 0px 10px rgba(0, 51, 102, 0.3);
      }

      .mode-buttons {
        display: flex;
        justify-content: center;
        gap: 10px;
        margin: 5px;
      }

      button.button {
        background-color: #003366;
        color: #fff;
        border: 2px solid #ffcc00;
        border-radius: 10px;
        padding: 10px 20px;
        font-size: 12px;
        cursor: pointer;
        transition: all 0.2s ease-in-out;
        box-shadow: 0 3px 6px rgba(0, 51, 102, 0.2);
      }

      button.button:hover {
        background-color: #ffcc00;
        color: #003366;
        box-shadow: 0 4px 8px rgba(255, 204, 0, 0.4);
      }

      button.button:active {
        transform: translate(5px, 5px);
        box-shadow: none;
        background: #003366;
      }

      td.button {
        background-color: #003366;
        border: 2px solid #ffcc00;
        border-radius: 10px;
        box-shadow: 0 3px 6px rgba(0, 51, 102, 0.3);
        transition: all 0.1s ease-in-out;
        width: 40px;
        height: 40px;
        line-height: 40px;
      }

      td.button:active {
        transform: translate(5px, 5px);
        box-shadow: none;
        background-color: #ffcc00;
      }

      .arrows {
        font-size: 20px;
        color: #ffcc00;
        text-shadow: 1px 1px 8px rgba(255, 204, 0, 0.5);
      }

      .slider {
        -webkit-appearance: none;
        width: 80%;
        height: 10px;
        border-radius: 5px;
        background: linear-gradient(90deg, #003366, #ffcc00);
        outline: none;
        opacity: 0.8;
        transition: opacity 0.2s;
      }

      .slider:hover {
        opacity: 1;
      }

      .slider::-webkit-slider-thumb {
        -webkit-appearance: none;
        width: 15px;
        height: 15px;
        border-radius: 50%;
        background: #003366;
        cursor: pointer;
        box-shadow: 0 0 10px rgba(0, 51, 102, 0.7);
      }

      table {
        background-color: #ffffff;
        padding: 5px;
        margin: 5px;
      }

      img {
        border: 4px solid #ffcc00;
        border-radius: 5px;
        box-shadow: 0 4px 8px rgba(255, 204, 0, 0.3);
      }

      b {
        font-familly: 'Lexend';
        color: #003366;
        font-size: 14px;
      }
    </style>
  
  </head>
  <body class="noselect" align="center" style="background-color:white">
     
    <h2 style="color: #003366;text-align:center;">Multi-mode Smart Remote Control &#128663;</h2>
    <div class="mode-buttons">
      <button class="button" onclick='sendButtonInput("Web","5")'>Web Control</button>
      <button class="button" onclick='sendButtonInput("Autonomous","6")'>Autonomous</button>
      <button class="button" onclick='sendButtonInput("Hand","7")'>Hand Getsure</button>
      <button class="button" onclick='sendButtonInput("Follow","8")'>Follow</button>
    </div>
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <img id="cameraImage" src="" style="width:400px;height:300px"></td>
      </tr> 
      <tr>
        <td></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","1")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8679;</span></td>
        <td></td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","3")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8678;</span></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","0")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8857;</span></td>    
        <td class="button" ontouchstart='sendButtonInput("MoveCar","4")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8680;</span></td>
      </tr>
      <tr>
        <td></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","2")' ontouchend='sendButtonInput("MoveCar","0")'><span class="arrows" >&#8681;</span></td>
        <td></td>
      </tr>
      <tr/><tr/>
      <tr>
        <td style="text-align:left"><b>Speed:</b></td>
        <td colspan=2>
         <div class="slidecontainer">
            <input type="range" min="0" max="255" value="150" class="slider" id="Speed" oninput='sendButtonInput("Speed",value)'>
          </div>
        </td>
      </tr>        
      <tr>
        <td style="text-align:left"><b>Light:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="255" value="1" class="slider" id="Light" oninput='sendButtonInput("Light",value)'>
          </div>
        </td>   
      </tr>
    </table>
  
    <script>
      var webSocketCameraUrl = "ws:\/\/" + window.location.hostname + "/Camera";
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";      
      var websocketCamera;
      var websocketCarInput;
      
      function initCameraWebSocket() 
      {
        websocketCamera = new WebSocket(webSocketCameraUrl);
        websocketCamera.binaryType = 'blob';
        websocketCamera.onopen    = function(event){};
        websocketCamera.onclose   = function(event){setTimeout(initCameraWebSocket, 2000);};
        websocketCamera.onmessage = function(event)
        {
          var imageId = document.getElementById("cameraImage");
          imageId.src = URL.createObjectURL(event.data);
        };
      }
      
      function initCarInputWebSocket() 
      {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen    = function(event)
        {
          var speedButton = document.getElementById("Speed");
          sendButtonInput("Speed", speedButton.value);
          var lightButton = document.getElementById("Light");
          sendButtonInput("Light", lightButton.value);
        };
        websocketCarInput.onclose   = function(event){setTimeout(initCarInputWebSocket, 2000);};
        websocketCarInput.onmessage = function(event){};        
      }
      
      function initWebSocket() 
      {
        initCameraWebSocket ();
        initCarInputWebSocket();
      }

      function sendButtonInput(key, value) 
      {
        var data = key + "," + value;
        websocketCarInput.send(data);
      }
    
      window.onload = initWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });      
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";

void handleRoot(AsyncWebServerRequest *request) 
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request) 
{
    request->send(404, "text/plain", "File Not Found!");
}

void onCarInputWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      Serial.print("MoveCar,0\n");
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;  
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) 
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        Serial.printf("%s: %s\n", key.c_str(), value.c_str()); 
        int valueInt = atoi(value.c_str());     
        if (key == "MoveCar")
        {
          String command = "MoveCar," + String(valueInt) + "\n";
          Serial.print(command);
          // moveCar(valueInt);        
        }
        else if (key == "Speed")
        {
          String command = "Speed," + String(valueInt) + "\n";
          Serial.print(command);
          // ledcWrite(PWMSpeedChannel, valueInt);
        }
        else if (key == "Light")
        {
          ledcWrite(PWMLightChannel, valueInt);         
        }
        else if (key == "Web" || key == "Autonomous" || key == "Hand" || key == "Follow")
        {
          String command = String(key.c_str()) + "\n";
          Serial.print(command);
        }
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void onCameraWebSocketEvent(AsyncWebSocket *server, 
                      AsyncWebSocketClient *client, 
                      AwsEventType type,
                      void *arg, 
                      uint8_t *data, 
                      size_t len) 
{                      
  switch (type) 
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      cameraClientId = client->id();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      cameraClientId = 0;
      break;
    case WS_EVT_DATA:
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void setupCamera()
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
  
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }  

  if (psramFound())
  {
    heap_caps_malloc_extmem_enable(20000);   
  }  

  sensor_t *s = esp_camera_sensor_get();
  if (s != nullptr) {
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
    s->set_brightness(s, 2);
  }
}

void sendCameraPicture()
{
  if (cameraClientId == 0)
  {
    return;
  }
  unsigned long  startTime1 = millis();
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) 
  {
      Serial.println("Frame buffer could not be acquired!");
      return;
  }

  unsigned long  startTime2 = millis();
  wsCamera.binary(cameraClientId, fb->buf, fb->len);
  esp_camera_fb_return(fb);
    
  // Wait for message to be delivered
  while (true)
  {
    AsyncWebSocketClient * clientPointer = wsCamera.client(cameraClientId);
    if (!clientPointer || !(clientPointer->queueIsFull()))
    {
      break;
    }
    delay(1);
  }
  
  unsigned long  startTime3 = millis();
}

void setUpPinModes()
{
  ledcSetup(PWMLightChannel, PWMFreq, PWMResolution);
  pinMode(LIGHT_PIN, OUTPUT);    
  ledcAttachPin(LIGHT_PIN, PWMLightChannel);
}


void setup(void) 
{
  setUpPinModes();
  Serial.begin(115200);

  // Cấu hình WiFi Access Point
  const IPAddress local_IP(192, 168, 4, 1);     // IP Address of ESP32-CAM
  const IPAddress gateway(192, 168, 4, 1);      // Gateway (same IP ESP32-CAM)
  const IPAddress subnet(255, 255, 255, 0);     // Subnet mask
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Khởi động Access Point
  WiFi.softAP("ESP32-CAM-ICEK19", "20070828"); // SSID and Password of WiFi
  Serial.println("Access Point started!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP()); // Print IP Address of Access Point

  // Cấu hình WebServer
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
      
  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);

  server.begin();
  Serial.println("HTTP server started!");

  setupCamera();
}

void loop() 
{
  wsCamera.cleanupClients(); 
  wsCarInput.cleanupClients(); 
  sendCameraPicture();
}