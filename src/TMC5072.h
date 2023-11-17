
#ifndef LIB_TMC5072_H
#define LIB_TMC5072_H
#include <Arduino.h>
#include <SPI.h>

#include "trinamic/tmc/ic/TMC5072/TMC5072.h"
// some code in this program were generated by github copilot, reviewed by joshua-8, and tested with a tmc5072 driver IC

/**
 * macro that helps with accessing registers with less typing
 * @param write: true or false, should the write bit be set?
 * @param register: the name of the register, not including TMC5072_, for example: VMAX
 * @param whichMotor: 0 or 1, which motor
 * @param value: the value that gets shifted, masked.
 * becomes register,data
 * @note only works for registers that have one for each motor, and when only one value is going into the register but that covers a lot of cases
 * */
#define tmc_commpact(write, register, whichMotor, value) (write ? TMC_WRITE_BIT : 0) + TMC5072_##register(whichMotor), TMC5072_##register##_MASK&((value) << TMC5072_##register##_SHIFT)

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

    byte CSpin;

    uint32_t d_v1;
    uint32_t d_vmax;
    uint32_t d_a1;
    uint32_t d_amax;
    uint32_t d_dmax;
    uint32_t d_d1;

    /**
     * @brief  (accel in steps per ta^2)*accelScaler = accel in your chosen units
     */
    float accelScaler = 1.0;
    /**
     * @brief  (velocity in steps per t)*velocityScaler = velocity in your chosen units
     */
    float velocityScaler = 1.0;
    /**
     * @brief  (position in steps)*positionScaler = position in your chosen units
     */
    float positionScaler = 1.0;

