/**
 * File: usci_spi_slave.c - msp430 USCI SPI Slave implementation
 *
 * USCI flavor implementation by StefanSch
 * based on:
 * Copyright (c) 2012 by Rick Kimball <rick@kimballsoftware.com>
 * spi slave abstraction api for msp430
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
#include "usci_isr_handler.h"

#if defined(__MSP430_HAS_USCI_B0__) || defined(__MSP430_HAS_USCI_B1__) || defined(__MSP430_HAS_USCI__)
#ifndef __data16_write_addr
#define __data16_write_addr(x,y) *(unsigned long int*)(x) = y
#endif

#if defined(DEFAULT_SPI)
    uint8_t spiModule = DEFAULT_SPI;
#else
    uint8_t spiModule = 0;
#endif


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

uint8_t * rxptr;
uint8_t * txptr;
uint16_t rxcount = 0;
uint16_t txcount = 0;
uint16_t rxrecived = 0;
uint8_t com_mode = 0; /* mode : 0 - RX and TX  1 - RX only, TX to dummy */
#define COM_MODE_RX  0x1
#define COM_MODE_DMA 0x2

const uint8_t dummy = 0xFF;

/**
 * spi_slave_initialize() - Configure USCI UCB0 for SPI mode
 *
 * Pxx - CS (active low)
 * Pxx - SCLK
 * Pxx - MISO aka SOMI
 * Pxx - MOSI aka SIMO
 *
 */

void spi_slave_initialize(const uint8_t mode, const uint8_t datamode, const uint8_t order)
{
	UCB0CTL1 = UCSWRST;      // Put USCI in reset mode, source USCI clock from SMCLK
	UCB0CTL0 = SPI_MODE_0 | UCMSB | UCSYNC | UCMODE_1;  // Use SPI MODE 0 - CPOL=0 CPHA=0 - 4 wire STE = 1

    /* Calling this dummy function prevents the linker
     * from stripping the USCI interupt vectors.*/ 
    usci_isr_install();
	
	/* Put USCI in reset mode, source USCI clock from SMCLK. */
	UCB0CTL1 = UCSWRST;

	/* SPI in slave MODE 0 - CPOL=0 SPHA=0. - 3 wire STE */
	UCB0CTL0 |= UCSYNC;
	
	UCB0CTL0 = (UCB0CTL0 & ~UCMSB) | ((order == 1 /*MSBFIRST*/) ? UCMSB : 0); /* MSBFIRST = 1 */

    switch(mode) {
    case 0: /* 3 wire  */
        UCB0CTL0 |= UCMODE_0;
        break;
    case 1: /* 4 wire STE = 1 */
        UCB0CTL0 |= UCMODE_1;
        break;
    case 2: /* 4 wire STE = 0 */
        UCB0CTL0 |= UCMODE_2;
        break;
    default:
        break;
    }

    switch(datamode) {
    case 0: /* SPI_MODE0 */
        UCB0CTL0 |= SPI_MODE_0;
        break;
    case 1: /* SPI_MODE1 */
        UCB0CTL0 |= SPI_MODE_1;
        break;
    case 2: /* SPI_MODE2 */
        UCB0CTL0 |= SPI_MODE_2;
        break;
    case 3: /* SPI_MODE3 */
        UCB0CTL0 |= SPI_MODE_3;
        break;
    default:
        break;
    }
#if defined(DMA_BASE)
    com_mode = COM_MODE_DMA;
#else	
    com_mode = 0;
#endif
#if defined(DMA_BASE) && defined(DMA0TSEL__UCA0RXIFG) && defined(DMA1TSEL__UCA0TXIFG)
		if (com_mode & COM_MODE_DMA){
			DMACTL0 = DMA0TSEL__UCA0RXIFG;
			__data16_write_addr((unsigned short)(DMA0SA),(unsigned long)&UCA0RXBUF);

			DMACTL0 = DMA1TSEL__UCA0TXIFG;
			__data16_write_addr((unsigned short)(DMA1DA),(unsigned long)&UCA0TXBUF); 
		}
#endif

	/* Release USCI for operation. */
	UCB0CTL1 &= ~UCSWRST;			    // release USCI for operation
}


/**
 * spi_slave_disable() - put USCI into reset mode.
 */
void spi_slave_disable(void)
{
	/* Wait for previous tx to complete. */
	while (UCB0STAT & UCBUSY);
	/* Put USCI in reset mode. */
	UCB0CTL1 |= UCSWRST;
}

