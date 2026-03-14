#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_Si7021.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define ONE_WIRE_BUS 4
#define LDR_PIN 35
#define IR_SENSOR 17

// ---------------- BLE SETUP ----------------
#define DEVICE_NAME "ESP32_Imax_Monitor"
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-5678-90ab-cdef-1234567890ab"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Phone connected to BLE");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Phone disconnected from BLE");
  }
};

// ---------------- SENSOR OBJECTS ----------------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature wireSensor(&oneWire);
Adafruit_Si7021 ambientSensor = Adafruit_Si7021();

// ---------------- WIND VARIABLES ----------------
volatile int spoonCount = 0;
unsigned long lastTime = 0;

float radius_m = 0.0762;
float circumference = 2 * PI * radius_m;

// ---------------- CONSTANTS ----------------
float Diameter = 0.0159;
float epsilon = 0.9;
float sigma = 5.67e-8;
float alpha = 0.9;
float R20 = 13.4;
float alphaTCR = 0.0004;
float Airdensity = 1.225;

// ---------------- HELPER FUNCTION ----------------
float adcToLux(int adcValue) {
  return (adcValue / 4095.0) * 1000.0;
}

void setup() {
  Serial.begin(115200);

  pinMode(LDR_PIN, INPUT);
  pinMode(IR_SENSOR, INPUT);

  wireSensor.begin();

  Wire.begin(18, 19);

  if (!ambientSensor.begin()) {
    Serial.println("Si7021 not found");
    while (1);
  }

  // -------- BLE START --------
  BLEDevice::init(DEVICE_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("ESP32 ready");

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("BLE is ready and advertising");

  lastTime = millis();
}

void loop() {
  // -------- WIND SPEED --------
  static int lastState = HIGH;
  int currentState = digitalRead(IR_SENSOR);

  if (lastState == HIGH && currentState == LOW) {
    spoonCount++;
  }

  lastState = currentState;

  float Velo = 0.0;

  if (millis() - lastTime >= 1000) {
    float revolutions = spoonCount / 3.0;
    Velo = revolutions * circumference;

    spoonCount = 0;
    lastTime = millis();

    // -------- WIRE TEMP --------
    wireSensor.requestTemperatures();
    float Tc = wireSensor.getTempCByIndex(0);

    // -------- AMBIENT TEMP --------
    float Ta = ambientSensor.readTemperature();

    // -------- LUX --------
    int adcValue = analogRead(LDR_PIN);
    float PRlux = adcToLux(adcValue);

    // -------- HEAT TRANSFER CALCS --------
    float Tfilm = (Tc + Ta) / 2.0;

    float muF = (1.456e-6 * pow(Tfilm + 273.15, 1.5)) / (Tfilm + 383.55);

    float Re = (Diameter * Airdensity * Velo) / muF;

    float Nu = 0.65 * pow(Re, 0.2) + 0.23 * pow(Re, 0.61);

    float Kf = 2.424e-2 + 7.477e-5 * Tfilm - 4.407e-9 * pow(Tfilm, 2);

    float h = (Nu * Kf) / Diameter;

    float Qc = h * pow(Diameter / 2.0, 2) * (Tc - Ta);

    float Qr = epsilon * sigma * (pow(Tc + 273.15, 4) - pow(Ta + 273.15, 4));

    float Qs = alpha * PRlux * 0.0079 * Diameter;

    float Rtc = R20 * (1 + alphaTCR * (Tc - 20));

    float insideSqrt = (Qc + Qr - Qs) / Rtc;

    float Imax;
    if (insideSqrt >= 0) {
      Imax = sqrt(insideSqrt);
    } else {
      Imax = 0;  // prevents NaN if expression goes negative
    }

    // -------- DECISION MESSAGE --------
    String phoneMessage;
    if (Imax < 2.5) {
      phoneMessage = "all good";
    } else {
      phoneMessage = "reduce current";
    }

    // -------- SERIAL PRINT --------
    Serial.println("------ SENSOR DATA ------");

    Serial.print("Wire Temp Tc (C): ");
    Serial.println(Tc);

    Serial.print("Ambient Temp Ta (C): ");
    Serial.println(Ta);

    Serial.print("Wind Speed (m/s): ");
    Serial.println(Velo);

    Serial.print("Lux: ");
    Serial.println(PRlux);

    Serial.print("Imax (Amps): ");
    Serial.println(Imax);

    Serial.print("Phone Message: ");
    Serial.println(phoneMessage);

    Serial.println("-------------------------");

    // -------- SEND TO PHONE OVER BLE --------
    if (deviceConnected) {
      pCharacteristic->setValue(phoneMessage.c_str());
      pCharacteristic->notify();
    }
  }

  // -------- HANDLE BLE RECONNECT --------
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    Serial.println("Restarted BLE advertising");
    oldDeviceConnected = deviceConnected;
  }

  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}
