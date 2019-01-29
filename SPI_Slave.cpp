/*
    Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
    SPI Master library for arduino.

    This file is free software; you can redistribute it and/or modify
    it under the terms of either the GNU General Public License version 2
    or the GNU Lesser General Public License version 2.1, both as
    published by the Free Software Foundation.

    2012-04-29 rick@kimballsoftware.com - added msp430 support.

*/

#include <Energia.h>

#include "SPI_Slave.h"

SPISlaveClass::SPISlaveClass(void)
{
#if defined(DEFAULT_SPI)
    setModule(DEFAULT_SPI);
#else
    setModule(0);
#endif
}

void SPISlaveClass::setModule(uint8_t module)
{
    spiModule = module;
#if defined(__MSP430_HAS_EUSCI_B0__)
    if (module == 0)
    {
        SPI_slave_baseAddress = UCB0_BASE;
    }
#endif
#if defined(__MSP430_HAS_EUSCI_B1__)
    if (module == 1)
    {
        SPI_slave_baseAddress = UCB1_BASE;
    }
#endif
#if defined(__MSP430_HAS_EUSCI_B2__)
    if (module == 2)
    {
        SPI_slave_baseAddress = UCB2_BASE;
    }
#endif
#if defined(__MSP430_HAS_EUSCI_B3__)
    if (module == 3)
    {
        SPI_slave_baseAddress = UCB3_BASE;
    }
#endif
#if defined(__MSP430_HAS_EUSCI_A0__)
    if (module == 10)
    {
        SPI_slave_baseAddress = UCA0_BASE;
    }
#endif
#if defined(__MSP430_HAS_EUSCI_A1__)
    if (module == 11)
    {
        SPI_slave_baseAddress = UCA1_BASE;
    }
#endif
#if defined(__MSP430_HAS_EUSCI_A2__)
    if (module == 12)
    {
        SPI_slave_baseAddress = UCA2_BASE;
    }
#endif
#if defined(__MSP430_HAS_EUSCI_A3__)
    if (module == 13)
    {
        SPI_slave_baseAddress = UCA3_BASE;
    }
#endif
}

void SPISlaveClass::initPins()
{
   /* Set pins to SPI mode. */
#if defined(DEFAULT_SPI)
#if defined(UCB0_BASE) && defined(SPISCK0_SET_MODE)
    if (SPI_slave_baseAddress == UCB0_BASE) {
        pinMode_int(SCK0, SPISCK0_SET_MODE);
        pinMode_int(MOSI0, SPIMOSI0_SET_MODE);
        pinMode_int(MISO0, SPIMISO0_SET_MODE);
        pinMode_int(SS0, SPISCK0_SET_MODE); // STE=/CS
    }
#endif   
#if defined(UCB1_BASE) && defined(SPISCK1_SET_MODE)
    if (SPI_slave_baseAddress == UCB1_BASE) {
        pinMode_int(SCK1, SPISCK1_SET_MODE);
        pinMode_int(MOSI1, SPIMOSI1_SET_MODE);
        pinMode_int(MISO1, SPIMISO1_SET_MODE);
        pinMode_int(SS1, SPISCK1_SET_MODE); // STE=/CS
    }
#endif   
#if defined(UCB2_BASE) && defined(SPISCK2_SET_MODE)
    if (SPI_slave_baseAddress == UCB2_BASE) {
        pinMode_int(SCK2, SPISCK2_SET_MODE);
        pinMode_int(MOSI2, SPIMOSI2_SET_MODE);
        pinMode_int(MISO2, SPIMISO2_SET_MODE);
        pinMode_int(SS2, SPISCK2_SET_MODE); // STE=/CS
    }
#endif   
#if defined(UCB3_BASE) && defined(SPISCK3_SET_MODE)
    if (SPI_slave_baseAddress == UCB3_BASE) {
        pinMode_int(SCK3, SPISCK3_SET_MODE);
        pinMode_int(MOSI3, SPIMOSI3_SET_MODE);
        pinMode_int(MISO3, SPIMISO3_SET_MODE);
        pinMode_int(SS3, SPISCK3_SET_MODE); // STE=/CS
    }
#endif   
#if defined(UCA0_BASE) && defined(SPISCK10_SET_MODE)
    if (SPI_slave_baseAddress == UCA0_BASE) {
        pinMode_int(SCK10, SPISCK10_SET_MODE);
        pinMode_int(MOSI10, SPIMOSI10_SET_MODE);
        pinMode_int(MISO10, SPIMISO10_SET_MODE);
        pinMode_int(SS10, SPISCK10_SET_MODE); // STE=/CS
    }
#endif   
#if defined(UCA1_BASE) && defined(SPISCK11_SET_MODE)
    if (SPI_slave_baseAddress == UCA1_BASE) {
        pinMode_int(SCK11, SPISCK11_SET_MODE);
        pinMode_int(MOSI11, SPIMOSI11_SET_MODE);
        pinMode_int(MISO11, SPIMISO11_SET_MODE);
        pinMode_int(SS11, SPISCK11_SET_MODE); // STE=/CS
    }
#endif   
#if defined(UCA2_BASE) && defined(SPISCK12_SET_MODE)
    if (SPI_slave_baseAddress == UCA2_BASE) {
        pinMode_int(SCK12, SPISCK12_SET_MODE);
        pinMode_int(MOSI12, SPIMOSI12_SET_MODE);
        pinMode_int(MISO12, SPIMISO12_SET_MODE);
        pinMode_int(SS12, SPISCK12_SET_MODE); // STE=/CS
    }
#endif   
#if defined(UCA3_BASE) && defined(SPISCK13_SET_MODE)
    if (SPI_slave_baseAddress == UCA3_BASE) {
        pinMode_int(SCK13, SPISCK13_SET_MODE);
        pinMode_int(MOSI13, SPIMOSI13_SET_MODE);
        pinMode_int(MISO13, SPIMISO13_SET_MODE);
        pinMode_int(SS13, SPISCK13_SET_MODE); // STE=/CS
    }
#endif   
#else
    pinMode_int(SCK, SPISCK_SET_MODE);
    pinMode_int(MOSI, SPIMOSI_SET_MODE);
    pinMode_int(MISO, SPIMISO_SET_MODE);
    pinMode_int(SS, SPISCK_SET_MODE); // STE=/CS
#endif
}

/*
    Pre-Initialize a SPI instances
*/
SPISlaveClass SPISlave;
