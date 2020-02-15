
#include<Wire.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
//#include <BLEServer.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define SERVICE_UUID        "f78bd81c-1a66-4c50-8f24-632988e64dad"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

//IMU related variables 
const int MPU_addr=0x68;
int16_t AcX,AcY,AcZ;
//,Tmp,GyX,GyY,GyZ;
int minVal=265;
int maxVal=402;
double x;
double y;
double z;

//Low battery detection 
int lowbat = 0;
int low_count = 0;
float voltage = 0.0;

//bluetooth related variables
int blue = 0;
int buzz = 0;
BLECharacteristic *pCharacteristic;

//SD card variable
File root;
int buzz_count = 0;
int gate1 = 0;
int gate2 = 0;

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
  pinMode(36, INPUT);
  
  // SD Card Initialization
  if (SD.begin()){
    Serial.println("SD card is ready to use.");
  } else{
    Serial.println("SD card initialization failed");
    return;
  }
  writeFile(SD, "/Gesture.txt", "Welcome to Gesture, here are the information of incorrect posture detected: \n");
//  readFile(SD, "/Gesture.txt");
//  deleteFile(SD, "/Gesture.txt");
//  root = SD.open("/");
//  printDirectory(root, 0);


  //Bluetooth initialization
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
  //MPU initialization
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);

  //finding angles of 3 XYZ axis in degree by acclerarion
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  int xAng = map(AcX,minVal,maxVal,-90,90);
  int yAng = map(AcY,minVal,maxVal,-90,90);
  int zAng = map(AcZ,minVal,maxVal,-90,90);
  x= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
  y= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
  z= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);

  //Signal buzzer and vibrator if the angle tilts more than +-20 deg along xy direction
  if ((x<340)&&(x>20)||(y<340)&&(y>20)){
   digitalWrite(17,HIGH);
   digitalWrite(16,HIGH); 
  }
  else{
   digitalWrite(17,LOW);
   digitalWrite(16,LOW);
  }
     
  //insert incorrect posture record, write into SD card
  buzz = digitalRead(16);
  if ((buzz==HIGH) && (gate1==1)){
    appendFile(SD, "/Gesture.txt", buzz_count + "\t time \t angle\n");
    buzz_count++;
    gate1 = 0;  
  }
  else if(buzz==LOW){
    gate1 = 1;
  }

     
  //bluetooth connection LED --> replaced:using a wire as button
  blue = digitalRead(36);
//  if (blue == 0){
//   digitalWrite(2,!blue); 
//  }
  if ((blue == HIGH)&&(gate2 == 1)){
    String message;
    readFile(SD, "/Gesture.txt",message); 
    int length = message.length();
    char mes[length];
    message.toCharArray(mes,length);
    pCharacteristic->setValue(mes);
    Serial.println("sent through blutooth:");
    Serial.println(message);
    gate2 = 0;
  }
  else if((blue == LOW)){
    pCharacteristic->setValue("111");
    Serial.println("111");
    gate2 = 1;
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

  //signal low battery measure LED(not tested)  
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
}


void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void readFile(fs::FS &fs, const char * path, String& message){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
      char temp = file.read();
      message += temp;
          //Serial.write(file.read());
    }
//    Serial.println(message);
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}
