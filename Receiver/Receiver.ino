#include "nRF24L01.hpp"
#include "ArduinoInterface.hpp"

nRF24L01::Controller<nRF24L01::ArduinoInterface> *n;

#include <U8x8lib.h>
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

template <typename T> void printBits(T in) {
    unsigned char s = sizeof(T) * 8;
    Serial.print("0b");
    for (signed char g = s - 1; g >= 0; g--) {
        Serial.print( ((in & (1 << ((unsigned char) g))) > 0) ? "1" : "0");
    }
    Serial.println("");
}

template <typename T> void printBitsu8x8(T in) {
    unsigned char s = sizeof(T) * 8;
    u8x8.print("0b");
    for (signed char g = s - 1; g >= 0; g--) {
        u8x8.print( ((in & (1 << ((unsigned char) g))) > 0) ? "1" : "0");
    }
}

volatile byte lastPacketSize = 0;
volatile unsigned char dataOut[32];
volatile unsigned long bytesTransferred = 0;
volatile unsigned long lastMillis = millis();
void readData() {
    unsigned char packetSize = n->getNextPacketSize();
    lastPacketSize = packetSize;
    n->readData((unsigned char *)dataOut, lastPacketSize);
    bytesTransferred += 32;
}
void nrfInterrupt() {
    // Get and clear the interrupt bits.
    n->readAndClearInterruptBits();
    if(n->didReceivePayload()) {
        readData();
    }    
}

void setup() {
    // put your setup code here, to run once:
    u8x8.begin();
    u8x8.setPowerSave(0);
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.drawString(0, 0, "Powering up...");
    
    attachInterrupt(digitalPinToInterrupt(2), nrfInterrupt, FALLING);

    n = new nRF24L01::Controller<nRF24L01::ArduinoInterface>(8, 2, 10);
    n->setPoweredUp(true);
    n->setPrimaryReceiver();
    unsigned char addr[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    n->setAddress(addr, 3);
    
    //n->setChannel(2);
    
    n->setAutoAcknowledgementEnabled(false);
    n->setUsesDynamicPayloadLength(false);
    n->setBitrate(2);
    n->setReceivedPacketLength(32);
    n->setCRCEnabled(false);

    n->readAndClearInterruptBits();
}
void loop() {
    // put your main code here, to run repeatedly:
    delay(250);
    
    lastMillis = millis();
    

    u8x8.clearLine(0);
    u8x8.setCursor(0, 0);
    //u8x8.print(  ((int)dataOut[0]) + (((int)dataOut[1]) << 8)  );
    u8x8.print(bytesTransferred);

    u8x8.clearLine(1);
    u8x8.setCursor(0, 1);
    u8x8.print("lm:");
    u8x8.print(lastMillis);

    u8x8.clearLine(2);
    u8x8.setCursor(0, 2);
    unsigned char stat = (n->getStatusAndConfigRegisters() >> 8);
    u8x8.print( (double) bytesTransferred / ( ((double) millis()) / 1000.0) );
    u8x8.clearLine(3);
    u8x8.setCursor(0, 3);
    printBitsu8x8(stat);

    u8x8.setCursor(0,4);
    u8x8.print("FIFO:");
    u8x8.clearLine(5);
    u8x8.setCursor(0,5);
    unsigned char fifo = n->getFIFOStatus();
    //unsigned char g = ADCSRA;
    printBitsu8x8(fifo);

    if(n->dataInRXFIFO()) {
        readData();
    }
    
    u8x8.clearLine(6);
    u8x8.setCursor(0,6);
    u8x8.print(lastPacketSize, DEC);
    
//    stat = stat >> 1;
//    u8x8.setCursor(0, 2);
//    u8x8.print(stat);
}
