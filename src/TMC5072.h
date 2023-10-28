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
#define tmc_commpact(write, register, whichMotor, value) (write ? TMC_WRITE_BIT : 0) + TMC5072_##register(whichMotor), TMC5072_##register##_MASK&(value << TMC5072_##register##_SHIFT)

/**
 * @brief
 */
class TMC5072 {
protected:
    SPIClass& spi;
    SPISettings spiset;
    byte whichMotor;
    uint8_t statusbits;
    uint32_t rampstatbits;

public:
    TMC5072(SPIClass& _spi, SPISettings _spiset, byte _whichMotor)
        : spi(_spi)
        , spiset(_spiset)
    {
        whichMotor = _whichMotor & 1;
        statusbits = 0;
        rampstatbits = 0;
    }
    void movePositionRaw(int32_t target)
    {
        commWrite(tmc_commpact(true, RAMPMODE, whichMotor, 0));
        commWrite(tmc_commpact(true, XTARGET, whichMotor, target));
    }

    void setVelocitiesAndAccelerationsRaw(uint32_t v1, uint32_t vmax, uint32_t a1, uint32_t amax, uint32_t dmax, uint32_t d1)
    {
        commWrite(tmc_commpact(true, V1, whichMotor, v1));
        commWrite(tmc_commpact(true, VMAX, whichMotor, vmax));
        commWrite(tmc_commpact(true, A1, whichMotor, a1));
        commWrite(tmc_commpact(true, AMAX, whichMotor, amax));
        commWrite(tmc_commpact(true, DMAX, whichMotor, dmax));
        commWrite(tmc_commpact(true, D1, whichMotor, d1));
    }

    /**
     * @brief
     * @note   The datasheet says that VSTART should be >= VSTOP
     * @param  vstart:
     * @param  vstop:
     * @retval None
     */
    void setVStopAndVStartRaw(uint32_t vstart, uint32_t vstop)
    {
        commWrite(tmc_commpact(true, VSTOP, 1, vstart));
        commWrite(tmc_commpact(true, VSTART, 1, vstop));
    }

    /**
     * @brief  Sets velocity and acceleration settings for velocity mode
     * @param  vmax:
     * @param  amax:
     * @param  dmax:
     * @retval None
     */
    void setVelAndAccelRaw(uint32_t vmax, uint16_t amax, uint16_t dmax)
    {
        commWrite(tmc_commpact(true, VMAX, whichMotor, vmax));
        if (amax != 0)
            commWrite(tmc_commpact(true, AMAX, whichMotor, amax));
        if (dmax != 0)
            commWrite(tmc_commpact(true, DMAX, whichMotor, dmax));
    }
    /**
     * @brief sets the driver to velocity mode and sets a velocity target and optionally changes the acceleration
     * @param  speed: (int32_t) velocity, no unit conversion
     * @param  accel: (uint32_t) accel, no unit conversion
     * @retval None
     */
    void moveVelRaw(int32_t speed, uint32_t accel = 0)
    {
        commWrite(tmc_commpact(true, RAMPMODE, whichMotor, speed >= 0 ? 1 : 2));
        if (accel != 0)
            commWrite(tmc_commpact(true, AMAX, whichMotor, accel));
        commWrite(tmc_commpact(true, VMAX, whichMotor, abs(speed)));
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
     * @brief calls spi.begin and sets up chip select pin
     * @note only needed once per SPI bus
     * @param  SCK: clock
     * @param  SDI: AKA CIPO and MISO
     * @param  SDO: AKA COPI and MOSI
     * @param  CS: chip select pin for this driver, AKA SS
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

    boolean isNotAtTarget(boolean read = true)
    {
        return !isAtTarget(read);
    }
    boolean isAtTarget(boolean read = true)
    {
        return (getRampStatusBits(read) >> 9) & 1;
    }

    uint16 getRampStatusBits(boolean read = true)
    {
        if (read)
            comm(TMC5072_RAMPSTAT(whichMotor), 0, &rampstatbits);
        return rampstatbits;
    }

    boolean getStopSwitchStatus(boolean read = true)
    {
        return (getStatusBits(read) >> (whichMotor + 5)) & 1; // left stop switch
    }

    boolean getVelocityReachedStatus(boolean read = true)
    {
        return (getStatusBits(read) >> (whichMotor + 3)) & 1; // whether reached target velocity
    }

    boolean getDriverErrorStatus(boolean read = true, boolean clear = false)
    {
        boolean status = (getStatusBits(read) >> (whichMotor + 1)) & 1; // whether reached target velocity
        if (clear && status) {
            clearGSTAT();
        }
        return status;
    }

    boolean getDriverResetFlag(boolean read = true, boolean clear = false)
    {
        boolean status = (getStatusBits(read)) & 1;
        if (clear && status) {
            clearGSTAT();
        }
        return status;
    }
    void clearGSTAT()
    {
        comm(TMC5072_GSTAT, 0, nullptr);
    }

    uint8_t getStatusBits(boolean read = true)
    {
        if (read) { // updated read
            // status returned for all communication, reading ifcnt is arbitrary, but safe since it's read only
            comm(TMC5072_IFCNT, 0, nullptr); // comm sets the statusbits variable internally
        }
        return statusbits;
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
        comm(reg, send, nullptr);
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
    void comm(uint8_t reg, uint32_t send, uint32_t* data)
    {
        spi.beginTransaction(spiset);
        digitalWrite(spi.pinSS(), LOW);

        statusbits = spi.transfer(reg);

        uint32_t databits = spi.transfer32(send);
        if (data != NULL)
            *data = databits;

        digitalWrite(spi.pinSS(), HIGH);
        spi.endTransaction();
    }
};
#endif // TMC5072_H
