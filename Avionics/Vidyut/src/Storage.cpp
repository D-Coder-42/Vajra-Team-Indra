#include "Storage.h"

// Constructor to initialize storage parameters
Storage::Storage() : dataCount(0), bufferSize(MAX_DATA_POINTS * 100), sdCS(5), fileName("data_log.txt") {
    psramBuffer = (char *)ps_malloc(bufferSize); // Allocate PSRAM buffer
    if (!psramBuffer) {
        Serial.println("PSRAM allocation failed!"); // Check for allocation failure
    }
}

// Initialize storage and SD card
bool Storage::begin() {
    if (!psramBuffer) return false; // Check PSRAM allocation
    if (!SD.begin(sdCS)) {
        Serial.println("SD card initialization failed!"); // Check SD card initialization
        return false;
    }
    return true;
}

// Store sensor data in PSRAM buffer
void Storage::storeData(uint32_t timestamp, float pressure, float accelX, float accelY, float accelZ, float gyroX, float gyroY, float gyroZ, float temperature) {
    if (dataCount >= MAX_DATA_POINTS) {
        Serial.println("PSRAM buffer full. Auto writing to SD."); // Auto write if buffer is full
        writeToSD();
    }
    
    size_t offset = dataCount * 100; // Calculate offset for new data
    snprintf(psramBuffer + offset, 100, "%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n", timestamp, pressure, accelX, accelY, accelZ, gyroX, gyroY, gyroZ, temperature);
    dataCount++; // Increment data count
}

// Write stored data from PSRAM to SD card
void Storage::writeToSD() {
    if (!SD.begin(sdCS)) {
        Serial.println("SD card initialization failed!"); // Check SD card initialization
        return;
    }

    logFile = SD.open(fileName, FILE_WRITE); // Open file for writing
    if (!logFile) {
        Serial.println("Failed to open file on SD!"); // Check file opening
        return;
    }

    size_t bytesWritten = 0; // Track bytes written
    while (bytesWritten < dataCount * 100) {
        size_t bytesToWrite = std::min<size_t>(CHUNK_SIZE, static_cast<size_t>((dataCount * 100) - bytesWritten));
        logFile.write(reinterpret_cast<const uint8_t*>(psramBuffer + bytesWritten), bytesToWrite); // Write chunk to file
        bytesWritten += bytesToWrite; // Update written bytes count
    }

    logFile.close(); // Close the file
    Serial.println("Data written to SD card in chunks.");

    dataCount = 0; // Clear PSRAM buffer after writing
    Serial.println("PSRAM cleared.");
}

// Trigger a write operation if there is data in the buffer
void Storage::triggerWrite() {
    if (dataCount > 0) {
        writeToSD(); // Write to SD if there is data
    }
}

// Set chip select pin for SD card communication
void Storage::setCSpin(int cs) {
    sdCS = cs;
}

// Set the filename for logging data
void Storage::setFileName(String name) {
    fileName = name;
}
