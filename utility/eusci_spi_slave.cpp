/**
 * File: eusci_spi.c - msp430 USCI SPI implementation
 *
 * EUSCI flavor implementation by Robert Wessels <robertinant@yahoo.com>
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
#include "usci_isr_handler.h"

#if defined(__MSP430_HAS_EUSCI_B0__) || defined(DEFAULT_SPI)


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


#if defined(DEFAULT_SPI)
#if (DEFAULT_SPI == 0)
uint16_t SPI_baseAddress = UCB0_BASE;
#endif	
#if (DEFAULT_SPI == 1)
uint16_t SPI_baseAddress = UCB1_BASE;
#endif	
#if (DEFAULT_SPI == 2)
uint16_t SPI_baseAddress = UCB2_BASE;
#endif	
#if (DEFAULT_SPI == 3)
uint16_t SPI_baseAddress = UCB3_BASE;
#endif	
#if (DEFAULT_SPI == 10)
uint16_t SPI_baseAddress = UCA0_BASE;
#endif	
#if (DEFAULT_SPI == 11)
uint16_t SPI_baseAddress = UCA1_BASE;
#endif	
#if (DEFAULT_SPI == 12)
uint16_t SPI_baseAddress = UCA2_BASE;
#endif
#if (DEFAULT_SPI == 13)
uint16_t SPI_baseAddress = UCA3_BASE;
#endif	
#else
uint16_t SPI_baseAddress = UCB0_BASE;
#endif


#define UCzCTLW0     (*((volatile uint16_t *)((uint16_t)(OFS_UCBxCTLW0  + SPI_baseAddress))))
#define UCzCTL0      (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxCTL0   + SPI_baseAddress))))
#define UCzCTL1      (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxCTL1   + SPI_baseAddress))))
#define UCzBRW       (*((volatile uint16_t *)((uint16_t)(OFS_UCBxBRW    + SPI_baseAddress))))
#define UCzBR0       (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxBR0    + SPI_baseAddress))))
#define UCzBR1       (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxBR1    + SPI_baseAddress))))
#define UCzTXBUF     (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxTXBUF  + SPI_baseAddress))))
#define UCzRXBUF     (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxRXBUF  + SPI_baseAddress))))
#define UCzIFG       ( (spiModule < 10) ? (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxIFG    + SPI_baseAddress)))) : (*((volatile uint8_t *) ((uint16_t)(OFS_UCAxIFG    + SPI_baseAddress)))) )
#define UCzIE        ( (spiModule < 10) ? (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxIE     + SPI_baseAddress)))) : (*((volatile uint8_t *) ((uint16_t)(OFS_UCAxIE     + SPI_baseAddress)))) )

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

#define SPI_MODE_0 (UCCKPH)		/* CPOL=0 CPHA=0 */
#define SPI_MODE_1 (0)			/* CPOL=0 CPHA=1 */
#define SPI_MODE_2 (UCCKPL | UCCKPH)	/* CPOL=1 CPHA=0 */
#define SPI_MODE_3 (UCCKPL)		/* CPOL=1 CPHA=1 */

#define SPI_MODE_MASK (UCCKPL | UCCKPH)

uint8_t * rxptr;
uint8_t * txptr;
uint16_t rxcount = 0;
uint16_t txcount = 0;
uint8_t com_mode = 0; /* mode : 0 - RX and TX  1 - RX only, TX to dummy */
#define COM_MODE_RX  0x1
#define COM_MODE_DMA 0x2

const uint8_t dummy = 0xFF;

/**
 * spi_slave_initialize() - Configure USCI UCz for SPI mode
 *
 * P2.0 - CS (active low)
 * P1.5 - SCLK
 * P1.6 - MISO aka SOMI
 * P1.7 - MOSI aka SIMO
 *
 */
