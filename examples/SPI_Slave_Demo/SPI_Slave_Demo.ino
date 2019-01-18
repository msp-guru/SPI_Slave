/*
  SPI_Slave_Demo
  
  This example Demos the SPI Slave mode.
 
 created 09 Jan 2019
 by StefanSch
  
*/


// include the SPI Slave library:
#include <SPI_Slave.h>

uint8_t txbuffer[10] = {0,1,2,3,4,5,6,7,8,9};
uint8_t rxbuffer[10];

void setup() {
  //Initialize serial 
  Serial.begin(9600); 

  // initialize SPI Slave with STE low = active:

  SPISlave.setModule(13);
    pinMode_int(5, PORT_SELECTION0);
    pinMode_int(4, PORT_SELECTION0);
    pinMode_int(3, PORT_SELECTION0);
    pinMode_int(8, PORT_SELECTION0); 
  SPISlave.setMode(MODE_4WIRE_STE0); 
  SPISlave.begin(); 
}

void loop() {
  SPISlave.transfer(txbuffer, sizeof(txbuffer));
  while(!SPISlave.transactionDone());
  Serial.print(txbuffer[0], HEX);
}