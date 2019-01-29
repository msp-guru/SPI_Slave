/*
    SPI_Slave_Demo

    This example Demos the SPI Slave modewith a variable length of data send in one package
    between CS pulses.

    created 23 Jan 2019
    by StefanSch

*/


// include the SPI Slave library:
#include <SPI_Slave.h>

uint8_t txbuffer[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
uint8_t rxbuffer[10];

#define STE 17

void setup()
{
    pinMode(RED_LED, OUTPUT);
    digitalWrite(RED_LED, LOW);   // set the LED off
    pinMode(GREEN_LED, OUTPUT);
    digitalWrite(GREEN_LED, LOW);   // set the LED off
    //Initialize serial
    Serial.begin(115200);

    Serial.println("\nSlave Started");

    // sync on STE
    pinMode(STE, INPUT);
    while (digitalRead(STE) != HIGH); // wait for High = disable -> start

    // initialize SPI Slave with STE low = active:
#if 1
    pinMode_int(17 , PORT_SELECTION0); // STE=/CS
    SPISlave.begin(SPISlaveSettings(MODE_4WIRE_STE0, MSBFIRST, SPI_MODE0));
#else
    SPISlave.begin(SPISlaveSettings(MODE_4WIRE_STE0, MSBFIRST, SPI_MODE0), 
    13, /* Module */
    5, /* SCK */
    3, /* MOSI */
    4, /* MISO */
    STE, /* CS */
    PORT_SELECTION0 /* PIN Mode */
    );
#endif
}

void loop()
{
    uint8_t i;
    SPISlave.transfer(rxbuffer, txbuffer, sizeof(txbuffer));  /* rx buffer = tx buffer */

    digitalWrite(RED_LED, HIGH);   // set the LED on
    while (SPISlave.bytes_received() == 0);
    while (SPISlave.bytes_received() == 0 && (SPISlave.getCS(STE) == LOW))
    {
        digitalWrite(GREEN_LED, HIGH);   // set the LED on
    }
    digitalWrite(GREEN_LED, LOW);   // set the LED off
    digitalWrite(RED_LED, LOW);   // set the LED off

    Serial.print("RX => "); // data received
    for (i = 0; i < SPISlave.bytes_received(); i++)
    {
        Serial.print(rxbuffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println("");
    Serial.print("TX <= "); // data sent
    for (i = 0; i < SPISlave.bytes_received(); i++)
    {
        Serial.print(txbuffer[i], HEX);
        Serial.print(" ");
    }
    Serial.println("");
}
