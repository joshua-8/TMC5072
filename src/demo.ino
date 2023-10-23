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

    driver1.setCurrent(5, 1);

    driver1.enable();
}
void loop()
{
    driver1.setVel(90000, 1000);
    delay(4000);
    driver1.setVel(-90000, 5000);
    delay(2000);
}
