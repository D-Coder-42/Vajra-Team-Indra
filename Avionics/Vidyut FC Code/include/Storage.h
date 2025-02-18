#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <SD.h>

#define MAX_DATA_POINTS 10000 // Maximum data points to store
#define CHUNK_SIZE 512 * 1024 // Chunk size for SD write (512 KB)

class Storage {
public:
    Storage(); // Constructor
    bool begin(); // Initialize storage
    void storeData(uint32_t timestamp, float pressure, float accelX, float accelY, float accelZ, float gyroX, float gyroY, float gyroZ, float temperature); // Store sensor data
    void writeToSD(); // Write stored data to SD card
    void triggerWrite(); // Trigger a write operation
    void setCSpin(int cs); // Set chip select pin for SD card
    void setFileName(String name); // Set the filename for logging

private:
    char *psramBuffer; // Buffer for storing data in PSRAM
    size_t dataCount; // Count of stored data points
    size_t bufferSize; // Size of the buffer
    File logFile; // File object for SD card operations
    int sdCS; // Chip select pin for SD card
    String fileName; // Name of the log file
};

#endif // STORAGE_H