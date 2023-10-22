#include "TMC5072.h"
#include "trinamic/tmc/ic/TMC5072/TMC5072.h"
#include <Arduino.h>
#include <SPI.h>

TMC5072 driver = TMC5072(SPI, SPISettings(100000, SPI_MSBFIRST, SPI_MODE3));
void setup()
{
    Serial.begin(115200);
    driver.beginSPI(SCK, MISO, MOSI, SS);

    driver.commWrite(TMC_WRITE_BIT + TMC5072_IHOLD_IRUN(1), 15 | 15 << 8);

    // comm( TMC_WRITE_BIT + TMC5072_V1(1), 5000);

    driver.commWrite(TMC_WRITE_BIT + TMC5072_RAMPMODE(1), 1);
    driver.commWrite(TMC5072_RAMPMODE(1), 0);

    driver.commWrite(TMC_WRITE_BIT + TMC5072_CHOPCONF(1), TMC5072_TOFF_MASK & 4); // turns on

    // comm( TMC_WRITE_BIT + TMC5072_A1(1), 100); //
    // comm( TMC_WRITE_BIT + TMC5072_V1(1), 500); //
}
void loop()
{
    Serial.println("###");
    driver.commWrite(TMC_WRITE_BIT + TMC5072_AMAX(1), 50000);
    driver.commWrite(TMC_WRITE_BIT + TMC5072_VMAX(1), 600000);
    delay(4000);
    driver.commWrite(TMC_WRITE_BIT + TMC5072_AMAX(1), 5000);
    driver.commWrite(TMC_WRITE_BIT + TMC5072_VMAX(1), 0);
    delay(2000);
}