void spi_slave_initialize(const uint8_t mode, const uint8_t datamode)
{
    /* Calling this dummy function prevents the linker
     * from stripping the USCI interupt vectors.*/ 
    usci_isr_install();
	
	/* Put USCI in reset mode, source USCI clock from SMCLK. */
	UCzCTLW0 = UCSWRST;

	/* SPI in slave MODE 0 - CPOL=0 SPHA=0. - 3 wire STE */
	UCzCTLW0 |= UCMSB | UCSYNC;

    switch(mode) {
    case 0: /* 3 wire  */
        UCzCTLW0 |= UCMODE_0;
        break;
    case 1: /* 4 wire STE = 1 */
        UCzCTLW0 |= UCMODE_1;
        break;
    case 2: /* 4 wire STE = 0 */
        UCzCTLW0 |= UCMODE_2;
        break;
    default:
        break;
    }

    switch(datamode) {
    case 0: /* SPI_MODE0 */
        UCzCTLW0 |= SPI_MODE_0;
        break;
    case 1: /* SPI_MODE1 */
        UCzCTLW0 |= SPI_MODE_1;
        break;
    case 2: /* SPI_MODE2 */
        UCzCTLW0 |= SPI_MODE_2;
        break;
    case 3: /* SPI_MODE3 */
        UCzCTLW0 |= SPI_MODE_3;
        break;
    default:
        break;
    }
    com_mode = 0;
    com_mode = COM_MODE_DMA;
	/* Set pins to SPI mode. */
#if defined(DEFAULT_SPI)
#if defined(UCB0_BASE) && defined(SPISCK0_SET_MODE)
	if (SPI_baseAddress == UCB0_BASE) {
		pinMode_int(SCK0, SPISCK0_SET_MODE);
		pinMode_int(MOSI0, SPIMOSI0_SET_MODE);
		pinMode_int(MISO0, SPIMISO0_SET_MODE);
		if (mode > 0){
			pinMode_int(SS0, SPISCK0_SET_MODE);  /* SPISS0_SET_MODE is not defined in pins_enegia.h so hope this has the same */
		}
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCB0RXIFG) && defined(DMA0TSEL__UCB0TXIFG)
		if (com_mode & COM_MODE_DMA){
			DMACTL0 = DMA0TSEL__UCB0RXIFG;
			__data16_write_addr((unsigned short) &DMA0SA,(unsigned long)&UCB0RXBUF);
			//__data16_write_addr((unsigned short) &DMA0DA,(unsigned long)BUF); 
			//DMA0SZ = xxx;   
			//DMA0CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN + DMAIE;

			DMACTL0 = DMA1TSEL__UCB0TXIFG;
			//__data16_write_addr((unsigned short) &DMA1SA,(unsigned long)BUF);
			__data16_write_addr((unsigned short) &DMA1DA,(unsigned long)&UCB0TXBUF); 
			//DMA1SZ = xxx;   
			//DMA1CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN;
		}
#endif
	}
#endif	
#if defined(UCB1_BASE) && defined(SPISCK1_SET_MODE)
	if (SPI_baseAddress == UCB1_BASE) {
		pinMode_int(SCK1, SPISCK1_SET_MODE);
		pinMode_int(MOSI1, SPIMOSI1_SET_MODE);
		pinMode_int(MISO1, SPIMISO1_SET_MODE);
		if (mode > 0){
			pinMode_int(SS1, SPISCK1_SET_MODE);  /* SPISS1_SET_MODE is not defined in pins_enegia.h so hope this has the same */
		}
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCB1RXIFG) && defined(DMA0TSEL__UCB1TXIFG)
		if (com_mode & COM_MODE_DMA){
			DMACTL0 = DMA0TSEL__UCB1RXIFG;
			__data16_write_addr((unsigned short) &DMA0SA,(unsigned long)&UCB1RXBUF);
			//__data16_write_addr((unsigned short) &DMA0DA,(unsigned long)BUF); 
			//DMA0SZ = xxx;   
			//DMA0CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN + DMAIE;

			DMACTL0 = DMA1TSEL__UCB1TXIFG;
			//__data16_write_addr((unsigned short) &DMA1SA,(unsigned long)BUF);
			__data16_write_addr((unsigned short) &DMA1DA,(unsigned long)&UCB1TXBUF); 
			//DMA1SZ = xxx;   
			//DMA1CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN;
		}
#endif
	}
