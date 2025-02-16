#include "RYLR993.h"

// Constructor to initialize the RYLR993 with serial parameters
RYLR993::RYLR993(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin, uint32_t baudRate)
    : serial(serial), rxPin(rxPin), txPin(txPin), baudRate(baudRate) {}

// Initialize communication with the module
void RYLR993::begin() {
    serial.begin(baudRate, SERIAL_8N1, rxPin, txPin);
    sendCommand("AT+OPMODE=1"); // Set operating mode
}

// Set the device address
bool RYLR993::setAddress(uint16_t address) {
    return sendCommand("AT+ADDRESS=" + String(address)).indexOf("OK") != -1;
}

// Set the network ID for communication
bool RYLR993::setNetworkID(uint8_t id) {
    return sendCommand("AT+NETWORKID=" + String(id)).indexOf("OK") != -1;
}

// Configure RF parameters such as frequency and spreading factor
bool RYLR993::setRFParameters(uint32_t frequency, uint8_t spreadingFactor, uint8_t bandwidth, uint8_t codingRate) {
    return sendCommand("AT+PARAMETER=" + String(frequency) + "," + String(spreadingFactor) + "," + String(bandwidth) + "," + String(codingRate)).indexOf("OK") != -1;
}

// Set the transmit power level
bool RYLR993::setTransmitPower(uint8_t power) {
    return sendCommand("AT+POWER=" + String(power)).indexOf("OK") != -1;
}

// Send a message to a specified destination
bool RYLR993::sendMessage(uint16_t destination, const String &message) {
    return sendCommand("AT+SEND=" + String(destination) + "," + String(message.length()) + "," + message).indexOf("OK") != -1;
}

// Receive a message from the module
String RYLR993::receiveMessage() {
    if (serial.available()) {
        return serial.readString();
    }
    return "";
}

// Set the UART baud rate for communication
bool RYLR993::setUARTBaudRate(uint32_t baudrate) {
    return sendCommand("AT+UART=" + String(baudrate)).indexOf("OK") != -1;
}

// Set the AES encryption key for secure communication
bool RYLR993::setAESKey(const String &key) {
    return sendCommand("AT+AESKEY=" + key).indexOf("OK") != -1;
}

// Enable or disable relay mode
bool RYLR993::enableRelayMode(bool enable) {
    return sendCommand("AT+RELAY=" + String(enable ? 1 : 0)).indexOf("OK") != -1;
}

// Reset parameters to default settings
bool RYLR993::setParametersToDefault() {
    return sendCommand("AT+DEFAULT").indexOf("OK") != -1;
}

// Get the firmware version of the module
String RYLR993::getFirmwareVersion() {
    return sendCommand("AT+VERSION");
}

// Send a command to the module and wait for a response
String RYLR993::sendCommand(const String &command, uint32_t timeout) {
    serial.print(command + "\r");
    unsigned long start = millis();
    while (millis() - start < timeout) {
        if (serial.available()) {
            return serial.readString();
        }
    }
    return ""; // Return empty string if no resp
}