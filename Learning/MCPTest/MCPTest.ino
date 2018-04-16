#include <SPI.h>

template <typename T> void printBits(T in) {
    unsigned char s = sizeof(T) * 8;
    Serial.print("0b");
    for (signed char g = s - 1; g >= 0; g--) {
        Serial.print( ((in & (1 << ((unsigned char) g))) > 0) ? "1" : "0");
    }
    Serial.println("");
}


const byte CSN = 4;

void changeValue(byte val) {

    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    digitalWrite(CSN, LOW);
    int toTransfer = 0x00FF & val;
    SPI.transfer16(toTransfer);
    printBits(toTransfer);
    digitalWrite(CSN, HIGH);
    SPI.endTransaction();
}


void setup() {
    // put your setup code here, to run once:
    pinMode(CSN, OUTPUT);
    digitalWrite(CSN, HIGH);

    pinMode(A3, INPUT);

    SPI.begin();
    Serial.begin(57600);
}

byte cur = 0;
void loop() {
    changeValue(cur);
    Serial.println(analogRead(A3));
    cur++;
    delay(100);
}