#endif	
#if defined(UCB2_BASE) && defined(SPISCK2_SET_MODE)
	if (SPI_baseAddress == UCB2_BASE) {
		pinMode_int(SCK2, SPISCK2_SET_MODE);
		pinMode_int(MOSI2, SPIMOSI2_SET_MODE);
		pinMode_int(MISO2, SPIMISO2_SET_MODE);
		if (mode > 0){
			pinMode_int(SS2, SPISCK2_SET_MODE);  /* SPISS2_SET_MODE is not defined in pins_enegia.h so hope this has the same */
		}
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCB2RXIFG) && defined(DMA0TSEL__UCB2TXIFG)
		if (com_mode & COM_MODE_DMA){
			DMACTL0 = DMB2TSEL__UCA0RXIFG;
			__data16_write_addr((unsigned short) &DMA0SA,(unsigned long)&UCB2RXBUF);
			//__data16_write_addr((unsigned short) &DMA0DA,(unsigned long)BUF); 
			//DMA0SZ = xxx;   
			//DMA0CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN + DMAIE;

			DMACTL0 = DMA1TSEL__UCB2TXIFG;
			//__data16_write_addr((unsigned short) &DMA1SA,(unsigned long)BUF);
			__data16_write_addr((unsigned short) &DMA1DA,(unsigned long)&UCB2TXBUF); 
			//DMA1SZ = xxx;   
			//DMA1CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN;
		}
#endif
	}
#endif	
#if defined(UCB3_BASE) && defined(SPISCK3_SET_MODE)
	if (SPI_baseAddress == UCB3_BASE) {
		pinMode_int(SCK3, SPISCK3_SET_MODE);
		pinMode_int(MOSI3, SPIMOSI3_SET_MODE);
		pinMode_int(MISO3, SPIMISO3_SET_MODE);
		if (mode > 0){
			pinMode_int(SS3, SPISCK3_SET_MODE);  /* SPISS3_SET_MODE is not defined in pins_enegia.h so hope this has the same */
		}
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCB3RXIFG) && defined(DMA0TSEL__UCB3TXIFG)
		if (com_mode & COM_MODE_DMA){
			DMACTL0 = DMA0TSEL__UCB3RXIFG;
			__data16_write_addr((unsigned short) &DMA0SA,(unsigned long)&UCB3RXBUF);
			//__data16_write_addr((unsigned short) &DMA0DA,(unsigned long)BUF); 
			//DMA0SZ = xxx;   
			//DMA0CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN + DMAIE;

			DMACTL0 = DMA1TSEL__UCB3TXIFG;
			//__data16_write_addr((unsigned short) &DMA1SA,(unsigned long)BUF);
			__data16_write_addr((unsigned short) &DMA1DA,(unsigned long)&UCB3TXBUF); 
			//DMA1SZ = xxx;   
			//DMA1CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN;
		}
#endif
	}
#endif	
#if defined(UCA0_BASE) && defined(SPISCK10_SET_MODE)
	if (SPI_baseAddress == UCA0_BASE) {
		pinMode_int(SCK10, SPISCK10_SET_MODE);
		pinMode_int(MOSI10, SPIMOSI10_SET_MODE);
		pinMode_int(MISO10, SPIMISO10_SET_MODE);
		if (mode > 0){
			pinMode_int(SS10, SPISCK10_SET_MODE);  /* SPISS10_SET_MODE is not defined in pins_enegia.h so hope this has the same */
		}
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCA0RXIFG) && defined(DMA0TSEL__UCA0TXIFG)
		if (com_mode & COM_MODE_DMA){
			DMACTL0 = DMA0TSEL__UCA0RXIFG;
			__data16_write_addr((unsigned short) &DMA0SA,(unsigned long)&UCA0RXBUF);
			//__data16_write_addr((unsigned short) &DMA0DA,(unsigned long)BUF); 
			//DMA0SZ = xxx;   
			//DMA0CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN + DMAIE;

			DMACTL0 = DMA1TSEL__UCA0TXIFG;
			//__data16_write_addr((unsigned short) &DMA1SA,(unsigned long)BUF);
			__data16_write_addr((unsigned short) &DMA1DA,(unsigned long)&UCA0TXBUF); 
			//DMA1SZ = xxx;   
			//DMA1CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN;
		}
#endif
	}
