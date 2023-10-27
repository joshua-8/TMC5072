/*
 * This example for the https://github.com/joshua-8/tmc5072 library demonstrates velocity control
 * Motor2 will start spinning then stop spinning, and accelerate quickly but slow down slowly.
 */

#include "TMC5072.h"

TMC5072 driver1 = TMC5072(SPI, SPISettings(100000, SPI_MSBFIRST, SPI_MODE3), 1);
void setup()
{
    Serial.begin(115200);
    driver1.beginSPI(SCK, MISO, MOSI, SS);
    delay(10);

    driver1.setCurrent(16, 16);

    driver1.commWrite(TMC5072_COMMPACT(true, A1, 1, 1000));
    driver1.commWrite(TMC5072_COMMPACT(true, D1, 1, 1000));
    driver1.commWrite(TMC5072_COMMPACT(true, VSTOP, 1, 10));
    driver1.commWrite(TMC5072_COMMPACT(true, VSTART, 1, 1));
    driver1.commWrite(TMC5072_COMMPACT(true, V1, 1, 5000));
    driver1.commWrite(TMC5072_COMMPACT(true, AMAX, 1, 500));
    driver1.commWrite(TMC5072_COMMPACT(true, DMAX, 1, 500));

    driver1.enable();

    driver1.commWrite(TMC5072_COMMPACT(true, XACTUAL, 1, 0));

    driver1.commWrite(TMC5072_COMMPACT(true, RAMPMODE, 1, 0));
}
void loop()
{
    // driver1.setTargetRaw(0);
    // driver1.setVelRaw(10000, 1000);
    driver1.commWrite(TMC5072_COMMPACT(true, XTARGET, 1, 5000));
    for (int i = 0; i < 7; i++) {
        delay(1000);
        uint8_t a;
        uint32_t b;
        driver1.comm(TMC5072_XTARGET(1), 0, a, b);

        Serial.print(a);
        Serial.print(",");
        Serial.println(b);

        uint8_t A;
        uint32_t B;
        driver1.comm(TMC5072_XACTUAL(1), 0, A, B);
        Serial.print(A);
        Serial.print(";");
        Serial.println(B);
    }

    // driver1.setTargetRaw(10000);
    driver1.commWrite(TMC5072_COMMPACT(true, RAMPMODE, 1, 0));
    driver1.commWrite(TMC5072_COMMPACT(true, XTARGET, 1, 0));
    uint8_t a;
    uint32_t b;
    driver1.comm(TMC5072_XTARGET(1), 0, a, b);
    Serial.print(a);
    Serial.print("$");
    Serial.println(b);

    delay(4000);
}
