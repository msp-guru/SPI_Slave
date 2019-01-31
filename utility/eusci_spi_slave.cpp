/**
    File: eusci_spi_slave.c - msp430 eUSCI SPI Slave implementation

    EUSCI flavor implementation by StefanSch
    based on:
    Copyright (c) 2012 by Rick Kimball <rick@kimballsoftware.com>
    spi slave abstraction api for msp430

    This file is free software; you can redistribute it and/or modify
    it under the terms of either the GNU General Public License version 2
    or the GNU Lesser General Public License version 2.1, both as
    published by the Free Software Foundation.

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
uint16_t SPI_slave_baseAddress = UCB0_BASE;
#endif
#if (DEFAULT_SPI == 1)
uint16_t SPI_slave_baseAddress = UCB1_BASE;
#endif
#if (DEFAULT_SPI == 2)
uint16_t SPI_slave_baseAddress = UCB2_BASE;
#endif
#if (DEFAULT_SPI == 3)
uint16_t SPI_slave_baseAddress = UCB3_BASE;
#endif
#if (DEFAULT_SPI == 10)
uint16_t SPI_slave_baseAddress = UCA0_BASE;
#endif
#if (DEFAULT_SPI == 11)
uint16_t SPI_slave_baseAddress = UCA1_BASE;
#endif
#if (DEFAULT_SPI == 12)
uint16_t SPI_slave_baseAddress = UCA2_BASE;
#endif
#if (DEFAULT_SPI == 13)
uint16_t SPI_slave_baseAddress = UCA3_BASE;
#endif
#else
uint16_t SPI_slave_baseAddress = UCB0_BASE;
#endif


#define UCzCTLW0     (*((volatile uint16_t *)((uint16_t)(OFS_UCBxCTLW0  + SPI_slave_baseAddress))))
#define UCzCTL0      (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxCTL0   + SPI_slave_baseAddress))))
#define UCzCTL1      (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxCTL1   + SPI_slave_baseAddress))))
#define UCzBRW       (*((volatile uint16_t *)((uint16_t)(OFS_UCBxBRW    + SPI_slave_baseAddress))))
#define UCzBR0       (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxBR0    + SPI_slave_baseAddress))))
#define UCzBR1       (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxBR1    + SPI_slave_baseAddress))))
#define UCzSTATW     ( (spiSlaveModule < 10) ? (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxSTATW  + SPI_slave_baseAddress)))) : (*((volatile uint8_t *) ((uint16_t)(OFS_UCAxSTATW  + SPI_slave_baseAddress)))) )
#define UCzTXBUF     (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxTXBUF  + SPI_slave_baseAddress))))
#define UCzRXBUF     (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxRXBUF  + SPI_slave_baseAddress))))
#define UCzIFG       ( (spiSlaveModule < 10) ? (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxIFG    + SPI_slave_baseAddress)))) : (*((volatile uint8_t *) ((uint16_t)(OFS_UCAxIFG    + SPI_slave_baseAddress)))) )
#define UCzIE        ( (spiSlaveModule < 10) ? (*((volatile uint8_t *) ((uint16_t)(OFS_UCBxIE     + SPI_slave_baseAddress)))) : (*((volatile uint8_t *) ((uint16_t)(OFS_UCAxIE     + SPI_slave_baseAddress)))) )

#ifndef __data16_write_addr
#define __data16_write_addr(x,y) *(unsigned long int*)(x) = y
#endif
#ifndef HWREG16
#define HWREG16(x)                                                             \
    (*((volatile uint16_t*)((uint16_t)x)))
#endif

#if defined(DEFAULT_SPI)
uint8_t spiSlaveModule = DEFAULT_SPI;
#else
uint8_t spiSlaveModule = 0;
#endif

/**
    USCI flags for various the SPI MODEs

    Note: The msp430 UCCKPL tracks the CPOL value. However,
    the UCCKPH flag is inverted when compared to the CPHA
    value described in Motorola documentation.
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
uint16_t rxrecived = 0;
uint8_t com_mode = 0; /* mode : 0 - RX and TX  1 - RX only, TX to dummy */
#define COM_MODE_RX  0x1
#define COM_MODE_DMA 0x2

uint8_t dma_idx = 0; /* index to DMA channel */

const uint8_t dummy = 0xFF;

/**
    spi_slave_initialize() - Configure USCI UCz for SPI mode

    Pxx - CS (active low)
    Pxx - SCLK
    Pxx - MISO aka SOMI
    Pxx - MOSI aka SIMO

*/

