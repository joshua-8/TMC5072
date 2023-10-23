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
    boolean whichMotor;

public:
    TMC5072(SPIClass& _spi, SPISettings _spiset, boolean _whichMotor)
        : spi(_spi)
        , spiset(_spiset)
    {
        whichMotor = _whichMotor;
    }
    void enable(byte toff = 4)
    {
        commWrite(TMC_WRITE_BIT + TMC5072_CHOPCONF(whichMotor), TMC5072_TOFF_MASK & toff); // turns on
    }
    void setCurrent(byte run, byte hold)
    {
        commWrite(TMC_WRITE_BIT + TMC5072_IHOLD_IRUN(whichMotor), (hold & TMC5072_IHOLD_MASK) | (TMC5072_IRUN_MASK & (run << TMC5072_IRUN_SHIFT)));
    }
    void setVel(int32_t speed, uint32_t accel = 0)
    {
        commWrite(TMC_WRITE_BIT + TMC5072_RAMPMODE(whichMotor), speed >= 0 ? 1 : 2);
        if (accel != 0)
            commWrite(TMC_WRITE_BIT + TMC5072_AMAX(whichMotor), accel);
        commWrite(TMC_WRITE_BIT + TMC5072_VMAX(whichMotor), abs(speed));
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
