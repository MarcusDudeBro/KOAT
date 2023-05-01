#include <Arduino.h>
#include <TFT_eSPI.h>
#include "Wire.h"
#include "SparkFunLSM6DSO.h"
#include "../include/imu.h"
#include <ArduinoBLE.h>

#define SERVICE_UUID        "307ba605-b5bf-437a-8f70-87fff5eb5f48"
#define CHARACTERISTIC_UUID "4a5d39f9-256d-4fa0-9afb-97e61afb86fa"

#define SAMP_RATE 1
#define BLE_SCAN_RATE 500
#define OUT_RANGE_COUNT_THRESHOLD 3
#define SIGNAL_STRENGTH_THRESHOLD -65
#define BUZZ_PIN 32
#define BUZZ_TIME 250
#define BUZZ_FREQ 0.5

LSM6DSO myIMU;
TFT_eSPI tft = TFT_eSPI();
IMU_Vals p;
BLEService batteryService(SERVICE_UUID);
BLEUnsignedCharCharacteristic batteryLevelChar(CHARACTERISTIC_UUID, BLERead | BLENotify);
BLEDevice central;

float prevCalAccY;
float prevVelY;
float prevT;
float rssiTimer;
float outRangeCount;

// buzzer vars
float buzzTimer;
bool buzzerOn;
float freqBuzz;
bool buzz;

void buzzerActivate() {
  if(millis() > buzzTimer) {
    buzzTimer = millis() + BUZZ_TIME;
    buzzerOn = !buzzerOn;
  }
  if(buzzerOn) {
    if(millis() > freqBuzz) {
      freqBuzz = millis() + BUZZ_FREQ;
      buzz = !buzz;
    }
    if(buzz) {
      digitalWrite(BUZZ_PIN, HIGH);
    } else {
      digitalWrite(BUZZ_PIN, LOW);
    }
  } else {
    digitalWrite(BUZZ_PIN, LOW);
  }
}

void updateVel(float dt, float accY) {
  //calculate velocity in y direction
  if(prevCalAccY < accY) {
    p.vy += dt*(accY-prevCalAccY)/2.0 + prevCalAccY*dt;
  } else if(prevCalAccY > accY) {
    p.vy += dt*(prevCalAccY-accY)/2.0 + accY*dt;
  } else {
    p.vy += prevCalAccY*dt;
  }
  // if velocity is small enough, just set it to 0
  if(p.vy < 0.02 && p.vy > -0.02) {
    p.vy = 0;
  }
}

void updatePos(float dt, float velY) {
  //calculate position in y direction
  if(prevVelY < velY) {
    p.y += dt*(velY-prevVelY)/2.0 + prevVelY*dt;
  } else if(prevVelY > velY) {
    p.y += dt*(prevVelY-velY)/2.0 + velY*dt;
  } else {
    p.y += prevVelY*dt;
  }
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
  BLE.setAdvertisedService(batteryService);
  batteryService.addCharacteristic(batteryLevelChar);
  BLE.addService(batteryService);
  BLE.advertise();
  central = BLE.central();
  Serial.println("Bluetooth device active, waiting for connections...");
  rssiTimer = millis() + BLE_SCAN_RATE;
  outRangeCount = 0;

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

  // init onboard screen
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(0);

  // init pose and biases
  p.x = 0;
  p.y = 0;
  p.z = 0;
  p.vy = 0;
  p.biasX = 0;
  p.biasY = 0;
  p.biasZ = 0;
  p.degX  = 0;
  p.degY = 0;
  p.degZ = 0;
  prevCalAccY = 0;
  prevVelY = 0;
  
  // calibrate IMU
  Serial.println("The IMU will begin calibration, level the board and hold it still...");
  sleep(PRE_CAL_T);
  Serial.println("Calibrating IMU...");
  int count = 0;
  while(millis() < CAL_T) {
    p.biasX += myIMU.readFloatAccelX();
    p.biasY += myIMU.readFloatAccelY();
    p.biasZ += myIMU.readFloatAccelZ();
    count++;
  }
  p.biasX /= count;
  p.biasY /= count;
  p.biasZ /= count;
  Serial.println("Calibration finished.");
  Serial.printf("acceleration x bias: %.2f\n", p.biasX);
  Serial.printf("acceleration y bias: %.2f\n", p.biasY);
  Serial.printf("acceleration z bias: %.2f\n", p.biasZ);
  prevT = millis()/1000.0;

  // init buzzer
  pinMode(BUZZ_PIN, OUTPUT);
  buzzTimer = millis() + BUZZ_TIME;
  buzzerOn = false;
  freqBuzz = millis();
  buzz = false;
  digitalWrite(BUZZ_PIN, LOW);
}

void loop() {
  // wait until ble connection established
  while(!central.connected()){
    digitalWrite(BUZZ_PIN, LOW);
    central = BLE.central();
    Serial.println("Waiting for BLE connection...");
    delay(BLE_SCAN_RATE);
  }

  float time = millis();
  // delta time of current measurement and previous measurement
  float dt = time/1000.0 - prevT;
  // current calibrated acceleration x
  float calAccY = (myIMU.readFloatAccelY() - p.biasY) * G;
  // if the measured values are small enough just set acc to 0
  if(calAccY < 0.07 && calAccY > -0.07) {
    calAccY = 0;
  }

  // get RSSI value & count weak signal strength instances
  if(central) {
    if(time > rssiTimer && central.connected()){
      int rssi = BLE.rssi();
      Serial.printf("RSSI val: %d\n", rssi);
      // if phone is out of range, count
      if(rssi < SIGNAL_STRENGTH_THRESHOLD) {
        outRangeCount++;
      } else {
        outRangeCount = 0;
      }
      rssiTimer = time + BLE_SCAN_RATE;
    }
  }
  // if phone was out of range consistently, buzz buzzer
  if(outRangeCount >= OUT_RANGE_COUNT_THRESHOLD) {
    buzzerActivate();
  } else {
    digitalWrite(BUZZ_PIN, LOW);
  }

  // update pose values
  updateVel(dt, calAccY);
  updatePos(dt, p.vy);
  
  prevVelY = p.vy;
  prevCalAccY = calAccY;
  prevT = time/1000.0;
  // Serial.printf("acc: %.2f m/s^2 | vel: %.3f m/s | pos: %.2f cm\n", calAccY, p.vy, p.y*100);
  //byte font = 6;
  //tft.drawFloat(p.vy, 0, 60, font);
}