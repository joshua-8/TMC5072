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
    driver1setup();
}
void loop()
{
    if (driver1.getDriverResetFlag(false, false)) {
        Serial.println("driver restarted");
        driver1setup();
    }
    driver1.moveToPositionRaw(70000);

    delay(5000);

    driver1.moveToPositionRaw(0);

    do { // blocking wait until at target
        delay(1);

        if (driver1.getDriverResetFlag(false, false)) {
            Serial.println("driver restarted");
            driver1setup();
        }

    } while (driver1.isNotAtTarget());
}
void driver1setup()
{
    driver1.clearGSTAT();
    driver1.setCurrent(16, 1);
    driver1.setVelocitiesAndAccelerationsRaw(5000, 50000, 300, 150, 1500, 3000);
    driver1.enable();
}