void spi_slave_initialize(const uint8_t mode, const uint8_t datamode, const uint8_t order)
{
    /*  Calling this dummy function prevents the linker
        from stripping the USCI interupt vectors.*/
    usci_isr_install();

    /* Put USCI in reset mode, source USCI clock from SMCLK. */
    UCzCTLW0 = UCSWRST;

    /* SPI in slave MODE 0 - CPOL=0 SPHA=0. - 3 wire STE */
    UCzCTLW0 |= UCSYNC;

    UCzCTLW0 = (UCzCTLW0 & ~UCMSB) | ((order == 1 /*MSBFIRST*/) ? UCMSB : 0); /* MSBFIRST = 1 */

    switch (mode)
    {
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

    switch (datamode)
    {
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
#if defined(__MSP430_HAS_DMA__)
    com_mode = COM_MODE_DMA;
#else
    com_mode = 0;
#endif
    /* Set pins to SPI mode. */
#if defined(DEFAULT_SPI)
#if defined(UCB0_BASE)
    if (SPI_slave_baseAddress == UCB0_BASE)
    {
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCB0RXIFG) && defined(DMA0TSEL__UCB0TXIFG)
        dma_idx = OFS_DMA0SA - OFS_DMA0SA;
        if (com_mode & COM_MODE_DMA)
        {
            DMACTL0 = (DMACTL0 & ~DMA0TSEL__DMAE0) | DMA0TSEL__UCB0RXIFG;

            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA0SA), (unsigned long)&UCB0RXBUF);

            DMACTL0 = (DMACTL0 & ~DMA1TSEL__DMAE0) | DMA1TSEL__UCB0TXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA1DA), (unsigned long)&UCB0TXBUF);
        }
#else
        com_mode &= ~COM_MODE_DMA;
#endif
    }
#endif
#if defined(UCB1_BASE)
    if (SPI_slave_baseAddress == UCB1_BASE)
    {
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCB1RXIFG) && defined(DMA0TSEL__UCB1TXIFG)
        if (com_mode & COM_MODE_DMA)
        {
            dma_idx = OFS_DMA0SA - OFS_DMA0SA;
            DMACTL0 = (DMACTL0 & ~DMA0TSEL__DMAE0) | DMA0TSEL__UCB1RXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA0SA), (unsigned long)&UCB1RXBUF);

            DMACTL0 = (DMACTL0 & ~DMA1TSEL__DMAE0) | DMA1TSEL__UCB1TXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA1DA), (unsigned long)&UCB1TXBUF);
        }
#else
        com_mode &= ~COM_MODE_DMA;
#endif
    }
#endif
#if defined(UCB2_BASE)
    if (SPI_slave_baseAddress == UCB2_BASE)
    {
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCB2RXIFG) && defined(DMA0TSEL__UCB2TXIFG)
        if (com_mode & COM_MODE_DMA)
        {
            dma_idx = OFS_DMA0SA - OFS_DMA0SA;
            DMACTL0 = (DMACTL0 & ~DMA0TSEL__DMAE0) | DMA0TSEL__UCB2RXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA0SA), (unsigned long)&UCB2RXBUF);

            DMACTL0 = (DMACTL0 & ~DMA1TSEL__DMAE0) | DMA1TSEL__UCB2TXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA1DA), (unsigned long)&UCB2TXBUF);
        }
#else
        com_mode &= ~COM_MODE_DMA;
#endif
    }
#endif
#if defined(UCB3_BASE)
    if (SPI_slave_baseAddress == UCB3_BASE)
    {
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCB3RXIFG) && defined(DMA0TSEL__UCB3TXIFG)
        if (com_mode & COM_MODE_DMA)
        {
            dma_idx = OFS_DMA0SA - OFS_DMA0SA;
            DMACTL0 = (DMACTL0 & ~DMA0TSEL__DMAE0) | DMA0TSEL__UCB3RXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA0SA), (unsigned long)&UCB3RXBUF);

            DMACTL0 = (DMACTL0 & ~DMA1TSEL__DMAE0) | DMA1TSEL__UCB3TXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA1DA), (unsigned long)&UCB3TXBUF);
        }
#else
        com_mode &= ~COM_MODE_DMA;
#endif
    }
#endif
#if defined(UCA0_BASE)
    if (SPI_slave_baseAddress == UCA0_BASE)
    {
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCA0RXIFG) && defined(DMA1TSEL__UCA0TXIFG)
        if (com_mode & COM_MODE_DMA)
        {
            dma_idx = OFS_DMA0SA - OFS_DMA0SA;
            DMACTL0 = (DMACTL0 & ~DMA0TSEL__DMAE0) | DMA0TSEL__UCA0RXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA0SA), (unsigned long)&UCA0RXBUF);

            DMACTL0 = (DMACTL0 & ~DMA1TSEL__DMAE0) | DMA1TSEL__UCA0TXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA1DA), (unsigned long)&UCA0TXBUF);
        }
#else
        com_mode &= ~COM_MODE_DMA;
