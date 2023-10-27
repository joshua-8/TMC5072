#ifndef LIB_TMC5072_H
#define LIB_TMC5072_H
#include <Arduino.h>
#include <SPI.h>

#include "trinamic/tmc/ic/TMC5072/TMC5072.h"

/**
 * macro that helps with accessing registers with less typing
 * @param write: true or false, should the write bit be set?
 * @param register: the name of the register, not including TMC5072_, for example: VMAX
 * @param whichMotor: 0 or 1, which motor
 * @param value: the value that gets shifted, masked.
 * becomes register,data
 * @note only works for registers that have one for each motor, and when only one value is going into the register but that covers a lot of cases
 * */
#define TMC5072_COMMPACT(write, register, whichMotor, value) (write ? TMC_WRITE_BIT : 0) + TMC5072_##register(whichMotor), TMC5072_##register##_MASK&(value << TMC5072_##register##_SHIFT)

/**
 * @brief
 */
class TMC5072 {
protected:
    SPIClass& spi;
    SPISettings spiset;
    byte whichMotor;

public:
    TMC5072(SPIClass& _spi, SPISettings _spiset, byte _whichMotor)
        : spi(_spi)
        , spiset(_spiset)
    {
        whichMotor = _whichMotor & 1;
    }
    void setVelAndAccelRaw(uint32_t vmax, uint16_t amax, uint16_t dmax)
    {
        commWrite(TMC5072_COMMPACT(true, VMAX, whichMotor, vmax));
        if (amax != 0)
            commWrite(TMC5072_COMMPACT(true, AMAX, whichMotor, amax));
        if (dmax != 0)
            commWrite(TMC5072_COMMPACT(true, DMAX, whichMotor, dmax));
    }
    void setTargetRaw(int32_t target)
    {
        commWrite(TMC5072_COMMPACT(true, RAMPMODE, whichMotor, 0));
        commWrite(TMC5072_COMMPACT(true, XTARGET, whichMotor, target));
    }
    /**
     * @brief turns on power to the motor
     * @param  toff: 1-15, "sets the slow decay off time" see datasheet page 57
     * @retval None
     */
    void enable(byte toff = 4)
    {
        commWrite(TMC_WRITE_BIT + TMC5072_CHOPCONF(whichMotor), TMC5072_TOFF_MASK & (toff << TMC5072_TOFF_SHIFT)); // turns on
    }
    /**
     * @brief  turns off power to the motor and allows it to turn freely
     * @retval None
     */
    void disable()
    {
        commWrite(TMC_WRITE_BIT + TMC5072_CHOPCONF(whichMotor), 0);
    }
    /**
     * @brief See page 47 of the datasheet
     * @note For high precision motor operation, work with a current scaling factor in the range 16 to 31
     * @param  run: (0-31) scale factor for current when the motor is turning
     * @param  hold: (0-31) scale factor for current when the motor is in stand still
     * @retval None
     */
    void setCurrent(byte run, byte hold)
    {
        commWrite(TMC_WRITE_BIT + TMC5072_IHOLD_IRUN(whichMotor), (TMC5072_IHOLD_MASK & (run << TMC5072_IHOLD_SHIFT)) | (TMC5072_IRUN_MASK & (run << TMC5072_IRUN_SHIFT)));
    }
    /**
     * @brief sets the driver to velocity mode and sets a velocity target and optionally changes the acceleration
     * @param  speed: (int32_t) velocity, no unit conversion
     * @param  accel: (uint32_t) accel, no unit conversion
     * @retval None
     */
    void setVelRaw(int32_t speed, uint32_t accel = 0)
    {
        commWrite(TMC5072_COMMPACT(true, RAMPMODE, whichMotor, speed >= 0 ? 1 : 2));
        if (accel != 0)
            commWrite(TMC5072_COMMPACT(true, AMAX, whichMotor, accel));
        commWrite(TMC5072_COMMPACT(true, VMAX, whichMotor, abs(speed)));
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
        beginSPIcs(CS);
    }
    /**
     * @brief call this for every driver IC with which pin is connected to the driver's chip select pin
     * @param  CS: chip select, AKA SS
     * @retval None
     */
    void beginSPIcs(byte CS)
    {
        pinMode(SPI.pinSS(), OUTPUT);
        digitalWrite(SPI.pinSS(), HIGH);
    }

    /**
     * @brief
     * @note
     * @param  reg:
     * @param  send:
     * @retval None
     */
    void commWrite(uint8_t reg, uint32_t send)
    {
        spi.beginTransaction(spiset);
        digitalWrite(spi.pinSS(), LOW);
        spi.transfer(reg);
        spi.transfer32(send);
        digitalWrite(spi.pinSS(), HIGH);
        spi.endTransaction();
    }
    /**
     * @brief
     * @note
     * @param  reg:
     * @param  send:
     * @param  status:
     * @param  data:
     * @retval None
     */
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
