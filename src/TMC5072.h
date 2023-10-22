#ifndef LIB_TMC5072_H
#define LIB_TMC5072_H
#include "trinamic/tmc/ic/TMC5072/TMC5072.h"
#include <Arduino.h>
#include <SPI.h>
/**
 * @brief
 */
class TMC5072 {
protected:
    SPIClass& spi;
    SPISettings spiset;

public:
    TMC5072(SPIClass& _spi, SPISettings _spiset)
        : spi(_spi)
        , spiset(_spiset)
    {
    }
    /**
     * @brief calls spi.begin and sets up chip select pin
     * @note only needed once per SPI bus
     * @param  SCK: clock
     * @param  SDI: AKA CIPO and MISO
     * @param  SDO: AKA COPI and MOSI
     * @param  CS: chip select, AKA SS
     * @retval None
     */
    void beginSPI(byte SCK, byte SDI, byte SDO, byte CS)
    {
        SPI.begin(SCK, MISO, MOSI, SS);
        pinMode(SPI.pinSS(), OUTPUT);
        digitalWrite(SPI.pinSS(), HIGH);
    }
    void commWrite(uint8_t reg, uint32_t send)
    {
        spi.beginTransaction(spiset);
        digitalWrite(spi.pinSS(), LOW);
        spi.transfer(reg);
        spi.transfer32(send);
        digitalWrite(spi.pinSS(), HIGH);
        spi.endTransaction();
    }
    void comm(uint8_t reg, uint32_t send, uint8_t& status, uint32_t& data)
    {
        spi.beginTransaction(spiset);
        digitalWrite(spi.pinSS(), LOW);
        status = spi.transfer(reg);
        data = spi.transfer32(send);
        digitalWrite(spi.pinSS(), HIGH);
        spi.endTransaction();
    }
};
#endif // TMC5072_H
