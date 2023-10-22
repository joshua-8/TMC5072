#include "trinamic/tmc/ic/TMC5072/TMC5072.h"
#include <Arduino.h>
#include <SPI.h>
SPISettings spiset = SPISettings(100000, SPI_MSBFIRST, SPI_MODE3);
void setup()
{
    Serial.begin(115200);
    SPI.begin(SCK, MISO, MOSI, SS);
    pinMode(SPI.pinSS(), OUTPUT);
    digitalWrite(SPI.pinSS(), HIGH);
    delay(100);

    commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_IHOLD_IRUN(1), 15 | 15 << 8);

    // commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_V1(1), 5000);

    commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_RAMPMODE(1), 1);
    commAndPrint(SPI, spiset, TMC5072_RAMPMODE(1), 0);

    commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_CHOPCONF(1), TMC5072_TOFF_MASK & 4); // turns on

    // commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_A1(1), 100); //
    // commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_V1(1), 500); //
}
void loop()
{
    Serial.println("###");
    commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_AMAX(1), 5000);
    commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_VMAX(1), 500000);
    delay(8000);
    commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_AMAX(1), 50000);
    commAndPrint(SPI, spiset, TMC_WRITE_BIT + TMC5072_VMAX(1), 10000);
    delay(4000);
}

void commAndPrint(SPIClass& spi, SPISettings spiSettings, uint8_t reg, uint32_t send)
{
    uint8_t a;
    uint32_t b;
    comm(spi, spiSettings, reg, send, a, b);
    Serial.print(a, BIN);
    Serial.print(" ");
    Serial.println(b, BIN);
}
void comm(SPIClass& spi, SPISettings spiSettings, uint8_t reg, uint32_t send, uint8_t& status, uint32_t& data)
{
    spi.beginTransaction(spiSettings);
    digitalWrite(spi.pinSS(), LOW);
    status = spi.transfer(reg);
    data = spi.transfer32(send);
    digitalWrite(spi.pinSS(), HIGH);
    spi.endTransaction();
}