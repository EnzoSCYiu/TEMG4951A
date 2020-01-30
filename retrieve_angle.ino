
//Written by Ahmet Burkay KIRNIK
//TR_CapaFenLisesi
//Measure Angle with a MPU-6050(GY-521)

#include<Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "f78bd81c-1a66-4c50-8f24-632988e64dad"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

int minVal=265;
int maxVal=402;

double x;
double y;
double z;
 
int lowbat = 0;
int low_count = 0;
float voltage = 0.0;
int blue = 0;
int incoming = 0;
int buzz = 0;
BLECharacteristic *pCharacteristic;

void setup(){
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(115200);
  pinMode(5, OUTPUT);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  pinMode(18, INPUT);

  Serial.println("Starting BLE work!");
  BLEDevice::init("project_gesture");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
}

void loop(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
    int xAng = map(AcX,minVal,maxVal,-90,90);
    int yAng = map(AcY,minVal,maxVal,-90,90);
    int zAng = map(AcZ,minVal,maxVal,-90,90);

       x= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
       y= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
       z= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);

     //signal buzzer and vibrator if the angle tilts more than +-20 deg along xy direction
     if ((x<340)&&(x>20)||(y<340)&&(y>20)){
      digitalWrite(17,HIGH);
      digitalWrite(16,HIGH); 
     }
     else{
      digitalWrite(17,LOW);
      digitalWrite(16,LOW);
     }
     
     //signal low battery measure LED  
     while (low_count < 10) {
      lowbat += analogRead(A13);
      low_count++;
      delay(10);
     }
     voltage = (lowbat / 10.0 * 5.0) / 1024.0;
     voltage = voltage * 10.0 / 27.3;
     low_count = 0;
     lowbat = 0;
     if (voltage < 2.0){
       digitalWrite(5,HIGH);
     }
     
     //singal bluetooth transmition
     buzz = digitalRead(16);
     pCharacteristic->setValue(buzz);
     
     //bluetooth connection LED
     blue = digitalRead(18);
     if (blue == 0){
      digitalWrite(2,!blue); 
     }
//     Serial.print("AngleX= ");
//     Serial.println(x);
//
//     Serial.print("AngleY= ");
//     Serial.println(y);
//
//     Serial.print("AngleZ= ");
//     Serial.println(z);
//     Serial.println("-----------------------------------------");
//     delay(1000);
}
