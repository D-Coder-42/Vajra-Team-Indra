#ifndef LPS22HH_H
#define LPS22HH_H

#include <Wire.h>

#define LPS22HH_ADDRESS 0x5C // I2C address for LPS22HHTR

// Register addresses
#define LPS22HH_WHO_AM_I 0x0F
#define LPS22HH_CTRL_REG1 0x10
#define LPS22HH_CTRL_REG2 0x11
#define LPS22HH_CTRL_REG3 0x12
#define LPS22HH_PRESS_OUT_XL 0x28
#define LPS22HH_PRESS_OUT_L 0x29
#define LPS22HH_PRESS_OUT_H 0x2A
#define LPS22HH_TEMP_OUT_L 0x2B
#define LPS22HH_TEMP_OUT_H 0x2C
#define LPS22HH_FIFO_CTRL 0x13
#define LPS22HH_FIFO_STATUS1 0x25
#define LPS22HH_FIFO_STATUS2 0x26
#define LPS22HH_FIFO_DATA_OUT_PRESS_XL 0x78
#define LPS22HH_FIFO_DATA_OUT_PRESS_L 0x79
#define LPS22HH_FIFO_DATA_OUT_PRESS_H 0x7A
#define LPS22HH_FIFO_DATA_OUT_TEMP_L 0x7B
#define LPS22HH_FIFO_DATA_OUT_TEMP_H 0x7C
#define LPS22HH_RPDS_L 0x18 // Offset pressure low register
#define LPS22HH_RPDS_H 0x19 // Offset pressure high register
#define LPS22HH_INT_CFG 0x24 // Interrupt configuration
#define LPS22HH_CALIB_REG 0x30 // Calibration register

class LPS22HH {
public:
    LPS22HH();
    bool begin();
    float readPressure();
    float readTemperature();
    void setODR(uint8_t odr);
    void enableFIFO(uint8_t mode);
    void readFIFOData(int32_t &pressure, int16_t &temperature);
    void setOffset(int16_t offset);
    void calibrateSensor();

private:
    uint8_t readRegister(uint8_t reg);
    void writeRegister(uint8_t reg, uint8_t value);
    void readRawData(int32_t &pressure, int16_t &temperature);
};

#endif // LPS22HH_H
