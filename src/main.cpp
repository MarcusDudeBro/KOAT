#include <Arduino.h>
#include <WiFi.h>
#include "Wire.h"
#include "SparkFunLSM6DSO.h"
#include "../include/imu.h"
#include <ArduinoBLE.h>
#include <string>
#include <AsyncHTTPRequest_Generic.h>


#define SERVICE_UUID        "307ba605-b5bf-437a-8f70-87fff5eb5f48"
#define CHARACTERISTIC_UUID "4a5d39f9-256d-4fa0-9afb-97e61afb86fa"

#define SAMP_RATE 1
#define BLE_SCAN_RATE 500
#define OUT_RANGE_COUNT_THRESHOLD 5
#define SIGNAL_STRENGTH_THRESHOLD -55
#define BUZZ_PIN 32
#define BUZZ_TIME 250
#define BUZZ_FREQ 0.5
#define WIFI_SEARCH_DELAY 1000

LSM6DSO myIMU;
IMU_Vals p;
BLEService service(SERVICE_UUID);
BLEUnsignedCharCharacteristic characteristic(CHARACTERISTIC_UUID, BLERead | BLENotify);
BLEDevice central;

// wifi vars
char ssid[] = "Tell my wifi love her";
char pass[] = "hmmmmmmm";
const char kHostname[] = "54.215.244.83:5000";
const int kNetworkDelay = 1000;
AsyncHTTPRequest request;

bool bleConnected;
bool blePrevConnected;
float bleScanTimer;

// imu pose vars
float prevCalAccY;
float prevCalAccX;
float prevVelY;
float prevVelX;
float prevT;
float rssiTimer;
float outRangeCount;
float driftYCount;
float driftXCount;

// buzzer vars
float buzzTimer;
bool buzzerOn;

void buzzerActivate() {
  if(millis() > buzzTimer) {
    buzzTimer = millis() + BUZZ_TIME;
    buzzerOn = !buzzerOn;
  }
  if(buzzerOn) {
    digitalWrite(BUZZ_PIN, HIGH);
  } else {
    digitalWrite(BUZZ_PIN, LOW);
  }
}

void updateVel(float dt, float accY, float accX) {
  //calculate velocity in y direction
  if(prevCalAccY < accY) {
    p.vy += dt*(accY-prevCalAccY)/2.0 + prevCalAccY*dt;
  } else if(prevCalAccY > accY) {
    p.vy += dt*(prevCalAccY-accY)/2.0 + accY*dt;
  } else {
    p.vy += prevCalAccY*dt;
  }
  // remove drift
  if(abs(p.vy - prevVelY) < DRIFT_THRESH) {
    driftYCount++;
  } else {
    driftYCount = 0;
  }
  if(driftYCount > MAX_DRIFT_COUNT) {
    p.vy = 0;
  }
  
  //calculate velocity in x direction
  if(prevCalAccX < accX) {
    p.vx += dt*(accX-prevCalAccX)/2.0 + prevCalAccX*dt;
  } else if(prevCalAccX > accX) {
    p.vx += dt*(prevCalAccX-accX)/2.0 + accX*dt;
  } else {
    p.vx += prevCalAccX*dt;
  }
  // remove drift
  if(abs(p.vx - prevVelX) < DRIFT_THRESH) {
    driftXCount++;
  } else {
    driftXCount = 0;
  }
  if(driftXCount > MAX_DRIFT_COUNT) {
    p.vx = 0;
  }
}

void updatePos(float dt, float velY, float velX) {
  //calculate position in y direction
  if(prevVelY < velY) {
    p.y += dt*(velY-prevVelY)/2.0 + prevVelY*dt;
  } else if(prevVelY > velY) {
    p.y += dt*(prevVelY-velY)/2.0 + velY*dt;
  } else {
    p.y += prevVelY*dt;
  }

  //calculate position in x direction
  if(prevVelX < velX) {
    p.x += dt*(velX-prevVelX)/2.0 + prevVelX*dt;
  } else if(prevVelX > velX) {
    p.x += dt*(prevVelX-velX)/2.0 + velX*dt;
  } else {
    p.x += prevVelX*dt;
  }
}

void updatePose(float currTime) {
  float dt = currTime/1000.0 - prevT;
  float calAccY = (myIMU.readFloatAccelY() - p.biasY) * G;
  float calAccX = (myIMU.readFloatAccelX() - p.biasX) * G;
  if(calAccY < 0.07 && calAccY > -0.07) {
    calAccY = 0;
  }
  if(calAccX < 0.07 && calAccX > -0.07) {
    calAccX = 0;
  }
  updateVel(dt, calAccY, calAccX);
  updatePos(dt, p.vy, p.vx);
  prevVelY = p.vy;
  prevCalAccY = calAccY;
  prevVelX = p.vx;
  prevCalAccX = calAccX;
  Serial.printf("pos x: %.2f | pos y: %.2f\n", p.x, p.y);
  //Serial.printf("acc: %.2f m/s^2 | vel: %.3f m/s | pos: %.2f cm\n", calAccY, p.vy, p.y*100);
}

