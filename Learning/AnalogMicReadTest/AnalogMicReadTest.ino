template <typename T> void printBits(T in) {
    unsigned char s = sizeof(T) * 8;
    Serial.print("0b");
    for (signed char g = s - 1; g >= 0; g--) {
        Serial.print( ((in & (1 << ((unsigned char) g))) > 0) ? "1" : "0");
    }
    Serial.println("");
}

void wirelessMic_setupADC() {
    pinMode(A2, INPUT);
    // Lets see if we can get the multiplexer to select this input
    //  without messing with the registers by calling analogRead.
    analogRead(A2);
    //A normal conversion takes 13 ADC clock cycles.
    //The first conversion after the ADC is switched on (i.e., ADCSRA.ADEN is written to '1')
    //  takes 25 ADC clock cycles in order to initialize the analog circuitry.

    // We want the result to be left adjusted so we can get an 8 bit result in one swoop
    byte ADLARbit = (1 << 5);
    ADMUX |= ADLARbit;

    // Do a conversion to initialize things
    ADCSRA |= (1 << 7); // ADEN
    ADCSRA |= (1 << 6); // ADSC
    delayMicroseconds(20);

    //printBits(CLKPR);
    // Bits 2:0 – ADPSn: ADC Prescale r Select [n = 2:0]
    // (Clear and set)
    ADCSRA = ADCSRA & (0b11111000);
    ADCSRA |= 0b100; //100 = divide input clock by 16
    // Enable ADC Interrupt
    ADCSRA |= (1 << 3); // ADIE
    // Select timer 0 for triggering the ADC
    // Do Timer 0 compare match A
    ADCSRB = ADCSRB & (0b11111000);
    ADCSRB |= 0b101; //0b101 Timer/Counter1 Compare Match B
    // Enable auto triggering of ADC conversions
    ADCSRA |= (1 << 5); // ADATE
    printBits(ADCSRA);
}
void wirelessMic_setupTimer1() {

    // Setup timer 1!
    PRR = PRR & (~(1 << 3)); // Enable timer 1
    TCCR1A = 0b00000000; // CTC mode
    TCCR1B = TCCR1B & 0b00000000; // No prescaler, CTC Mode, noise canceller disabled
    TCCR1B |= 0b01001;// NORMAL MODE unless 01001 THEN IT WILL BE CTC
    //TIFR1 = 0;

    // 363 for 44,077 Hz
    unsigned int timerACompare = 362;
    byte upper = ((timerACompare & 0xFF00) >> 8);
    byte lower = timerACompare & 0xFF;

    // TODO: "So, move OCR1A = 20; to after your last TCCR1B line and before your TIMSK1 line."
    // The value for A will reset the counter
    OCR1AH = upper;
    OCR1AL = lower;
    // Same value for B because that triggers the ADC
    OCR1BH = upper;
    OCR1BL = lower;
    //Serial.println(OCR1BL);
    //Serial.println(OCR1BH);

    TIMSK1 = (1 << 2);
}

volatile byte audioBytes[32];
volatile byte currentAudioByte = 0;
volatile long reads = 0;
ISR(ADC_vect) {
    byte high = ADCH;
    audioBytes[ (currentAudioByte & 0b00011111) ] = high;
    reads++;
    currentAudioByte++;
}
ISR(TIMER1_COMPA_vect) {
    // Must be here or the Arduino will explode internally.
}
ISR(TIMER1_COMPB_vect) {
    // Must be here or the Arduino will explode internally.
}


void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    wirelessMic_setupADC();
    wirelessMic_setupTimer1();
}

void loop() {
    // put your main code here, to run repeatedly:
    //Serial.println(analogRead(A2));
    delay(5);
    cli();
    for(byte i = 0; i < 32; i++) {
        Serial.println(audioBytes[i]);
    }
    //Serial.println(reads);
    //Serial.println( (((int)TCNT1H) << 8) + TCNT1L);
    sei();
    reads = 0;
}


