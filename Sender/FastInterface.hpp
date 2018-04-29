#ifndef FastInterface_hpp
#define FastInterface_hpp

#include "NRF24L01Interface.hpp"

class FastInterface : public nRF24L01::NRF24L01Interface {
    public:
        void begin();
        void end();

        void beginTransaction();
        void endTransaction();

        unsigned char transferByte(unsigned char b);
        void transferBytes(unsigned char **b, unsigned char size);

        void delay(unsigned int d);
        void delayMicroseconds(unsigned int d);

        void writeCSNHigh();
        void writeCSNLow();
        void writeCEHigh();
        void writeCELow();

        FastInterface(nRF24L01::SpecialPinHolder *s): nRF24L01::NRF24L01Interface(s) {

        }
    private:

};


#endif /* FastInterface_hpp */