void sendRequest(std::string kPath) {
  delay(200);
  static bool requestOpenResult;
  if (request.readyState() == readyStateUnsent || request.readyState() == readyStateDone) {
    requestOpenResult = request.open("PUT", (kHostname + kPath).c_str());
    if (requestOpenResult) {
      // Only send() if open() returns true, or crash
      request.send();
    } else {
      Serial.println("Can't send, bad request");
    }
  } else {
    Serial.println("Can't send request");
  }
}

void sendBLEConnectionStatus(std::string connectionStatus) {
  std::string kPath = "/connection?connected=" + connectionStatus;
  sendRequest(kPath);
}

void sendPosition(float x, float y) {
  std::string kPath = "/positions?x=" + std::to_string(x) + "&y=" + std::to_string(y);
  sendRequest(kPath);
}

void setup() {
  Serial.begin(9600);
  Serial.println();
  pinMode(BUZZ_PIN, OUTPUT);

  // setup BLE
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");
    while (1);
  }
  BLE.setLocalName("KOAT Device");
  BLE.setAdvertisedService(service);
  service.addCharacteristic(characteristic);
  BLE.addService(service);
  BLE.advertise();
  central = BLE.central();
  Serial.println("Bluetooth device active, waiting for connections...");
  rssiTimer = millis() + BLE_SCAN_RATE;
  outRangeCount = 0;
  bleConnected = false;
  blePrevConnected = false;

  // setup wifi
  delay(kNetworkDelay);
  Serial.print("\n\nConnecting to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
  sendBLEConnectionStatus("false");

  // init IMU
  Serial.println("Initializing IMU...");
  Wire.begin();
  delay(10);
  if(myIMU.begin()) {
    Serial.println("IMU ready.");
  } else {
    Serial.println("Could not connect to IMU.");
    Serial.println("Freezing");
  }
  if(myIMU.initialize(BASIC_SETTINGS))
    Serial.println("IMU settings loaded");

  // init pose and biases
  // position
  p.x = 0;
  p.y = 0;
  p.vy = 0;
  p.vx = 0;
  // drift velocities
  p.driftVY = 0;
  p.driftVX = 0;
  // biases
  p.biasX = 0;
  p.biasY = 0;
  // degrees
  p.degX  = 0;
  p.degY = 0;
  prevCalAccY = 0;
  prevCalAccX = 0;
  prevVelY = 0;
  prevVelX = 0;
  driftYCount = 0;
  driftXCount = 0;
  
  // calibrate IMU
  Serial.println("The IMU will begin calibration, level the board and hold it still...");
  sleep(PRE_CAL_T);
  Serial.println("Calibrating IMU...");
  int count = 0;
  int calibrationTimer = millis() + CAL_T;
  while(millis() < calibrationTimer) {
    p.biasX += myIMU.readFloatAccelX();
    p.biasY += myIMU.readFloatAccelY();
    count++;
  }
  p.biasX /= count;
  p.biasY /= count;
  Serial.println("Calibration finished.");
  Serial.printf("acceleration x bias: %.2f\n", p.biasX);
  Serial.printf("acceleration y bias: %.2f\n", p.biasY);
  prevT = millis()/1000.0;

  // init buzzer
  pinMode(BUZZ_PIN, OUTPUT);
  buzzTimer = millis() + BUZZ_TIME;
  buzzerOn = false;
}

void loop() {
  // first check if board is connected
  if(central) {
    if(central.connected()) {
      bleConnected = true;
    } else {
      Serial.println("central disconnected: connected false");
      bleConnected = false;
    }
  } else if (millis() > bleScanTimer){
    Serial.println("central not found: connected false");
    bleConnected = false;
    central = BLE.central();
    bleScanTimer = millis() + BLE_SCAN_RATE;
    digitalWrite(BUZZ_PIN, LOW);
  }

  float currTime = millis();

  // get RSSI value & count weak signal strength instances
  if(bleConnected && currTime > rssiTimer) {
    int rssi = BLE.rssi();
    Serial.printf("RSSI val: %d\n", rssi);
    // if phone is out of range, count
    if(rssi < SIGNAL_STRENGTH_THRESHOLD) {
      outRangeCount++;
    } else {
      outRangeCount = 0;
    }
    rssiTimer = currTime + BLE_SCAN_RATE;
  }

  // if phone was out of range consistently, buzz buzzer
  if(outRangeCount >= OUT_RANGE_COUNT_THRESHOLD) {
    //Serial.println("out of range: connected false");
    buzzerActivate();
    bleConnected = false;
    updatePose(currTime);
    sendPosition(p.x, p.y);
  } else {
    digitalWrite(BUZZ_PIN, LOW);
  }

  if(blePrevConnected && !bleConnected) {
    // async?
    sendBLEConnectionStatus("false");
  } else if(bleConnected && !blePrevConnected) {
    sendBLEConnectionStatus("true");
  }
  
  blePrevConnected = bleConnected;
  prevT = currTime/1000.0;
}