#endif
    }
#endif
#if defined(UCA1_BASE)
    if (SPI_slave_baseAddress == UCA1_BASE)
    {
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCA1RXIFG) && defined(DMA1TSEL__UCA1TXIFG)
        if (com_mode & COM_MODE_DMA)
        {
            dma_idx = OFS_DMA0SA - OFS_DMA0SA;
            DMACTL0 = (DMACTL0 & ~DMA0TSEL__DMAE0) | DMA0TSEL__UCA0RXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA0SA), (unsigned long)&UCA1RXBUF);

            DMACTL0 = (DMACTL0 & ~DMA1TSEL__DMAE0) | DMA1TSEL__UCA1TXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA1DA), (unsigned long)&UCA1TXBUF);
        }
#else
        com_mode &= ~COM_MODE_DMA;
#endif
    }
#endif
#if defined(UCA2_BASE)
    if (SPI_slave_baseAddress == UCA2_BASE)
    {
#if defined(__MSP430_HAS_DMA__) && defined(DMA3TSEL__UCA2RXIFG) && defined(DMA4TSEL__UCA2TXIFG)
        if (com_mode & COM_MODE_DMA)
        {
            dma_idx = OFS_DMA3SA - OFS_DMA0SA;
            DMACTL1 = (DMACTL1 & ~DMA3TSEL__DMAE0) | DMA3TSEL__UCA2RXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA3SA), (unsigned long)&UCA2RXBUF);

            DMACTL2 = (DMACTL2 & ~DMA4TSEL__DMAE0) | DMA4TSEL__UCA2TXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA4DA), (unsigned long)&UCA2TXBUF);
        }
#else
        com_mode &= ~COM_MODE_DMA;
#endif
    }
#endif
#if defined(UCA3_BASE)
    if (SPI_slave_baseAddress == UCA3_BASE)
    {
#if defined(__MSP430_HAS_DMA__) && defined(DMA3TSEL__UCA3RXIFG) && defined(DMA4TSEL__UCA3TXIFG)
        if (com_mode & COM_MODE_DMA)
        {
            dma_idx = OFS_DMA3SA - OFS_DMA0SA;
            DMACTL1 = (DMACTL1 & ~DMA3TSEL__DMAE0) | DMA3TSEL__UCA3RXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA3SA), (unsigned long)&UCA3RXBUF);

            DMACTL2 = (DMACTL2 & ~DMA4TSEL__DMAE0) | DMA4TSEL__UCA3TXIFG;
            __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA4DA), (unsigned long)&UCA3TXBUF);
        }
#else
        com_mode &= ~COM_MODE_DMA;
#endif
    }
#endif
#else // #if defined(DEFAULT_SPI)
#if defined(__MSP430_HAS_DMA__) && defined(DMA0TSEL__UCA0RXIFG) && defined(DMA1TSEL__UCA0TXIFG)
    if (com_mode & COM_MODE_DMA)
    {
        dma_idx = OFS_DMA0SA - OFS_DMA0SA;
        DMACTL0 = (DMACTL0 & ~DMA0TSEL__DMAE0) | DMA0TSEL__UCA0RXIFG;
        __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA0SA), (unsigned long)&UCA0RXBUF);

        DMACTL0 = (DMACTL0 & ~DMA1TSEL__DMAE0) | DMA1TSEL__UCA0TXIFG;
        __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA1DA), (unsigned long)&UCA0TXBUF);
    }
#else
    com_mode &= ~COM_MODE_DMA;
#endif

#endif // #if defined(DEFAULT_SPI)




    /* Release USCI for operation. */
    UCzCTLW0 &= ~UCSWRST;
}

/**
    spi_slave_disable() - put USCI into reset mode.
*/
void spi_slave_disable(void)
{
    /* Wait for previous tx to complete. */
    while (UCzSTATW & UCBUSY);
    /* Put USCI in reset mode. */
    UCzCTLW0 |= UCSWRST;
}

/**
    spi_slave_transfer() - send a bytes and recv response.
*/

void spi_slave_transfer(uint8_t *rxbuf, uint8_t *txbuf, uint16_t count)
{
    rxcount = count;
    txcount = count;
    rxrecived = 0;
#ifdef __MSP430_HAS_DMA__
    if (com_mode & COM_MODE_DMA)
    {
        HWREG16(DMA_BASE + OFS_DMA0CTL + dma_idx) = 0;
        HWREG16(DMA_BASE + OFS_DMA1CTL + dma_idx) = 0;
        /* Toggle USCI reset mode to flush TX pipe */
        UCzCTLW0 |= UCSWRST;
        UCzCTLW0 &= ~UCSWRST;
        // RXIFG
        __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA0DA + dma_idx), (unsigned long)rxbuf);
        HWREG16(DMA_BASE + OFS_DMA0SZ  + dma_idx) = count;
        HWREG16(DMA_BASE + OFS_DMA0CTL + dma_idx) = DMADT_0 + DMADSTINCR + DMASBDB + DMALEVEL + DMAEN;

        //TXIFG;
        __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA1SA + dma_idx), (unsigned long)txbuf);
        HWREG16(DMA_BASE + OFS_DMA1SZ  + dma_idx) = count;
        HWREG16(DMA_BASE + OFS_DMA1CTL + dma_idx) = DMADT_0 + DMASRCINCR + DMASBDB + DMALEVEL + DMAEN;
    }
    else
