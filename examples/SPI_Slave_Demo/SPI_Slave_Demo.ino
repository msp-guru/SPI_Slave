/*
  SPI_Slave_Demo
  
  This example Demos the SPI Slave modewith a variable length of data send in one package
  between CS pulses.
 
 created 23 Jan 2019
 by StefanSch
  
*/


// include the SPI Slave library:
#include <SPI_Slave.h>

uint8_t txbuffer[10] = {0,1,2,3,4,5,6,7,8,9};
uint8_t rxbuffer[10];

#define STE 8

void setup() {
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);   // set the LED off
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);   // set the LED off
  //Initialize serial 
  Serial.begin(115200);

  Serial.println("\nSlave Started");

  // sync on STE
  pinMode(STE, INPUT);
  while (digitalRead(STE)!=HIGH); // wait for High = disable -> start

  // initialize SPI Slave with STE low = active:
  SPISlave.setModule(13);
    pinMode_int(5, PORT_SELECTION0);
    pinMode_int(4, PORT_SELECTION0);
    pinMode_int(3, PORT_SELECTION0);
    pinMode_int(STE, PORT_SELECTION0);
  SPISlave.begin(SPISlaveSettings(MODE_4WIRE_STE0, MSBFIRST, SPI_MODE0));
}

void loop() {
  uint8_t i;
  SPISlave.transfer(txbuffer, sizeof(txbuffer));  /* rx buffer = tx buffer */

  digitalWrite(RED_LED, HIGH);   // set the LED on
  while (SPISlave.bytes_received() == 0);
  while (SPISlave.bytes_received() == 0 && (SPISlave.getCS(STE) == LOW)){
          digitalWrite(GREEN_LED, HIGH);   // set the LED on
  }
  digitalWrite(GREEN_LED, LOW);   // set the LED off
  digitalWrite(RED_LED, LOW);   // set the LED off

  Serial.print("=> "); // data received
  for (i = 0; i<SPISlave.bytes_received(); i++){
      Serial.print(txbuffer[i], HEX);
      Serial.print(" ");
  }
  Serial.println("");
}
