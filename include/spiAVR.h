#ifndef SPIAVR_H
#define SPIAVR_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// B5 should always be SCK(spi clock) and B3 should always be MOSI. If you are using an
// SPI peripheral that sends data back to the arduino, you will need to use B4 as the MISO pin.
// The SS pin can be any digital pin on the arduino. Right before sending an 8 bit value with
// the SPI_SEND() funtion, you will need to set your SS pin to low. If you have multiple SPI
// devices, they will share the SCK, MOSI and MISO pins but should have different SS pins.
// To send a value to a specific device, set it's SS pin to low and all other SS pins to high.

// Outputs, pin definitions
#define PIN_SCK PORTB5  // SHOULD ALWAYS BE B5 ON THE ARDUINO
#define PIN_MOSI PORTB3 // SHOULD ALWAYS BE B3 ON THE ARDUINO
#define PIN_SS PORTB2

// If SS is on a different port, make sure to change the init to take that into account.
void SPI_INIT()
{
    DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS); // initialize your pins.,
    SPCR |= (1 << SPE) | (1 << MSTR);                         // initialize SPI coomunication
}

void SPI_SEND(char data)
{
    SPDR = data; // set data that you want to transmit
    while (!(SPSR & (1 << SPIF)))
        ; // wait until done transmitting
}

void MAX7219_INIT()
{
    // initialize to no decode
    PORTB = SetBit(PORTB, PIN_SS, 0);
    SPI_SEND(0x09);
    SPI_SEND(0x00);
    PORTB = SetBit(PORTB, PIN_SS, 1);

    // set brightness
    PORTB = SetBit(PORTB, PIN_SS, 0);
    SPI_SEND(0x0A);
    SPI_SEND(0x04);
    PORTB = SetBit(PORTB, PIN_SS, 1);

    // set scan limit
    PORTB = SetBit(PORTB, PIN_SS, 0);
    SPI_SEND(0x0B);
    SPI_SEND(0x07);
    PORTB = SetBit(PORTB, PIN_SS, 1);

    // normal operation
    PORTB = SetBit(PORTB, PIN_SS, 0);
    SPI_SEND(0x0C);
    SPI_SEND(0x01);
    PORTB = SetBit(PORTB, PIN_SS, 1);

    // clear all rows
    for (unsigned char i = 1; i <= 8; i++)
    {
        PORTB = SetBit(PORTB, PIN_SS, 0);
        SPI_SEND(i);
        SPI_SEND(0x00);
        PORTB = SetBit(PORTB, PIN_SS, 1);
    }
}

#endif /* SPIAVR_H */