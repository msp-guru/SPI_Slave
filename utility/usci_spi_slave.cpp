/**
 * File: usci_spi.c - msp430 USCI SPI implementation
 *
 * Copyright (c) 2012 by Rick Kimball <rick@kimballsoftware.com>
 * spi abstraction api for msp430
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 *
 */

#include <msp430.h>
#include <stdint.h>
#include "spi_slave_430.h"
#include <Energia.h>

#if defined(__MSP430_HAS_USCI_B0__) || defined(__MSP430_HAS_USCI_B1__) || defined(__MSP430_HAS_USCI__)

/**
 * USCI flags for various the SPI MODEs
 *
 * Note: The msp430 UCCKPL tracks the CPOL value. However,
 * the UCCKPH flag is inverted when compared to the CPHA
 * value described in Motorola documentation.
 */

#define SPI_MODE_0 (UCCKPH)			    /* CPOL=0 CPHA=0 */
#define SPI_MODE_1 (0)                 	/* CPOL=0 CPHA=1 */
#define SPI_MODE_2 (UCCKPL | UCCKPH)    /* CPOL=1 CPHA=0 */
#define SPI_MODE_3 (UCCKPL)			    /* CPOL=1 CPHA=1 */

#define SPI_MODE_MASK (UCCKPL | UCCKPH)

/**
 * spi_slave_initialize() - Configure USCI UCB0 for SPI mode
 *
 * P2.0 - CS (active low)
 * P1.5 - SCLK
 * P1.6 - MISO aka SOMI
 * P1.7 - MOSI aka SIMO
 *
 */


void spi_slave_initialize(const uint8_t mode)
{
	UCB0CTL1 = UCSWRST;      // Put USCI in reset mode, source USCI clock from SMCLK
	UCB0CTL0 = SPI_MODE_0 | UCMSB | UCSYNC | UCMODE_1;  // Use SPI MODE 0 - CPOL=0 CPHA=0 - 4 wire STE = 1
	spi_slave_set_mode(mode);

 	/* Set pins to SPI mode. */
	pinMode_int(SCK, SPISCK_SET_MODE);
	pinMode_int(MOSI, SPIMOSI_SET_MODE);
	pinMode_int(MISO, SPIMISO_SET_MODE);
	if (mode > 0){
		pinMode_int(SS, SPISCK_SET_MODE);  /* SPISS_SET_MODE is not defined in pins_enegia.h so hope this has the same */
	}

	UCB0CTL1 &= ~UCSWRST;			    // release USCI for operation
}

/**
 * spi_slave_disable() - put USCI into reset mode
 */
void spi_slave_disable(void)
{
    UCB0CTL1 |= UCSWRST;                // Put USCI in reset mode
}

/**
 * spi_slave_send() - send a byte and recv response
 */
uint8_t spi_slave_send(const uint8_t _data)
{
	UCB0TXBUF = _data; // setting TXBUF clears the TXIFG flag
	while (UCB0STAT & UCBUSY)
		; // wait for SPI TX/RX to finish

	return UCB0RXBUF; // reading clears RXIFG flag
}

#ifdef UC0IFG
/* redefine or older 2xx devices where the flags are in SFR */
#define UCB0IFG  UC0IFG   
#define UCRXIFG  UCB0RXIFG
#define UCTXIFG  UCB0TXIFG
#endif

uint16_t spi_slave_send16(const uint16_t data)
{
	uint16_t datain;
	/* Wait for previous tx to complete. */
	while (!(UCB0IFG & UCTXIFG));
	/* Setting TXBUF clears the TXIFG flag. */
	UCB0TXBUF = data | 0xFF;
	/* Wait for previous tx to complete. */
	while (!(UCB0IFG & UCTXIFG));

	datain = UCB0RXBUF << 8;
	/* Setting TXBUF clears the TXIFG flag. */
	UCB0TXBUF = data >> 8;

	/* Wait for a rx character? */
	while (!(UCB0IFG & UCRXIFG));

	/* Reading clears RXIFG flag. */
	return (datain | UCB0RXBUF);
}