public:
    TMC5072(SPIClass& _spi, SPISettings _spiset, byte _whichMotor, float _positionScaler = 1.0, float _velocityScaler = 1.0, float _accelScaler = 1.0, float _d_v1 = 0, float _d_vmax = 1, float _d_a1 = 1, float _d_amax = 1, float _d_dmax = 1, float _d_d1 = 1)
        : spi(_spi)
        , spiset(_spiset)
    {
        whichMotor = _whichMotor & 1;
        statusbits = 0;
        rampstatbits = 0;
        positionScaler = _positionScaler;
        velocityScaler = _velocityScaler;
        accelScaler = _accelScaler;

        CSpin = spi.pinSS();

        setDefaultVelocitiesAndAccelerations(_d_v1, _d_vmax, _d_a1, _d_amax, _d_dmax, _d_d1, false);
    }

    /**
     * @brief  home motor against a switch
     * @note  uses left stop switch that goes high when the motor hits it
     * this is a blocking function, it will not return until the motor has homed or an error occurs
     * @param  maxHomingDistance:
     * @param  switchOffset:
     * @param  finalSpeed:
     * @param  switchBackDistance:
     * @param  switchExtraDistance:
     * @param  smallMovementTimeoutMs:
     * @param  firstMovementTimeoutMs:
     * @retval 0 if successful, other numbers if not
     */
    int home(float maxHomingDistance = 15, float switchOffset = 1, float finalSpeed = 0.1, float switchBackDistance = 0.25, float switchExtraDistance = 0.125, unsigned long smallMovementTimeoutMS = 5000, unsigned long firstMovementTimeoutMS = 15000)
    {
        enum State {
            start,
            startMovingTowardsSwitch,
            movingTowardsSwitch,
            startMovingAwayFromSwitch,
            movingAwayFromSwitch,
            startFinalMoveTowardsSwitch,
            finalMoveTowardsSwitch,
            movingToFinalZero,
            homeFinished,
        };
        State state = start;
        boolean sw;
        uint32_t xlatch;
        commWrite(TMC5072_SWMODE(whichMotor) + TMC_WRITE_BIT, ((true << TMC5072_LATCH_L_ACTIVE_SHIFT) | (true << TMC5072_EN_SOFTSTOP_SHIFT) | (true << TMC5072_STOP_L_ENABLE_SHIFT))); // latch xactual into xlatch when left switch activated

        unsigned long stateStarted = millis();
        while (true) { // returned from to exit
            stateStarted = millis();
            switch (state) {
            case start:
                setXActualRaw(0);
                moveToPositionRaw(0);
                if (getStopSwitchStatus(true)) { // already at home
                    xlatch = commRead(TMC5072_XLATCH(whichMotor));
                    state = startMovingAwayFromSwitch;
                } else {
                    state = startMovingTowardsSwitch;
                }
                break;
            case startMovingTowardsSwitch: // moving towards switch
                moveToPosition(-abs(maxHomingDistance)); // enough to hit switch, but won't go forever
                state = movingTowardsSwitch;
                break;
            case movingTowardsSwitch:
                do {
                    delay(1);
                    sw = getStopSwitchStatus(true);
                } while (!sw && isNotAtTarget(true) && (millis() - stateStarted) < firstMovementTimeoutMS);
                if (sw) {
                    xlatch = commRead(TMC5072_XLATCH(whichMotor));
                    state = startMovingAwayFromSwitch;
                } else {
                    return 1; // didn't hit switch
                }
                break;
            case startMovingAwayFromSwitch:
                moveToPositionRaw(xlatch + abs(switchBackDistance) / positionScaler);
                state = movingAwayFromSwitch;
                break;
            case movingAwayFromSwitch:
                do {
                    delay(1);
                } while (isNotAtTarget(true) && (millis() - stateStarted) < smallMovementTimeoutMS);
                sw = getStopSwitchStatus(true);
                if (sw) { // switch still active for some reason, error
                    return 2; // switch didn't deactivate
                } else {
                    state = startFinalMoveTowardsSwitch;
                }
                break;
            case startFinalMoveTowardsSwitch:

                setVels(finalSpeed);
                moveToPositionRaw(xlatch - abs(switchExtraDistance) / positionScaler);
                state = finalMoveTowardsSwitch;
                break;
            case finalMoveTowardsSwitch:
                do {
                    delay(1);
                    sw = getStopSwitchStatus(true);
                } while (!sw && isNotAtTarget(true) && (millis() - stateStarted) < smallMovementTimeoutMS);
                if (sw) {
                    xlatch = commRead(TMC5072_XACTUAL(whichMotor));
                    resetToDefaultVelocitiesAndAccelerations();
                    moveToPositionRaw(xlatch + switchOffset / positionScaler);
                    state = movingToFinalZero;
                } else {
                    return 3; // didn't hit switch
                }
                break;
            case movingToFinalZero:
                commWrite(TMC5072_SWMODE(whichMotor) + TMC_WRITE_BIT, 0); // latch xactual into xlatch when left switch activated
                do {
                    delay(1);
                } while (isNotAtTarget(true) && (millis() - stateStarted) < smallMovementTimeoutMS);
                setXActualRaw(0);
                moveToPositionRaw(0);
                do {
                    delay(1);
                } while (isNotAtTarget(true) && (millis() - stateStarted) < smallMovementTimeoutMS);
                if (getStopSwitchStatus(true)) {
                    return 4; // didn't move off the switch
                }
                commWrite(TMC5072_SWMODE(whichMotor) + TMC_WRITE_BIT, ((true << TMC5072_LATCH_L_ACTIVE_SHIFT) | (false << TMC5072_EN_SOFTSTOP_SHIFT) | (true << TMC5072_STOP_L_ENABLE_SHIFT))); // latch xactual into xlatch when left switch activated
                return 0;
                break;
            }
        }
    }

    void setAccels(float amax, float a1 = 0)
    {
        setAccelsRaw(amax / accelScaler, a1 / accelScaler);
    }
    void setDecels(float dmax, float d1 = 0)
    {
        setDecelsRaw(dmax / accelScaler, d1 / accelScaler);
    }
    void setVels(float vmax, float v1 = 0)
    {
        setVelsRaw(vmax / velocityScaler, v1 / velocityScaler);
    }

    void setPositionScaler(float _positionScaler)
    {
        positionScaler = _positionScaler;
    }
    void setVelocityScaler(float _velocityScaler)
    {
        velocityScaler = _velocityScaler;
    }
    void setAccelScaler(float _accelScaler)
    {
        accelScaler = _accelScaler;
    }

    void setXActual(float xactual)
    {
        setXActualRaw(xactual / positionScaler);
    }
    /**
     * @brief sets the driver to position mode and sets a position target
     * @param  target: (int32_t) target position in steps
     * @retval None
     */
    void moveToPosition(float target)
    {
        moveToPositionRaw(target / positionScaler);
    }

    /**
     * @brief sets the driver to velocity mode and sets a velocity target and optionally changes the acceleration
     * @param  speed: (float) velocity in your chosen units
     * @param  accel: (float) accel in your chosen units
     * @retval None
     */

    void moveVel(float speed, float accel = 0)
    {
        moveVelRaw(speed / velocityScaler, accel / accelScaler);
    }

    /**
     * @brief set all the velocity and acceleration settings for a position control move
     * see datasheet 6.2.1 and 11.2.2
     * @param  v1: threshold above which amax and dmax are used instead of a1 and d1, if 0 the IC uses only amax and dmax
     * @param  vmax: maximum velocity
     * @param  a1: acceleration while velocity is under v1
     * @param  amax: acceleration while velocity is above v1
     * @param  dmax: deceleration while velocity is above v1
     * @param  d1: deceleration while velocity is under v1
     * @retval None
     */
    void setVelocitiesAndAccelerations(float v1, float vmax, float a1, float amax, float dmax, float d1)
    {
        setVelocitiesAndAccelerationsRaw(v1 / velocityScaler, vmax / velocityScaler, a1 / accelScaler, amax / accelScaler, dmax / accelScaler, d1 / accelScaler);
    }
    void setDefaultVelocitiesAndAccelerations(float v1, float vmax, float a1, float amax, float dmax, float d1, boolean reset = false)
    {
        setDefaultVelocitiesAndAccelerationsRaw(v1 / velocityScaler, vmax / velocityScaler, a1 / accelScaler, amax / accelScaler, dmax / accelScaler, d1 / accelScaler, reset);
    }
    void resetToDefaultVelocitiesAndAccelerations()
    {
        setVelocitiesAndAccelerationsRaw(d_v1, d_vmax, d_a1, d_amax, d_dmax, d_d1);
    }

    void setXActualRaw(int32_t xactual)
    {
        commWrite(tmc_commpact(true, XACTUAL, whichMotor, xactual));
    }

    /**
     * @brief sets the driver to position mode and sets a position target
     * @param  target: (uint32_t) target position in steps
     * @retval None
     */
    void moveToPositionRaw(uint32_t target)
    {
        commWrite(tmc_commpact(true, RAMPMODE, whichMotor, 0));
        commWrite(tmc_commpact(true, XTARGET, whichMotor, target));
    }

    /**
     * @brief set all the velocity and acceleration settings for a position control move
     * see datasheet 6.2.1 and 11.2.2
     * @param  v1: 0...(2^20)-1 threshold above which amax and dmax are used instead of a1 and d1, if 0 the IC uses only amax and dmax
     * @param  vmax: 0...(2^23)-512 maximum velocity
     * @param  a1: 0...(2^16)-1 acceleration while velocity is under v1
     * @param  amax: 0...(2^16)-1 acceleration while velocity is above v1
     * @param  dmax: 0...(2^16)-1 deceleration while velocity is above v1
     * @param  d1: 1...(2^16)-1 deceleration while velocity is under v1
     * @retval None
     */
    void setVelocitiesAndAccelerationsRaw(uint32_t v1, uint32_t vmax, uint32_t a1, uint32_t amax, uint32_t dmax, uint32_t d1)
    {
        commWrite(tmc_commpact(true, V1, whichMotor, v1));
        commWrite(tmc_commpact(true, VMAX, whichMotor, vmax));
        commWrite(tmc_commpact(true, A1, whichMotor, a1));
        commWrite(tmc_commpact(true, AMAX, whichMotor, amax));
        commWrite(tmc_commpact(true, DMAX, whichMotor, dmax));
        commWrite(tmc_commpact(true, D1, whichMotor, d1));
    }
    void setDefaultVelocitiesAndAccelerationsRaw(uint32_t v1, uint32_t vmax, uint32_t a1, uint32_t amax, uint32_t dmax, uint32_t d1, boolean reset = false)
    {
        d_v1 = v1;
        d_vmax = vmax;
        d_a1 = a1;
        d_amax = amax;
        d_dmax = dmax;
        d_d1 = d1;
        if (reset) {
            resetToDefaultVelocitiesAndAccelerations();
        }
    }

    void setAccelsRaw(uint32_t amax, uint32_t a1 = 0)
    {
        commWrite(tmc_commpact(true, AMAX, whichMotor, amax));
        if (a1 != 0)
            commWrite(tmc_commpact(true, A1, whichMotor, a1));
    }
    void setDecelsRaw(uint32_t dmax, uint32_t d1 = 0)
    {
        commWrite(tmc_commpact(true, DMAX, whichMotor, dmax));
        if (d1 != 0)
            commWrite(tmc_commpact(true, D1, whichMotor, d1));
    }
    void setVelsRaw(uint32_t vmax, uint32_t v1 = 0)
    {
        commWrite(tmc_commpact(true, VMAX, whichMotor, vmax));
        if (v1 != 0)
            commWrite(tmc_commpact(true, V1, whichMotor, v1));
    }

    /**
     * @brief set motor start and stop velocities.
     * see datasheet 6.2.1
     * @note   The datasheet says that VSTART should be >= VSTOP
     * @param  vstart: 0...(2^18)-1 (recommend at least 1)
     * @param  vstop:  1...(2^18)-1 (datasheet says in positioning mode it should be at least 10)
     * @retval None
     */
    void setVStopAndVStartRaw(uint32_t vstart, uint32_t vstop)
    {
        commWrite(tmc_commpact(true, VSTOP, 1, vstart));
        commWrite(tmc_commpact(true, VSTART, 1, vstop));
    }

    /**
     * @brief sets the driver to velocity mode and sets a velocity target and optionally changes the acceleration
     * Time reference t for velocities: t = 2^24 / fCLK
     * Time reference ta² for accelerations: ta² = 2^41 / (fCLK)²
     * @param  speed: (int32_t) velocity in steps per t, no unit conversion
     * @param  accel: (uint32_t) accel in steps per ta^2, no unit conversion
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
        commWrite(TMC_WRITE_BIT + TMC5072_IHOLD_IRUN(whichMotor), (TMC5072_IHOLD_MASK & (hold << TMC5072_IHOLD_SHIFT)) | (TMC5072_IRUN_MASK & (run << TMC5072_IRUN_SHIFT)));
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
        beginSPIcs(CS);
        SPI.begin(SCK, MISO, MOSI);
    }

    /**
     * @brief call this for every driver IC with which pin is connected to the driver's chip select pin
     * @note  call this before SPI.begin()
     * @param  CS: chip select pin for this driver, AKA SS
     * @retval None
     */
    void beginSPIcs(byte CS)
    {
        CSpin = CS;
        pinMode(CSpin, OUTPUT);
        digitalWrite(CSpin, HIGH);
    }

    /**
     * @brief  returns inverse of position_reached bit of RAMP_STAT register
     * @note   see 6.2.2.2 in datasheet
     * @param  read: default=true, if true refresh bits, if false use stored status bits
     * @retval (boolean)
     */
    boolean isNotAtTarget(boolean read = true)
    {
        return !isAtTarget(read);
    }

    /**
     * @brief  returns position_reached bit of RAMP_STAT register
     * @note   see 6.2.2.2 in datasheet
     * @param  read: default=true, if true refresh bits, if false use stored status bits
     * @retval (boolean)
     */
    boolean isAtTarget(boolean read = true)
    {
        return (getRampStatusBits(read) >> 9) & 1;
    }

    /**
     * @brief  returns RAMP_STAT register
     * @note   see 6.2.2.2 in datasheet
     * @param  read: default=true, if true refresh bits, if false use stored status bits
     * @retval uint32_t
     */
    uint32_t getRampStatusBits(boolean read = true)
    {
        if (read) {
            rampstatbits = commRead(TMC5072_RAMPSTAT(whichMotor));
        }
        return rampstatbits;
    }

    /**
     * @brief has the left switch for this motor been activated
     * @param  read: default=true, if true refresh status bits, if false use stored status bits
     * @retval (boolean)
     */
    boolean getStopSwitchStatus(boolean read = true)
    {
        return (getStatusBits(read) >> (whichMotor + 5)) & 1; // left stop switch
    }

    /**
     * @brief has the motor reached its target velocity
     * @param  read: default=true, if true refresh status bits, if false use stored status bits
     * @retval (boolean)
     */
    boolean getVelocityReachedStatus(boolean read = true)
    {
        return (getStatusBits(read) >> (whichMotor + 3)) & 1; // whether reached target velocity
    }

    /**
     * @brief is the driver for this motor reporting an error through the SPI_STATUS flags?
     * @param  read: default=true, if true refresh status bits, if false use stored status bits
     * @param  clear:  default=false, if true clear the flag and all other flags cleared by reading GSTAT
     * @retval (boolean)
     */
    boolean getDriverErrorStatus(boolean read = true, boolean clear = false)
    {
        boolean status = (getStatusBits(read) >> (whichMotor + 1)) & 1; // whether the driver has an error
        if (clear && status) {
            clearGSTAT();
        }
        return status;
    }

    /**
     * @brief this flag is set to true each time the IC is turned on, in SPI_STATUS
     * you can use this to detect if the IC just came back from losing power
     * @param  read: default=true, if true refresh status bits, if false use stored status bits
     * @param  clear: default=false, if true clear the flag and all other flags cleared by reading GSTAT
     * @retval (boolean)
     */
    boolean getDriverResetFlag(boolean read = true, boolean clear = false)
    {
        boolean status = (getStatusBits(read)) & 1;
        if (clear && status) {
            clearGSTAT();
        }
        return status;
    }

    /**
     * @brief reads from GSTAT to clear flags that get cleared when GSTAT is read
     * clears status bits for driver error, reset/just booted flag, UV_CP (undervoltage)
     * @retval None
     */
    void clearGSTAT()
    {
        comm(TMC5072_GSTAT, 0, nullptr);
    }

    /**
     * @brief  returns the status bits received with each spi access
     * see datasheet section 4.1.2
     * @param  read: default=true, if true this function gets updated status bits if false this function returns previously received info
     * @retval
     */
    uint8_t getStatusBits(boolean read = true)
    {
        if (read) { // updated read
            // status returned for all communication, reading ifcnt is arbitrary, but safe since it's read only
            comm(TMC5072_IFCNT, 0, nullptr); // comm sets the statusbits variable internally
        }
        return statusbits;
    }

    /**
     * @brief reads data from register
     * uses comm() twice since the driver returns the data from the last request
     * @param  reg: register to read from
     * @retval uint32_t
     */
    uint32_t commRead(uint8_t reg)
    {
        uint32_t data;
        comm(reg, 0, nullptr);
        comm(reg, 0, &data);
        return data;
    }

    /**
     * @brief writes data to register
     * uses comm()
     * see datasheet section 4
     * @param  reg: which register should be accessed? Add TMC_WRITE_BIT to write
     * @param  send: data to send
     * @retval None
     */
    void commWrite(uint8_t reg, uint32_t send)
    {
        comm(reg, send, nullptr);
    }
    /**
     * @brief communicates over SPI with the tmc5072
     * see datasheet section 4
     * @note the status bits received from the tcm5072 are saved and can be accessed by getStatusBits()
     * @param  reg: which register should be accessed? Add TMC_WRITE_BIT to write
     * @param  send: data to write to the register (ignored by tmc5072 if it is in read mode)
     * @param  data: uint32_t* if not null, the received data from the tcm5072 is put here
     * @retval None
     */
    void comm(uint8_t reg, uint32_t send, uint32_t* data)
    {
        spi.beginTransaction(spiset);
        digitalWrite(CSpin, LOW);

        statusbits = spi.transfer(reg);

        uint32_t databits = spi.transfer32(send);
        if (data != NULL)
            *data = databits;

        digitalWrite(CSpin, HIGH);
        spi.endTransaction();
    }
};
#endif // TMC5072_H
