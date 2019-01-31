/*
    spi_slave_430.h - common function declarations for different SPI Slave implementations

    spi slave implementation by StefanSch
    based on:
    Copyright (c) 2012 by Rick Kimball <rick@kimballsoftware.com>
    spi abstraction api for msp430

    This file is free software; you can redistribute it and/or modify
    it under the terms of either the GNU General Public License version 2
    or the GNU Lesser General Public License version 2.1, both as
    published by the Free Software Foundation.

*/

#ifndef _SPI_SLAVE_430_H_
#define _SPI_SLAVE_430_H_

#if defined(__MSP430_HAS_USCI_B0__) || defined(__MSP430_HAS_USCI_B1__) || defined(__MSP430_HAS_USCI__) || defined(__MSP430_HAS_EUSCI_B0__) || defined(__MSP430_HAS_EUSCI_B1__) || defined(DEFAULT_SPI)

#elif defined(__MSP430_HAS_USI__)

#else
#error "SPI not supported by hardware on this chip"
#endif

extern uint16_t SPI_slave_baseAddress;
extern uint8_t spiSlaveModule;


void spi_slave_initialize(const uint8_t, const uint8_t, const uint8_t order);
void spi_slave_disable(void);
void spi_slave_transfer(uint8_t *rxbuf, uint8_t *txbuf, uint16_t count);
void spi_slave_receive(uint8_t *buf, uint16_t count);
int spi_data_done(void);
int spi_bytes_to_transmit(void);
int spi_bytes_received(void);


#endif /*_SPI_SLAVE_430_H_*/
