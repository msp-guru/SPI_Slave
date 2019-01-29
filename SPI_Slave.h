/*
    Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
    SPI Master library for arduino.

    This file is free software; you can redistribute it and/or modify
    it under the terms of either the GNU General Public License version 2
    or the GNU Lesser General Public License version 2.1, both as
    published by the Free Software Foundation.

    2012-04-29 rick@kimballsoftware.com - added msp430 support.

*/

#ifndef _SPISLAVE_H_INCLUDED
#define _SPISLAVE_H_INCLUDED

#include <Energia.h>
#include <inttypes.h>

#if defined(__MSP430_HAS_USI__) || defined(__MSP430_HAS_USCI_B0__) || defined(__MSP430_HAS_USCI_B1__) || defined(__MSP430_HAS_USCI__) || defined(__MSP430_HAS_EUSCI_B0__) || defined(__MSP430_HAS_EUSCI_B1__) || defined(__MSP430_HAS_EUSCI_B2__) || defined(__MSP430_HAS_EUSCI_B3__) || defined(DEFAULT_SPI)
#include "utility/spi_slave_430.h"
#endif

#define SPI_MODE0 0
#define SPI_MODE1 1
#define SPI_MODE2 2
#define SPI_MODE3 4

#define MODE_3WIRE      0
#define MODE_4WIRE_STE1 1
#define MODE_4WIRE_STE0 2


#if defined(__MSP430_HAS_USCI_B0__)
#define UCB0_BASE __MSP430_BASEADDRESS_USCI_B0__
#endif
#if defined(__MSP430_HAS_USCI_B1__)
#define UCB1_BASE __MSP430_BASEADDRESS_USCI_B1__
#endif
#if defined(__MSP430_HAS_USCI_B2__)
#define UCB2_BASE __MSP430_BASEADDRESS_USCI_B2__
#endif
#if defined(__MSP430_HAS_USCI_B3__)
#define UCB3_BASE __MSP430_BASEADDRESS_USCI_B3__
#endif

#if defined(__MSP430_HAS_EUSCI_B0__)
#define UCB0_BASE __MSP430_BASEADDRESS_EUSCI_B0__
#endif
#if defined(__MSP430_HAS_EUSCI_B1__)
#define UCB1_BASE __MSP430_BASEADDRESS_EUSCI_B1__
#endif
#if defined(__MSP430_HAS_EUSCI_B2__)
#define UCB2_BASE __MSP430_BASEADDRESS_EUSCI_B2__
#endif
#if defined(__MSP430_HAS_EUSCI_B3__)
#define UCB3_BASE __MSP430_BASEADDRESS_EUSCI_B3__
#endif

#if defined(__MSP430_HAS_EUSCI_A0__)
#define UCA0_BASE __MSP430_BASEADDRESS_EUSCI_A0__
#endif
#if defined(__MSP430_HAS_EUSCI_A1__)
#define UCA1_BASE __MSP430_BASEADDRESS_EUSCI_A1__
#endif
#if defined(__MSP430_HAS_EUSCI_A2__)
#define UCA2_BASE __MSP430_BASEADDRESS_EUSCI_A2__
#endif
#if defined(__MSP430_HAS_EUSCI_A3__)
#define UCA3_BASE __MSP430_BASEADDRESS_EUSCI_A3__
#endif


class SPISlaveSettings
{
  public:
    uint8_t _bitOrder;
    uint8_t _datamode;
    uint8_t _mode;
    SPISlaveSettings(uint32_t mode, uint8_t bitOrder, uint8_t dataMode) {
        init_AlwaysInline(mode, bitOrder, dataMode);
    }
    SPISlaveSettings() {
        init_AlwaysInline(MODE_4WIRE_STE0, MSBFIRST, SPI_MODE0);
    }
  private:

    void init_AlwaysInline(uint32_t mode, uint8_t bitOrder, uint8_t dataMode)
    __attribute__((__always_inline__)) {

        // Pack into the SPISlaveSettings class
        _bitOrder = bitOrder;
        _datamode     = dataMode ;
        _mode     =  mode;;
    }
    friend class SPISlaveClass;
};

extern uint8_t spiModule ;

class SPISlaveClass
{
  private:
    void initPins();

  public:

    
    inline static bool transactionDone(void);
    inline static size_t bytes_to_transmit(void);
    inline static size_t bytes_received(void);
    inline static int getCS(uint8_t pin);
    inline static void transfer(uint8_t *buf, size_t count);
    inline static void transfer(uint8_t *rxbuf, uint8_t *txbuf, size_t count);

    // SPI Configuration methods
    SPISlaveClass(void);
    inline static void begin(); // Default
    inline static void begin(SPISlaveSettings settings);
    inline static void begin(uint8_t module);
    inline static void begin(SPISlaveSettings settings, uint8_t module);
    inline static void begin(SPISlaveSettings settings, uint8_t module, uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t cs, uint8_t pin_mode);
    inline static void end();

    inline static void attachInterrupt();
    inline static void detachInterrupt();

    void setModule(uint8_t);
};

extern SPISlaveClass SPISlave;

void SPISlaveClass::begin(void)
{
    SPISlave.initPins();
    spi_slave_initialize(MODE_4WIRE_STE0, SPI_MODE0, MSBFIRST);
}

void SPISlaveClass::begin(SPISlaveSettings settings)
{
    SPISlave.initPins();
    spi_slave_initialize(settings._mode, settings._datamode, settings._bitOrder);
}

void SPISlaveClass::begin(uint8_t module)
{
    SPISlave.setModule(module);
    begin();
}

void SPISlaveClass::begin(SPISlaveSettings settings, uint8_t module)
{
    SPISlave.setModule(module);
    begin(settings);
}

void SPISlaveClass::begin(SPISlaveSettings settings, uint8_t module, uint8_t sck, uint8_t mosi, uint8_t miso, uint8_t cs, uint8_t pin_mode)
{
    pinMode_int(sck,  pin_mode); // SCK
    pinMode_int(mosi, pin_mode); // MOSI
    pinMode_int(miso, pin_mode); // MISO
    if (cs > 0) 
    {
        pinMode_int(cs, pin_mode); // STE=/CS
    }

    SPISlave.setModule(module);
    spi_slave_initialize(settings._mode, settings._datamode, settings._bitOrder);
}

void SPISlaveClass::transfer(uint8_t *buf, size_t count)
{
    spi_slave_transfer(buf, buf, count);
}

void SPISlaveClass::transfer(uint8_t *rxbuf, uint8_t *txbuf, size_t count)
{
    spi_slave_transfer(rxbuf, txbuf, count);
}

bool SPISlaveClass::transactionDone(void)
{
    return (spi_data_done());
}

int SPISlaveClass::getCS(uint8_t pin)
{
    uint8_t bit = digitalPinToBitMask(pin);
    uint8_t port = digitalPinToPort(pin);

    if (port == NOT_A_PORT)
    {
        return LOW;
    }

    if (*portInputRegister(port) & bit)
    {
        return HIGH;
    }
    return LOW;
}

size_t SPISlaveClass::bytes_to_transmit(void)
{
    return (spi_bytes_to_transmit());
}

size_t SPISlaveClass::bytes_received(void)
{
    return (spi_bytes_received());
}

void SPISlaveClass::end()
{
    spi_slave_disable();
}

void SPISlaveClass::attachInterrupt()
{
    /* undocumented in Arduino 1.0 */
}

void SPISlaveClass::detachInterrupt()
{
    /* undocumented in Arduino 1.0 */
}

#endif
