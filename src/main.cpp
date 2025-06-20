#include <Arduino.h>
#include <IO7F32.h>
#include <TFT_eSPI.h>

char* ssid_pfix = (char*)"TempSetter";
const int pulseA = 44;
const int pulseB = 43;
const int btn = 18;

TFT_eSPI tft = TFT_eSPI();
volatile int encoderValue = 0;
int prevValue = -1;
char tempStr[10];
bool btnPressed = false;

void IRAM_ATTR handleEncoder() {
  static int lastEncoded = 0;
  int MSB = digitalRead(pulseA);
  int LSB = digitalRead(pulseB);
  int encoded = (MSB << 1) | LSB;
  int sum  = (lastEncoded << 2) | encoded;

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue--;
  encoderValue = constrain(encoderValue, 0, 100);  // 0~100도 제한
  lastEncoded = encoded;
}

void IRAM_ATTR handleButton() {
  btnPressed = true;
}

void publishTargetTemp() {
  StaticJsonDocument<256> root;
  JsonObject data = root.createNestedObject("d");
  data["target_temp"] = encoderValue;
  serializeJson(root, msgBuffer);
  client.publish(evtTopic, msgBuffer);
  Serial.printf("[MQTT] 설정온도: %dC 전송\n", encoderValue);
}

void setup() {
  Serial.begin(115200);
  pinMode(pulseA, INPUT_PULLUP);
  pinMode(pulseB, INPUT_PULLUP);
  pinMode(btn, INPUT_PULLUP);
  attachInterrupt(pulseA, handleEncoder, CHANGE);
  attachInterrupt(pulseB, handleEncoder, CHANGE);
  attachInterrupt(btn, handleButton, FALLING);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.drawString("Set Temp", 60, 20);

  initDevice();
  WiFi.mode(WIFI_STA);
  WiFi.begin((const char*)cfg["ssid"], (const char*)cfg["w_pw"]);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  set_iot_server();
  iot_connect();
}

void loop() {
  client.loop();
  if (encoderValue != prevValue) {
    prevValue = encoderValue;
    tft.fillRect(40, 60, 160, 40, TFT_BLACK);  // 숫자 영역만 지우기
    sprintf(tempStr, "%dC", encoderValue);
    tft.drawString(tempStr, 90, 60);
  }

  if (btnPressed) {
    publishTargetTemp();
    btnPressed = false;
    tft.fillRect(60, 100, 160, 30, TFT_BLACK);
    tft.drawString("Confirmed!", 70, 100, 2);
  }
}
