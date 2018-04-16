
#include "nRF24L01.hpp"
#include "ArduinoInterface.hpp"

nRF24L01::Controller<nRF24L01::ArduinoInterface> *n;

//template <typename T> void //printBits(T in) {
//    unsigned char s = sizeof(T) * 8;
//    //Serial.print("0b");
//    for (signed char g = s - 1; g >= 0; g--) {
//        //Serial.print( ((in & (1 << ((unsigned char) g))) > 0) ? "1" : "0");
//    }
//    //Serial.println("");
//}

volatile unsigned char lastInterruptBits = 0;
volatile bool readyForMoreData = true;
void nrfInterrupt() {
    // Read and clear the interrupt bits.
    n->concludeSendingPacket();
    n->readAndClearInterruptBits();
    
    if(n->didSendPayload()) {
        readyForMoreData = true;
    } else if(n->didHitMaxRetry()) {
        readyForMoreData = true;
    }
}

unsigned int totalCount = 0;
void setup() {
    delay(100);
    //Serial.begin(9600);
    //Serial.println("Begin");
    n = new nRF24L01::Controller<nRF24L01::ArduinoInterface>(8, 2, 10);
    pinMode(A2, OUTPUT);
    digitalWrite(A2, HIGH);
    pinMode(A3, INPUT);
    
    ADCSRA = ADCSRA & (0b11111000);
    ADCSRA |= 0b100;
    
    unsigned int g = 0b1100101011110000;
    //printBits(g);

    attachInterrupt(digitalPinToInterrupt(2), nrfInterrupt, FALLING);
    

    n->setPoweredUp(true);
    n->setPrimaryTransmitter();
    unsigned char addr[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    n->setAddress(addr, 3);
    //n->setChannel(2);
    n->setAutoAcknowledgementEnabled(false);
    n->setUsesDynamicPayloadLength(false);
    n->setBitrate(2);
    n->setAutoRetransmitCount(0);
    n->readAndClearInterruptBits();
    n->setCRCEnabled(false);
    
    unsigned char text[32] = "1234567890123456789012345678901";
    while (true) {
        //Serial.print("Status and config:");
        //printBits(n->getStatusAndConfigRegisters());
        //Serial.print("Last interrupt:");
        //printBits(lastInterruptBits);
        //Serial.print("FIFO:");
        //unsigned char fifo = n->getFIFOStatus();
        //printBits(fifo);
        //delay(125);
        //digitalWrite(A2, LOW);
        //unsigned char text[2] = {(unsigned char)(totalCount & 0xFF), (unsigned char)((totalCount & 0xFF00) >> 8) };
        
        //Serial.print("Sending data: ");
        //Serial.println(totalCount);
        readyForMoreData = false;
        n->startSendingPacket(text, 32);
        for(byte i = 0; i < 32; i++) {
            text[i] = analogRead(A3);
        }
        //delayMicroseconds(5);
        //n->startSendingPacket(text, 32);
        //delayMicroseconds(20);
        
        while( readyForMoreData == false) {
            ////printBits(n->getStatusAndConfigRegisters());
            //delay(33);
        }
        //Serial.println("Sent data.");
        
        //delay(5);
        //digitalWrite(A2, LOW);
        //totalCount += 1;
    }
}

void loop() {
    // put your main code here, to run repeatedly:

}