#endif	
#if defined(UCA1_BASE) && defined(SPISCK11_SET_MODE)
	if (SPI_baseAddress == UCA1_BASE) {
		pinMode_int(SCK11, SPISCK11_SET_MODE);
		pinMode_int(MOSI11, SPIMOSI11_SET_MODE);
		pinMode_int(MISO11, SPIMISO11_SET_MODE);
		if (mode > 0){
			pinMode_int(SS11, SPISCK11_SET_MODE);  /* SPISS11_SET_MODE is not defined in pins_enegia.h so hope this has the same */
		}
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCA1RXIFG) && defined(DMA0TSEL__UCA1TXIFG)
		if (com_mode & COM_MODE_DMA){
			DMACTL0 = DMA1TSEL__UCA0RXIFG;
			__data16_write_addr((unsigned short) &DMA0SA,(unsigned long)&UCA1RXBUF);
			//__data16_write_addr((unsigned short) &DMA0DA,(unsigned long)BUF); 
			//DMA0SZ = xxx;   
			//DMA0CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN + DMAIE;

			DMACTL0 = DMA1TSEL__UCA1TXIFG;
			//__data16_write_addr((unsigned short) &DMA1SA,(unsigned long)BUF);
			__data16_write_addr((unsigned short) &DMA1DA,(unsigned long)&UCA1TXBUF); 
			//DMA1SZ = xxx;   
			//DMA1CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN;
		}
#endif
	}
#endif	
#if defined(UCA2_BASE) && defined(SPISCK12_SET_MODE)
	if (SPI_baseAddress == UCA2_BASE) {
		pinMode_int(SCK12, SPISCK12_SET_MODE);
		pinMode_int(MOSI12, SPIMOSI12_SET_MODE);
		pinMode_int(MISO12, SPIMISO12_SET_MODE);
		if (mode > 0){
			pinMode_int(SS12, SPISCK12_SET_MODE);  /* SPISS12_SET_MODE is not defined in pins_enegia.h so hope this has the same */
		}
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCA2RXIFG) && defined(DMA0TSEL__UCA2TXIFG)
		if (com_mode & COM_MODE_DMA){
			DMACTL0 = DMA1TSEL__UCA2RXIFG;
			__data16_write_addr((unsigned short) &DMA0SA,(unsigned long)&UCA2RXBUF);
			//__data16_write_addr((unsigned short) &DMA0DA,(unsigned long)BUF); 
			//DMA0SZ = xxx;   
			//DMA0CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN + DMAIE;

			DMACTL0 = DMA1TSEL__UCA20TXIFG;
			//__data16_write_addr((unsigned short) &DMA1SA,(unsigned long)BUF);
			__data16_write_addr((unsigned short) &DMA1DA,(unsigned long)&UCA2TXBUF); 
			//DMA1SZ = xxx;   
			//DMA1CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN;
		}
#endif
	}
#endif	
#if defined(UCA3_BASE) && defined(SPISCK13_SET_MODE)
	if (SPI_baseAddress == UCA3_BASE) {
		pinMode_int(SCK13, SPISCK13_SET_MODE);
		pinMode_int(MOSI13, SPIMOSI13_SET_MODE);
		pinMode_int(MISO13, SPIMISO13_SET_MODE);
		if (mode > 0){
			pinMode_int(SS13, SPISCK13_SET_MODE);  /* SPISS13_SET_MODE is not defined in pins_enegia.h so hope this has the same */
		}
	}
#endif	
#else
	pinMode_int(SCK, SPISCK_SET_MODE);
	pinMode_int(MOSI, SPIMOSI_SET_MODE);
	pinMode_int(MISO, SPIMISO_SET_MODE);
	if (mode > 0){
		pinMode_int(SS, SPISCK_SET_MODE); 
	}
#ifdef __MSP430_HAS_DMA__
		if (com_mode & COM_MODE_DMA){
			DMACTL0 = DMA0TSEL__UCA0RXIFG;
			__data16_write_addr((unsigned short) &DMA0SA,(unsigned long)&UCA0RXBUF);
			//__data16_write_addr((unsigned short) &DMA0DA,(unsigned long)BUF); 
			//DMA0SZ = xxx;   
			//DMA0CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN + DMAIE;

			DMACTL0 = DMA1TSEL__UCA0TXIFG;
			//__data16_write_addr((unsigned short) &DMA1SA,(unsigned long)BUF);
			__data16_write_addr((unsigned short) &DMA1DA,(unsigned long)&UCA0TXBUF); 
			//DMA1SZ = xxx;   
			//DMA1CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN;
		}
