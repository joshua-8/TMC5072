/*
 * This example for the https://github.com/joshua-8/tmc5072 library demonstrates velocity control
 * Motor2 will start spinning then stop spinning, and accelerate quickly but slow down slowly.
 */

#include "TMC5072.h"

TMC5072 driver1 = TMC5072(SPI, SPISettings(100000, SPI_MSBFIRST, SPI_MODE3), 1, 0.00001951, 0.0000144, 0.00025);
void setup()
{
    Serial.begin(115200);
    driver1.beginSPI(SCK, MISO, MOSI, SS);
    delay(1);

    driver1.setCurrent(16, 1);

    driver1.setVelocitiesAndAccelerationsRaw(10000, 50000, 30000, 10500, 10500, 30000);
    driver1.setVStopAndVStartRaw(100, 100);
    driver1.clearGSTAT();
    driver1.enable();
}
void loop()
{
    driver1.setVelocitiesAndAccelerations(0, 2, .1, .1, 100, 100);
    driver1.moveToPosition(1);
    delay(6000);
    // driver1.moveVel(-1);
    driver1.moveToPositionRaw(0);
    delay(6000);
}