void spi_slave_send(void *buf, uint16_t count)
{
    uint8_t *ptx = (uint8_t *)buf;
    uint8_t *prx = (uint8_t *)buf;
	if (count == 0) return;
	/* Wait for previous tx to complete. */
	while (!(UCB0IFG & UCTXIFG));
	while(count){
		if (UCB0IFG & UCRXIFG){
			/* Reading RXBUF clears the RXIFG flag. */
			*prx++ = UCB0RXBUF;
		}
		if (UCB0IFG & UCTXIFG){
			/* Setting TXBUF clears the TXIFG flag. */
			UCB0TXBUF = *ptx++;
			count--;
		}
	}
	/* Wait for last rx character? */
	while (!(UCB0IFG & UCRXIFG));
	*prx++ = UCB0RXBUF;
}

/**
 * spi_slave_transmit() - send a byte
 */
void spi_slave_transmit(const uint8_t _data)
{
	UCB0TXBUF = _data; // setting TXBUF clears the TXIFG flag

	while (UCB0STAT & UCBUSY); // wait for SPI TX/RX to finish
	// clear RXIFG flag
	UCB0IFG &= ~UCRXIFG;
}

void spi_slave_transmit16(const uint16_t data)
{
	/* Wait for previous tx to complete. */
	while (!(UCB0IFG & UCTXIFG));
	/* Setting TXBUF clears the TXIFG flag. */
	UCB0TXBUF = data | 0xFF;
	/* Wait for previous tx to complete. */
	while (!(UCB0IFG & UCTXIFG));
	/* Setting TXBUF clears the TXIFG flag. */
	UCB0TXBUF = data >> 8;

	while (UCB0STAT & UCBUSY); // wait for SPI TX/RX to finish
	// clear RXIFG flag
	UCB0IFG &= ~UCRXIFG;
}

void spi_slave_transmit(void *buf, uint16_t count)
{
    uint8_t *ptx = (uint8_t *)buf;
	if (count == 0) return;
	while(count){
		if (UCB0IFG & UCTXIFG){
			/* Setting TXBUF clears the TXIFG flag. */
			UCB0TXBUF = *ptx++;
			count--;
		}
	}
	while (UCB0STAT & UCBUSY); // wait for SPI TX/RX to finish
	// clear RXIFG flag
	UCB0IFG &= ~UCRXIFG;
}


/**
 * spi_slave_set_bitorder(LSBFIRST=0 | MSBFIRST=1)
 */
void spi_slave_set_bitorder(const uint8_t order)
{
    UCB0CTL1 |= UCSWRST;        // go into reset state
    UCB0CTL0 = (UCB0CTL0 & ~UCMSB) | ((order == 1 /*MSBFIRST*/) ? UCMSB : 0); /* MSBFIRST = 1 */
    UCB0CTL1 &= ~UCSWRST;       // release for operation
}

/**
 * spi_slave_set_mode() - mode 0 - 3
 */
void spi_slave_set_mode(const uint8_t mode)
{
    UCB0CTL1 |= UCSWRST;        // go into reset state
    switch(mode) {
    case 0: /* SPI_MODE0 */
		UCB0CTL0 = (UCB0CTL0 & ~UCMODE_3) | UCMODE_0;
        break;
    case 1: /* SPI_MODE1 */
		UCB0CTL0 = (UCB0CTL0 & ~UCMODE_3) | UCMODE_1;
        break;
    case 2: /* SPI_MODE2 */
		UCB0CTL0 = (UCB0CTL0 & ~UCMODE_3) | UCMODE_2;
        break;
    default:
        break;
    }
    UCB0CTL1 &= ~UCSWRST;       // release for operation
}

/**
 * spi_slave_set_datamode() - datamode 0 - 3
 */
void spi_slave_set_datamode(const uint8_t datamode)
{
    UCB0CTL1 |= UCSWRST;        // go into reset state
    switch(datamode) {
    case 0: /* SPI_MODE0 */
        UCB0CTL0 = (UCB0CTL0 & ~SPI_MODE_MASK) | SPI_MODE_0;
        break;
    case 1: /* SPI_MODE1 */
        UCB0CTL0 = (UCB0CTL0 & ~SPI_MODE_MASK) | SPI_MODE_1;
        break;
    case 2: /* SPI_MODE2 */
        UCB0CTL0 = (UCB0CTL0 & ~SPI_MODE_MASK) | SPI_MODE_2;
        break;
    case 3: /* SPI_MODE3 */
        UCB0CTL0 = (UCB0CTL0 & ~SPI_MODE_MASK) | SPI_MODE_3;
        break;
    default:
        break;
    }
    UCB0CTL1 &= ~UCSWRST;       // release for operation
}
#else
    //#error "Error! This device doesn't have a USCI peripheral"
#endif
