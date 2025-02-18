#include "LPS22HH.h"

// LPS22HH constructor
LPS22HH::LPS22HH() {}

// Initialize the sensor and check connectivity
bool LPS22HH::begin() {
    Wire.begin();
    setODR(75);
    uint8_t whoAmI = readRegister(LPS22HH_WHO_AM_I);
    return (whoAmI == 0xB3); // Expected WHO_AM_I response
}

// Read and return the pressure value
float LPS22HH::readPressure() {
    int32_t pressure_raw;
    int16_t temp_raw;
    readRawData(pressure_raw, temp_raw);
    return pressure_raw / 4096.0; // Convert to hPa
}

// Read and return the temperature value
float LPS22HH::readTemperature() {
    int32_t pressure_raw;
    int16_t temp_raw;
    readRawData(pressure_raw, temp_raw);
    return temp_raw / 100.0; // Convert to °C
}

// Set the output data rate (ODR)
void LPS22HH::setODR(uint8_t odr) {
    writeRegister(LPS22HH_CTRL_REG1, (odr & 0x70));
}

// Enable FIFO mode with specified settings
void LPS22HH::enableFIFO(uint8_t mode) {
    writeRegister(LPS22HH_FIFO_CTRL, mode);
}

// Read data from FIFO buffer
void LPS22HH::readFIFOData(int32_t &pressure, int16_t &temperature) {
    pressure = (readRegister(LPS22HH_FIFO_DATA_OUT_PRESS_H) << 16) |
               (readRegister(LPS22HH_FIFO_DATA_OUT_PRESS_L) << 8) |
               readRegister(LPS22HH_FIFO_DATA_OUT_PRESS_XL);
    temperature = (readRegister(LPS22HH_FIFO_DATA_OUT_TEMP_H) << 8) |
                  readRegister(LPS22HH_FIFO_DATA_OUT_TEMP_L);
}

// Set pressure offset for calibration
void LPS22HH::setOffset(int16_t offset) {
    writeRegister(LPS22HH_RPDS_L, offset & 0xFF);
    writeRegister(LPS22HH_RPDS_H, (offset >> 8) & 0xFF);
}

// Calibrate the sensor
void LPS22HH::calibrateSensor() {
    writeRegister(LPS22HH_CALIB_REG, 0x01); // Start calibration
    delay(100); // Allow time for calibration
}

// Read a byte from a specified register
uint8_t LPS22HH::readRegister(uint8_t reg) {
    Wire.beginTransmission(LPS22HH_ADDRESS);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(LPS22HH_ADDRESS, (uint8_t)1);
    return Wire.read();
}

// Write a byte to a specified register
void LPS22HH::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(LPS22HH_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

// Read raw pressure and temperature data
void LPS22HH::readRawData(int32_t &pressure, int16_t &temperature) {
    pressure = (readRegister(LPS22HH_PRESS_OUT_H) << 16) |
               (readRegister(LPS22HH_PRESS_OUT_L) << 8) |
               readRegister(LPS22HH_PRESS_OUT_XL);
    temperature = (readRegister(LPS22HH_TEMP_OUT_H) << 8) |
                  readRegister(LPS22HH_TEMP_OUT_L);
}
