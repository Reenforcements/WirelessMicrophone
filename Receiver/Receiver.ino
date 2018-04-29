#include "nRF24L01.hpp"
#include "FastInterface.hpp"
#include "ArduinoInterface.hpp"
#include "SPI.h"
#include <U8x8lib.h>

nRF24L01::Controller<FastInterface> *n;
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

const byte MCP_CSN = 4;
int MCPtoTransfer = 0;
void changeValue(byte val) {
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    PORTD ^= (1 << 4);
    MCPtoTransfer = 0x00FF & val;
    SPI.transfer16(MCPtoTransfer);
    PORTD ^= (1 << 4);

    SPI.endTransaction();
}

void wirelessMic_setupTimer1() {
    // Setup timer 1!
    PRR = PRR & (~(1 << 3)); // Enable timer 1
    TCCR1A = 0b00000000; // CTC mode
    TCCR1B = TCCR1B & 0b00000000; // No prescaler, CTC Mode, noise canceller disabled
    TCCR1B |= 0b01001;// NORMAL MODE unless 01001 THEN IT WILL BE CTC

    // 363 or 362 for 44,077 Hz
    unsigned int timerACompare = 500;
    byte upper = ((timerACompare & 0xFF00) >> 8);
    byte lower = timerACompare & 0xFF;

    // The value for A will reset the counter
    OCR1AH = upper;
    OCR1AL = lower;
    // Same value for B because that triggers the ADC
    //OCR1BH = upper;
    //OCR1BL = lower;

    // Enable output compare A
    TIMSK1 = (1 << 1);
}




bool first = true;
volatile unsigned int writtenCount = 0;
volatile byte lastPacketSize = 0;
volatile unsigned char dataOut[64];
volatile unsigned long bytesTransferred = 0;
volatile unsigned long lastMillis = millis();
volatile byte lastWritten = 0;
void writeAValue() {
    changeValue(dataOut[0b00111111 & lastWritten]);

    lastWritten++;
    if (lastWritten >= 63)
        lastWritten = 0;

    writtenCount++;
}
void readData() {
    unsigned char packetSize = n->getNextPacketSize();

    if (first) {
        n->readData((unsigned char *)(dataOut), packetSize);
    }
    else {
        n->readData((unsigned char *)(dataOut + 32), packetSize);
    }
    first = !first;

    bytesTransferred += 32;
    writeAValue();
}
//byte lastWritten = 0;

ISR(TIMER1_COMPA_vect) {
    writeAValue();
}
void nrfInterrupt() {
    // Get and clear the interrupt bits.
    n->readAndClearInterruptBits();
    if (n->didReceivePayload()) {
        readData();
    }
}

void setup() {
    // Set up the OLED display
    u8x8.begin();
    u8x8.setPowerSave(0);
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.drawString(0, 0, "Powering up...");

    // nRF24L01+ setup
    attachInterrupt(digitalPinToInterrupt(2), nrfInterrupt, FALLING);

    n = new nRF24L01::Controller<FastInterface>(8, 2, 10);
    n->setPoweredUp(true);
    n->setPrimaryReceiver();
    unsigned char addr[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    n->setAddress(addr, 3);

    //n->setChannel(2);

    n->setAutoAcknowledgementEnabled(false);
    n->setUsesDynamicPayloadLength(false);
    n->setBitrate(2);
    n->setReceivedPacketLength(32);
    n->setCRCEnabled(true);

    n->readAndClearInterruptBits();

    // MCP Digital pot setup
    pinMode(MCP_CSN, OUTPUT);
    digitalWrite(MCP_CSN, HIGH);
    // SPI should already be enabled from transceiver setup
    //Serial.begin(57600);
    for (byte i = 0; i < 32; i++) {
        dataOut[i] = 0;
    }
    wirelessMic_setupTimer1();
    Serial.begin(57600);
    changeValue(0);
}

long lastMeasureReset = 2000;
long lastPrint = millis();
void loop() {
    
    //Serial.println(dataOut[lastWritten]);
    //if (millis() > (lastPrint + 1000)) {
    //lastPrint = millis();
    Serial.print("Bytes read: ");
    Serial.print(bytesTransferred);
    Serial.print(" Bytes output:");
    Serial.println(writtenCount);
    //
    //        if (Serial.available() >= 2) {
    //
    //            unsigned int timerACompare = Serial.parseInt();
    //            byte upper = ((timerACompare & 0xFF00) >> 8);
    //            byte lower = timerACompare & 0xFF;
    //
    //            // The value for A will reset the counter
    //            OCR1AH = upper;
    //            OCR1AL = lower;
    //        }
    bytesTransferred = 0;
    writtenCount = 0;
    //}
    delay(1000);
    return;
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
    if ( millis() > (lastMeasureReset + 3000) ) {
        bytesTransferred = 0;
        lastMeasureReset = millis();
    }
    u8x8.print( (double) bytesTransferred /
                ( ((double) (millis() - lastMeasureReset)) / 1000.0) );
    u8x8.clearLine(3);
    u8x8.setCursor(0, 3);
    printBitsu8x8(stat);

    u8x8.setCursor(0, 4);
    u8x8.print("FIFO:");
    u8x8.clearLine(5);
    u8x8.setCursor(0, 5);
    unsigned char fifo = n->getFIFOStatus();
    //unsigned char g = ADCSRA;
    printBitsu8x8(fifo);

    if (n->dataInRXFIFO()) {
        readData();
    }

    u8x8.clearLine(6);
    u8x8.setCursor(0, 6);
    u8x8.print(lastPacketSize, DEC);

    //    stat = stat >> 1;
    //    u8x8.setCursor(0, 2);
    //    u8x8.print(stat);
}
