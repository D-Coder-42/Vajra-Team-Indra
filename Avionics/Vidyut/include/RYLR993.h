#ifndef RYLR993_H
#define RYLR993_H

#include <Arduino.h>

class RYLR993 {
public:
    RYLR993(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin, uint32_t baudRate = 57600); // Constructor
    void begin(); // Initialize communication
    bool setAddress(uint16_t address); // Set device address
    bool setNetworkID(uint8_t id); // Set network ID
    bool setRFParameters(uint32_t frequency, uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate); // Set RF parameters
    bool setTransmitPower(uint8_t power); // Set transmit power
    bool sendMessage(uint16_t destination, const String &message); // Send a message
    String receiveMessage(); // Receive a message
    bool setUARTBaudRate(uint32_t baudrate); // Set UART baud rate
    bool setAESKey(const String &key); // Set AES encryption key
    bool enableRelayMode(bool enable); // Enable relay mode
    bool setParametersToDefault(); // Reset to default settings
    String getFirmwareVersion(); // Get firmware version

private:
    HardwareSerial &serial; // Serial reference
    uint8_t rxPin, txPin;   // RX and TX pins
    uint32_t baudRate;      // Baud rate
    String sendCommand(const String &command, uint32_t timeout = 100); // Send command and wait for response
};

#endif // RYLR993_H
