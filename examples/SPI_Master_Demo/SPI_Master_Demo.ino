/*
  SPI_Master_Demo
  
  This example contains the SPI Master code for the SPI Slave demo.
  Program this code to a device which should act as master and connect the 4 wires used for SPI
  communication the the Slave *SCK, MISO, SIMO, SS (STE)
 
 created 09 Jan 2019
 by StefanSch
  
*/

#define DATASIZE 10
uint8_t data[DATASIZE];
uint8_t datain[DATASIZE];

// include the SPI library:
#include <SPI.h>

#ifndef SS
#define SS 5
#endif

void setup() {
  //Initialize serial 
  Serial.begin(115200);

  pinMode(PUSH1, INPUT_PULLUP);
  pinMode(PUSH2, INPUT_PULLUP);

  // initialize SPI Master:
  digitalWrite(SS,HIGH);
  pinMode(SS, OUTPUT);
  SPI.begin();
  SPI.setClockDivider(10);

  Serial.println("Master Started");

}

void sendData() {
  uint16_t i;
  static uint16_t count = 0;

  Serial.print("=> "); // new line
  for (i=0; i < DATASIZE; i++){
      data[i]= count++;
      Serial.print(data[i], HEX); // new data
      Serial.print(" ");
  }
  Serial.println(""); // new line

  // take the SS pin low to select the chip:
  digitalWrite(SS,LOW);
  SPI.transfer(data, DATASIZE);
  // take the SS pin high to de-select the chip:
  delay(10);
  digitalWrite(SS,HIGH);

  Serial.print("<= "); // new line
  for (i=0; i < DATASIZE; i++){
      Serial.print(data[i], HEX);   // last received data
      Serial.print(" ");
  }
  Serial.println(""); // new line

}


void loop() {

  if (digitalRead(PUSH2)==LOW) {
    delay(100);
    while (digitalRead(PUSH2)==LOW);
    sendData();
    delay(100);
  }
}
