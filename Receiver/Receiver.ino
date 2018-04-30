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
inline void changeValue(byte val) {
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    PORTD &= ~(1 << 4);
    int MCPtoTransfer = 0x00FF & val;
    SPI.transfer16(MCPtoTransfer);
    PORTD |= (1 << 4);
    SPI.endTransaction();
}

void wirelessMic_setupTimer1() {
    // Setup timer 1!
    PRR = PRR & (~(1 << 3)); // Enable timer 1
    TCCR1A = 0b00000000; // CTC mode
    TCCR1B = TCCR1B & 0b00000000; // No prescaler, CTC Mode, noise cancel dis
    TCCR1B |= 0b01001;// NORMAL MODE unless 01001 THEN IT WILL BE CTC

    // 363 or 362 for 44,077 Hz
    unsigned int timerACompare = 800;
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




volatile unsigned long writtenCount = 0;
volatile byte lastPacketSize = 0;
volatile unsigned char currentPacket = 0;
volatile unsigned char dataOut[48];
volatile unsigned long bytesTransferred = 0;
volatile unsigned long lastMillis = millis();
volatile byte lastWritten = 0;
inline void writeAValue() {
    if (lastWritten >= 48)
        return;
        
    changeValue(dataOut[lastWritten]);

    lastWritten++;
    //if (lastWritten >= 48)
    //    lastWritten = 0;

    writtenCount++;
}
void readData() {
    unsigned char packetSize = n->getNextPacketSize();

    n->readData((unsigned char *)(dataOut + currentPacket), packetSize);
    currentPacket += 16;
    if (currentPacket == 48)
    {
        lastWritten = 0;
        currentPacket = 0;
        //TCNT1H = 0;
        //TCNT1L = 0;
    }

    bytesTransferred += 16;
    //writeAValue();
}

ISR(TIMER1_COMPA_vect) {
    writeAValue();
    if (n->dataInRXFIFO()) {
        readData();
    }
}
ISR(TIMER1_COMPB_vect) {

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
    //attachInterrupt(digitalPinToInterrupt(2), nrfInterrupt, FALLING);

    n = new nRF24L01::Controller<FastInterface>(8, 2, 10);
    n->setPoweredUp(true);
    n->setPrimaryReceiver();
    unsigned char addr[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    n->setAddress(addr, 3);

    //n->setChannel(2);

    n->setAutoAcknowledgementEnabled(false);
    n->setUsesDynamicPayloadLength(false);
    n->setBitrate(2);
    n->setReceivedPacketLength(16);
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
    changeValue(0);
    // Stops the program from freezing when
    //  writing to the MCP
    SPI.usingInterrupt(0x0016);

    // Test signal
    pinMode(5, OUTPUT);
    analogWrite(5, 127);
}

long lastMeasureReset = 2000;
long lastPrint = millis();
void loop() {
    
    delay(250);
    lastMillis = millis();

    // Total bytes transferred
    u8x8.clearLine(0);
    u8x8.setCursor(0, 0);
    u8x8.print(bytesTransferred);

    // LastMillis
    u8x8.clearLine(1);
    u8x8.setCursor(0, 1);
    u8x8.print("LM:");
    u8x8.print(lastMillis);

    // Bytes/second
    u8x8.clearLine(2);
    u8x8.setCursor(0, 2);
    u8x8.print( (float) bytesTransferred /
                ( ((float) (millis() - lastMeasureReset)) / 1000.0) );

    // Status register
    unsigned char stat = (n->getStatusAndConfigRegisters() >> 8);
    u8x8.clearLine(3);
    u8x8.setCursor(0, 3);
    printBitsu8x8(stat);

    u8x8.setCursor(0, 4);
    u8x8.print("FIFO:");
    u8x8.clearLine(5);
    u8x8.setCursor(0, 5);
    unsigned char fifo = n->getFIFOStatus();
    printBitsu8x8(fifo);

    u8x8.clearLine(6);
    u8x8.setCursor(0, 6);
    u8x8.print( (float) writtenCount /
                ( ((float) (millis() - lastMeasureReset)) / 1000.0) );

    if ( millis() > (lastMeasureReset + 3000) ) {
        bytesTransferred = 0;
        writtenCount = 0;
        lastMeasureReset = millis();
    }

    // Straggler data
    if (n->dataInRXFIFO()) {
        readData();
    }
}