#endif
#endif
#if defined(__MSP430_HAS_DMA__) && defined(DMA3TSEL__UCA3RXIFG) && defined(DMA4TSEL__UCA3TXIFG)
        if (com_mode & COM_MODE_DMA){
            DMACTL1 = DMA3TSEL__UCA3RXIFG;
            __data16_write_addr((unsigned short) &DMA3SA,(unsigned long)&UCA3RXBUF);
            //__data16_write_addr((unsigned short) &DMA0DA,(unsigned long)BUF); 
            //DMA0SZ = xxx;   
            //DMA0CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN + DMAIE;

            DMACTL2 = DMA4TSEL__UCA3TXIFG;
            //__data16_write_addr((unsigned short) &DMA1SA,(unsigned long)BUF);
            __data16_write_addr((unsigned short) &DMA4DA,(unsigned long)&UCA3TXBUF); 
            //DMA1SZ = xxx;   
            //DMA1CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN;
        }
#endif
		


	/* Release USCI for operation. */
	UCzCTLW0 &= ~UCSWRST;
}

/**
 * spi_slave_disable() - put USCI into reset mode.
 */
void spi_disable(void)
{
	/* Put USCI in reset mode. */
	UCzCTLW0 |= UCSWRST;
}

/**
 * spi_slave_transfer() - send a bytes and recv response.
 */

void spi_slave_transfer(uint8_t *rxbuf, uint8_t *txbuf, uint16_t count)
{
#ifdef __MSP430_HAS_DMA__
	if (com_mode & COM_MODE_DMA){
		// RXIFG
		__data16_write_addr((unsigned short) &DMA3DA,(unsigned long)rxbuf); 
		DMA3SZ = count;   
		DMA3CTL = DMADT_0 + DMADSTINCR + DMASBDB + DMAEN;

		//TXIFG;
		__data16_write_addr((unsigned short) &DMA4SA,(unsigned long)txbuf);
		DMA4SZ = count;   
		DMA4CTL = DMADT_0 + DMASRCINCR + DMASBDB + DMAEN + DMALEVEL;
	}
	else
#endif
	{
		rxptr = rxbuf;
		txptr = txbuf;
		com_mode &= ~COM_MODE_RX;
		rxcount = count;
		txcount = count;
		while ((UCzIFG & UCTXIFG) && txcount)
		{
			*(&(UCzTXBUF)) = *txptr++;  /* put in first character */
			txcount--;
		}
		UCzIE |= UCRXIE;  /* need to receive data to transmit */
	}
}

/**
 * spi_slave_receive() - send a bytes.
 */
void spi_slave_receive(uint8_t *buf, uint16_t count)
{
    rxptr = buf;
    txptr = (uint8_t *) &dummy;
	rxcount = count;
    txcount = count;
	com_mode |= COM_MODE_RX;
    while ((UCzIFG & UCTXIFG) )
    {
        *(&(UCzTXBUF)) = dummy;  /* put in first characters */
    }
	UCzIE |= UCRXIE;  /* need to receive data to transmit */
}

int spi_bytes_to_transmit(void)
{
#ifdef __MSP430_HAS_DMA__
        return (DMA3SZ);
#else
        return (!(DMA3CTL & DMAEN) ? 0 : rxcount);
#endif        
}

int spi_data_done(void)
{
#ifdef __MSP430_HAS_DMA__
        return (!(DMA3CTL & DMAEN));
#else
        return (rxcount == 0);
#endif        
}


/**
 * spi_slave_set_bitorder(LSBFIRST=0 | MSBFIRST=1).
 */
void spi_set_bitorder(const uint8_t order)
{
	/* Hold UCz in reset. */
	UCzCTLW0 |= UCSWRST;

	UCzCTLW0 = (UCzCTLW0 & ~UCMSB) | ((order == 1 /*MSBFIRST*/) ? UCMSB : 0); /* MSBFIRST = 1 */

	/* Release for operation. */
	UCzCTLW0 &= ~UCSWRST;
}

void spi_rx_isr(uint8_t offset)
{
	uint8_t temp;
    temp = *txptr; // store in case tx and rx ptr are identical
	if (rxcount){
		if (rxptr != 0){
			*rxptr++ = *(&(UCzRXBUF));
	        rxcount--;
		}
	}else{
	    UCzIE &= ~UCRXIE;  /* disable interrupt */
	}
    if (txcount){
        if (txptr != 0){
            *(&(UCzTXBUF)) = temp;
            if ((com_mode & COM_MODE_RX) == 0){
                *txptr++;
            }
            txcount--;
        }
    }
	
}


#endif
	