/**
 * spi_slave_transfer() - send a bytes and recv response.
 */

void spi_slave_transfer(uint8_t *rxbuf, uint8_t *txbuf, uint16_t count)
{
    rxcount = count;
    txcount = count;
    rxrecived = 0;
#ifdef DMA_BASE
	if (com_mode & COM_MODE_DMA){
        DMA0CTL= 0;
        DMA1CTL = 0;
	    /* Toggle USCI reset mode to flush TX pipe */
	    UCB0CTL1 |= UCSWRST;
        UCB0CTL1 &= ~UCSWRST;
		// RXIFG
		__data16_write_addr((unsigned short)(DMA0DA),(unsigned long)rxbuf); 
		DMA0SZ  = count;   
		DMA0CTL = DMADT_0 + DMADSTINCR_3 + DMASBDB + DMALEVEL + DMAEN;

		//TXIFG;
		__data16_write_addr((unsigned short)(DMA1SA),(unsigned long)txbuf);
		DMA1SZ  = count;   
		DMA1CTL = DMADT_0 + DMASRCINCR_3 + DMASBDB + DMALEVEL + DMAEN;
	}
	else
#endif
	{
		rxptr = rxbuf;
		txptr = txbuf;
		com_mode &= ~COM_MODE_RX;
		while ((UCB0IFG & UCTXIFG) && txcount)
		{
			*(&(UCB0TXBUF)) = *txptr++;  /* put in first character */
			txcount--;
		}
		UCB0IE |= UCRXIE;  /* need to receive data to transmit */
	}
}

/**
 * spi_slave_receive() - send a bytes.
 */
void spi_slave_receive(uint8_t *buf, uint16_t count)
{
    rxcount = count;
    txcount = count;
    rxrecived = 0;
#ifdef DMA_BASE
    if (com_mode & COM_MODE_DMA){
        DMA0CTL = 0;
        DMA1CTL = 0;
        /* Toggle USCI reset mode to flush TX pipe */
        UCB0CTL1 |= UCSWRST;
        UCB0CTL1 &= ~UCSWRST;
        // RXIFG
        __data16_write_addr((unsigned short)(DMA0DA),(unsigned long)buf);
        DMA0SZ  = count;
        DMA0CTL = DMADT_0 + DMADSTINCR_3 + DMASBDB + DMALEVEL + DMAEN;

        //TXIFG;
        __data16_write_addr((unsigned short)(DMA1SA),(unsigned long)&dummy);
        DMA1SZ  = count;
        DMA1CTL = DMADT_0 + DMASBDB + DMALEVEL + DMAEN;
    }
    else
#endif
    {
        rxptr = buf;
        txptr = (uint8_t *) &dummy;
        com_mode |= COM_MODE_RX;
        while ((UCB0IFG & UCTXIFG) )
        {
            *(&(UCB0TXBUF)) = dummy;  /* put in first characters */
        }
        UCB0IE |= UCRXIE;  /* need to receive data to transmit */
    }
}

int spi_bytes_to_transmit(void)
{
#ifdef DMA_BASE
    // when DMA enabled return DMAxSZ else done return 0
    return ((DMA1CTL & DMAEN) ? DMA1SZ : 0);
#else
    return (txcount);
#endif        
}


int spi_bytes_received(void)
{
#ifdef DMA_BASE
    // when DMA enabled return DMAxSZ else done return 0
    if (com_mode & COM_MODE_DMA){
        return ((DMA0CTL & DMAEN) ? (rxcount - DMA0SZ) : rxcount);
    }else{
        return (rxrecived);
    }
#else
    return (rxrecived);
#endif        
}


int spi_data_done(void)
{
#ifdef DMA_BASE
        return (!(DMA0CTL & DMAEN));
#else
        return (rxcount == 0);
#endif        
}


void spi_rx_isr(uint8_t offset)
{
	uint8_t temp;
    temp = *txptr; // store in case tx and rx ptr are identical
	if (rxcount){
		if (rxptr != 0){
			*rxptr++ = *(&(UCB0RXBUF));
	        rxcount--;
	        rxrecived++;
		}
	}else{
	    UCB0IE &= ~UCRXIE;  /* disable interrupt */
	}
    if (txcount){
        if (txptr != 0){
            *(&(UCB0TXBUF)) = temp;
            if ((com_mode & COM_MODE_RX) == 0){
                txptr++;
            }
            txcount--;
        }
    }
	
}

#endif
	
