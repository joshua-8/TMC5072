/*
 * This example for the https://github.com/joshua-8/tmc5072 library demonstrates velocity control
 * Motor2 will start spinning then stop spinning, and accelerate quickly but slow down slowly.
 */

#include "TMC5072.h"

TMC5072 driver1 = TMC5072(SPI, SPISettings(100000, SPI_MSBFIRST, SPI_MODE3), 1, 0.00001951, 0.0000144, 0.00025, /*vel and accel:*/ 0, 1.25, 1, 1, 1, 1);
void setup()
{
    Serial.begin(115200);
    driver1.beginSPI(SCK, MISO, MOSI, SS);
    delay(1);

    driver1.setCurrent(18, 17);

    driver1.setVelocitiesAndAccelerations(0, 1.25, 1, 1, 1, 1);
    driver1.setVStopAndVStartRaw(1, 10);
    driver1.clearGSTAT();
    driver1.enable();

    delay(2000);
    Serial.println(driver1.home()); // homes to switch that pulls ENC2N high when pressed
    Serial.println("---");
}
void loop()
{
    driver1.moveToPosition(1);
    delay(2000);
    driver1.moveToPosition(0);
    delay(2000);
}
