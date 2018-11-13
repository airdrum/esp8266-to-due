/*
 *  ESP8266 Communication and Protocols
 *  SPIFFS Direct File Upload Example
 *  -Manoj R. Thkuar
 */

#include <ESP8266WiFi.h>
#include <FS.h>   //Include File System Headers

const char* file = "/blink.bin";   //Enter your file name
void setup() {
  delay(10000);
  Serial.begin(115200);
  //Serial.begin(115200);
  Serial.println("Begin");

  //Initialize File System
  SPIFFS.begin();
  //esp8266.println("File System Initialized");
  Serial.println("File System Initialized");
  pinMode(12,OUTPUT);
digitalWrite(12,HIGH);
}

void loop() {
    File dataFile = SPIFFS.open(file, "r");   //Open File for reading
        Serial.println();
    Serial.println("--------Reading Data from File-----------");
    //Data from file
    digitalWrite(12,LOW);
    delay(100);
    digitalWrite(12,HIGH);
    delay(10000);

    for(int i=0;i<dataFile.size();i++) //Read upto complete file size
    {
      Serial.print("0x");    //Read file
      Serial.print(dataFile.read(),HEX);    //Read file
      Serial.print(", ");    //Read file
      delayMicroseconds(10);
    }
    Serial.println();
    dataFile.close();
    delay(5000);

}
