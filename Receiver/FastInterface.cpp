#include <Arduino.h>
#include "FastInterface.hpp"
#include <SPI.h>



void FastInterface::begin() {
    SPI.begin();
    // Convert the pin number to the interrupt
    SPI.usingInterrupt( digitalPinToInterrupt(_IRQPin) );
    pinMode(_CSNPin, OUTPUT);
    digitalWrite(_CSNPin, HIGH);
}
void FastInterface::end() {
    SPI.end();
}

void FastInterface::beginTransaction() {
    /*
        Up to 10 Mbps, most significant bits first, clock pulses high for writing/reading and changes data on the trailing edge of each clock cycle.
    */
    SPI.beginTransaction( SPISettings(10000000, MSBFIRST, SPI_MODE0) );
    writeCSNLow();
}
void FastInterface::endTransaction() {
    writeCSNHigh();
    SPI.endTransaction();
}
unsigned char FastInterface::transferByte(unsigned char b) {
    unsigned char result = SPI.transfer(b);
    return result;
}
void FastInterface::transferBytes(unsigned char **b, unsigned char size) {
    SPI.transfer(*b, size);
}
void FastInterface::delay(unsigned int d) {
    ::delay(d);
}
void FastInterface::delayMicroseconds(unsigned int d) {
    ::delayMicroseconds(d);
}

inline void FastInterface::writeCSNHigh() {
    //digitalWrite(_CSNPin, HIGH);
    PORTB |= (1 << 2);
}
inline void FastInterface::writeCSNLow() {
    //digitalWrite(_CSNPin, LOW);
    PORTB ^= (1 << 2);
}
inline void FastInterface::writeCEHigh() {
    //digitalWrite(_CEPin, HIGH);
    PORTB |= 1;
}
inline void FastInterface::writeCELow() {
    //digitalWrite(_CEPin, LOW);
    PORTB ^= 1;
}


