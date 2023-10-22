#ifndef TMC5072_H
#define TMC5072_H
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
