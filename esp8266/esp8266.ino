/*
 *  ESP8266 Communication and Protocols
 *  SPIFFS Direct File Upload Example
 *  -Manoj R. Thkuar
 */

#include <ESP8266WiFi.h>
#include <FS.h>   //Include File System Headers

#define UART_DATA_SIZE 120
#define MAX_NUMBER_OF_CHECKSUM 65535
#define UART_PACKAGE_SIZE 128

struct uart_transfer_format{
  uint16_t header;
  uint16_t sequence_package_number;
  uint16_t total_package_number;
  uint8_t data[120];
  uint16_t checksum;
};

unsigned char uart_raw_data[UART_PACKAGE_SIZE];

const char* file = "/blink.bin";   //Enter your file name
void setup() {
  delay(10000);
  Serial.begin(115200);
  //Serial.println("Begin");
  SPIFFS.begin();
  //Serial.println("File System Initialized");
  pinMode(12,OUTPUT);
  digitalWrite(12,HIGH);
}
byte serialOut[128];
File dataFile;
uart_transfer_format uart_data;

void loop() {
    dataFile = SPIFFS.open(file, "r");   //Open File for reading
    //Serial.println();
    //Serial.println("--------Reading Data from File-----------");
    //Data from file
    digitalWrite(12,LOW);
    delay(100);
    digitalWrite(12,HIGH);
    delay(10000);
    int TotalSize = dataFile.size();
    int inner_loop = ceil(TotalSize/UART_DATA_SIZE);
    if (TotalSize % UART_DATA_SIZE)
        inner_loop++;
    
    //Serial.print("Firmware Size is: ");Serial.println(TotalSize);
    //Serial.print("Number of packets will be used: ");Serial.println(inner_loop);
    uint16_t checksum;

      uart_data.header = 0xABCD;
      uart_data.total_package_number = inner_loop;
    for(int i=0; i<uart_data.total_package_number; i++){
      uart_data.sequence_package_number = i + 1;
      for(int k=0; k<UART_DATA_SIZE; k++){
        
        uart_data.data[k] = dataFile.read();
        checksum += uart_data.data[k];
        
      }
      checksum += uart_data.sequence_package_number;
      checksum += uart_data.total_package_number;
      uart_data.header = 0xABCD;
      uart_data.checksum = checksum;

      memcpy(&uart_raw_data[0], (unsigned char *)&uart_data, UART_PACKAGE_SIZE);
     delayMicroseconds(1000);
      for(int i=0; i<UART_PACKAGE_SIZE; i++)
        Serial.print(uart_raw_data[i],HEX);
        
    }
    dataFile.close();
    delay(5000);
  

}