#endif
    {
        rxptr = rxbuf;
        txptr = txbuf;
        com_mode &= ~COM_MODE_RX;
        while ((UCzIFG & UCTXIFG) && txcount)
        {
            *(&(UCzTXBUF)) = *txptr++;  /* put in first character */
            txcount--;
        }
        UCzIE |= UCRXIE;  /* need to receive data to transmit */
    }
}

/**
    spi_slave_receive() - send a bytes.
*/
void spi_slave_receive(uint8_t *buf, uint16_t count)
{
    rxcount = count;
    txcount = count;
    rxrecived = 0;
#ifdef __MSP430_HAS_DMA__
    if (com_mode & COM_MODE_DMA)
    {
        HWREG16(DMA_BASE + OFS_DMA0CTL + dma_idx) = 0;
        HWREG16(DMA_BASE + OFS_DMA1CTL + dma_idx) = 0;
        /* Toggle USCI reset mode to flush TX pipe */
        UCzCTLW0 |= UCSWRST;
        UCzCTLW0 &= ~UCSWRST;
        // RXIFG
        __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA0DA + dma_idx), (unsigned long)buf);
        HWREG16(DMA_BASE + OFS_DMA0SZ  + dma_idx) = count;
        HWREG16(DMA_BASE + OFS_DMA0CTL + dma_idx) = DMADT_0 + DMADSTINCR + DMASBDB + DMALEVEL + DMAEN;

        //TXIFG;
        __data16_write_addr((unsigned short)(DMA_BASE + OFS_DMA1SA + dma_idx), (unsigned long)&dummy);
        HWREG16(DMA_BASE + OFS_DMA1SZ  + dma_idx) = count;
        HWREG16(DMA_BASE + OFS_DMA1CTL + dma_idx) = DMADT_0 + DMASBDB + DMALEVEL + DMAEN;
    }
    else
#endif
    {
        rxptr = buf;
        txptr = (uint8_t *) &dummy;
        com_mode |= COM_MODE_RX;
        while ((UCzIFG & UCTXIFG))
        {
            *(&(UCzTXBUF)) = dummy;  /* put in first characters */
        }
        UCzIE |= UCRXIE;  /* need to receive data to transmit */
    }
}

int spi_bytes_to_transmit(void)
{
#ifdef __MSP430_HAS_DMA__
    // when DMA enabled return DMAxSZ else done return 0
    return ((HWREG16(DMA_BASE + OFS_DMA1CTL + dma_idx) & DMAEN) ? HWREG16(DMA_BASE + OFS_DMA1SZ + dma_idx) : 0);
#else
    return (txcount);
#endif
}


int spi_bytes_received(void)
{
#ifdef __MSP430_HAS_DMA__
    // when DMA enabled return DMAxSZ else done return 0
    if (com_mode & COM_MODE_DMA)
    {
        return ((HWREG16(DMA_BASE + OFS_DMA0CTL + dma_idx) & DMAEN) ? (rxcount - HWREG16(DMA_BASE + OFS_DMA0SZ + dma_idx)) : rxcount);
    }
    else
    {
        return (rxrecived);
    }
#else
    return (rxrecived);
#endif
}


int spi_data_done(void)
{
#ifdef __MSP430_HAS_DMA__
    return (!(HWREG16(DMA_BASE + OFS_DMA0CTL + dma_idx) & DMAEN));
#else
    return (rxcount == 0);
#endif
}


void spi_rx_isr(uint8_t offset)
{
    uint8_t temp;
    temp = *txptr; // store in case tx and rx ptr are identical
    if (rxcount)
    {
        if (rxptr != 0)
        {
            *rxptr++ = *(&(UCzRXBUF));
            rxcount--;
            rxrecived++;
        }
    }
    else
    {
        UCzIE &= ~UCRXIE;  /* disable interrupt */
    }
    if (txcount)
    {
        if (txptr != 0)
        {
            *(&(UCzTXBUF)) = temp;
            if ((com_mode & COM_MODE_RX) == 0)
            {
                *txptr++;
            }
            txcount--;
        }
    }

}


#endif // #if defined(DEFAULT_SPI)

