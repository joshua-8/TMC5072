#include "trinamic/tmc/ic/TMC5072/TMC5072.h"
#include <Arduino.h>
#include <SPI.h>
void setup()
{
    Serial.begin(115200);
    SPI.begin(SCK, MISO, MOSI, SS);
    pinMode(SPI.pinSS(), OUTPUT);
    digitalWrite(SPI.pinSS(), HIGH);
}
void loop()
{
    SPI.beginTransaction(SPISettings(10000, SPI_MSBFIRST, SPI_MODE3));
    digitalWrite(SPI.pinSS(), LOW);
    uint8_t a = SPI.transfer(TMC5072_INPUT);
    uint32_t b = SPI.transfer32(0);
    digitalWrite(SPI.pinSS(), HIGH);
    SPI.endTransaction();
    Serial.println(a, BIN);
    Serial.println(b, BIN);
    Serial.println();
    delay(1000);
}
