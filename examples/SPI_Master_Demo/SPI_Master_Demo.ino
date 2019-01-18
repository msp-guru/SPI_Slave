/*
  SPI_Master_Demo
  
  This example contains the SPI Master code for the SPI Slave demo.
  Program this code to a device which should act as master and connect the 4 wires used for SPI
  communication the the Slave *SCK, MISO, SIMO, SS (STE)
 
 created 09 Jan 2019
 by StefanSch
  
*/

uint8_t data = 0;

// include the SPI library:
#include <SPI.h>


void setup() {
  //Initialize serial 
  Serial.begin(9600); 

  // initialize SPI Slave:
  SPI.begin(); 
}

void loop() {
  

  // take the SS pin low to select the chip:
  digitalWrite(slaveSelectPin,LOW);
  //  send and receive 2 bytes via SPI:
  data = SPI.transfer(data++);
  data = SPI.transfer(data++);
  // take the SS pin high to de-select the chip:
  digitalWrite(slaveSelectPin,HIGH); 
}

