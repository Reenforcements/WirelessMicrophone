
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

void wirelessMic_setupADC() {
    // A2 needs to be an input.
    pinMode(A2, INPUT);
    // Lets see if we can get the multiplexer to select this input
    //  without messing with the registers by calling analogRead.
    analogRead(A2);
    //A normal conversion takes 13 ADC clock cycles.
    //The first conversion after the ADC is switched on (i.e., ADCSRA.ADEN is written to '1')
    //  takes 25 ADC clock cycles in order to initialize the analog circuitry.

    // We want the result to be left adjusted so we can get an 8 bit result in one swoop
    ADMUX |= (1 << ADLAR);

    // Do a conversion to initialize things
    ADCSRA |= (1 << ADEN); // ADEN
    ADCSRA |= (1 << ADSC); // ADSC
    delayMicroseconds(20);

    // Bits 2:0 – ADPSn: ADC Prescale r Select [n = 2:0]
    // (Clear and set)
    ADCSRA = ADCSRA & (0b11111000);
    ADCSRA |= 0b100; //100 = divide input clock by 16 (011 for 8)
    // Enable ADC Interrupt
    ADCSRA |= (1 << ADIE);
    // Select timer 0 for triggering the ADC
    // Do Timer 0 compare match A
    ADCSRB = ADCSRB & (0b11111000);
    ADCSRB |= 0b101; //0b101 Timer/Counter1 Compare Match B
    // Enable auto triggering of ADC conversions
    ADCSRA |= (1 << ADATE);
}
void wirelessMic_setupTimer1() {
    // Setup timer 1!
    PRR = PRR & (~(1 << 3)); // Enable timer 1
    TCCR1A = 0b00000000; // CTC mode
    TCCR1B = TCCR1B & 0b00000000; // No prescaler, CTC Mode, noise canceller disabled
    TCCR1B |= 0b01001;// NORMAL MODE unless 01001 THEN IT WILL BE CTC

    // 363 for 44,077 Hz
    unsigned int timerACompare = 300;
    byte upper = ((timerACompare & 0xFF00) >> 8);
    byte lower = timerACompare & 0xFF;

    // The counter value must be in this spot in the code
    //  or the counter won't work.
    // The value for A will reset the counter
    OCR1AH = upper;
    OCR1AL = lower;
    // Same value for B because that triggers the ADC
    OCR1BH = upper;
    OCR1BL = lower;

    // Enable output compare A
    TIMSK1 = (1 << 2);
}

volatile byte audioBytes[32];
volatile byte currentAudioByte = 0;
volatile bool readyForMore = true;
volatile unsigned long sendCounter = 0;
volatile unsigned long readCounter = 0;
ISR(ADC_vect) {
    //byte high = ADCH;
    audioBytes[ currentAudioByte & 0b00011111 ] = ADCH;

    currentAudioByte++;
    readCounter++;

    if (currentAudioByte == 32 && readyForMore == true) {
        n->startSendingPacket((unsigned char*) audioBytes, 32);
        currentAudioByte = 0;
        readyForMore = false;
    }
}
ISR(TIMER1_COMPA_vect) {
    // Must be here or the Arduino will explode internally.
}
ISR(TIMER1_COMPB_vect) {
    // Must be here or the Arduino will explode internally.
}

volatile unsigned char lastInterruptBits = 0;
void nrfInterrupt() {
    // Read and clear the interrupt bits.
    //n->concludeSendingPacket();
    n->readAndClearInterruptBits();
    // Set the current byte to zero.
    //currentAudioByte = 0;
    readyForMore = true;
    sendCounter += 32;
}

void setup() {
    delay(100);
    Serial.begin(57600);

    n = new nRF24L01::Controller<nRF24L01::ArduinoInterface>(8, 2, 10);

    digitalWrite(A2, HIGH);
    pinMode(A3, INPUT);

    //ADCSRA = ADCSRA & (0b11111000);
    //ADCSRA |= 0b100;

    attachInterrupt(digitalPinToInterrupt(2), nrfInterrupt, FALLING);

    // Set up our transmitter to have minimal overhead
    n->setPoweredUp(true);
    n->setPrimaryTransmitter();
    unsigned char addr[] = {0x12, 0x34, 0x56, 0x78, 0x9A};
    n->setAddress(addr, 3);
    n->setChannel(2);
    n->setAutoAcknowledgementEnabled(false);
    n->setUsesDynamicPayloadLength(false);
    n->setBitrate(2);
    n->setAutoRetransmitCount(0);
    n->setCRCEnabled(false);
    n->readAndClearInterruptBits();
    n->enableCEPin();

    wirelessMic_setupADC();
    wirelessMic_setupTimer1();
}
long lastPrint = millis();
void loop() {
    if (millis() > (lastPrint + 1000)) {
        lastPrint = millis();
        Serial.print("Bytes sent: ");
        Serial.print(sendCounter);
        Serial.print(" Bytes ADC'd:");
        Serial.println(readCounter);
        sendCounter = 0;
        readCounter = 0;
    }

    //while (currentAudioByte != 32);
    //n->startSendingPacket((unsigned char*) audioBytes, 32);
    //while (currentAudioByte != 0);
}


