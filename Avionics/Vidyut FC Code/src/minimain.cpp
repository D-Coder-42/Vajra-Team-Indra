#include <Wire.h>
#include <EEPROM.h>
#include <ESP32Servo.h>
#include "Controls.h"

#define GAMMA 1.4f
#define R 287.06f

#define I2C_SLAVE_ADDR 0x08  // I2C address of ESP32-S3-MINI
#define PITOT1_PIN 13       // Analog pin for sensor 1
#define PITOT2_PIN 14       // Analog pin for sensor 2
#define EEPROM_SIZE 512      // Max EEPROM size for storing data

const int servoPin = 15;
const int EXTEND_ANGLE = 90;
const int RETRACT_ANGLE = 0;

typedef enum {
    STANDBY,
    LAUNCH,
    COASTING,
    APOGEE,
    MAIN,
    RECOVERY
} FlightStage;

FlightStage currStage = STANDBY;
Servo airbrakeServo;

int pitot1_ADC = 0;
int pitot2_ADC = 0;
float pitot1_pressure = 0.0;
float pitot2_pressure = 0.0;
char received_data[64];
float altitude = 0.0;
float velocity = 0.0;
float temp = 0.0;
float staticPressure = 0.0;
float dynamicPressure = 0.0;
float euler_angles[3] = {0.0, 0.0, 0.0};
int eeprom_address = 0;
float apogee = 3048.0;

float convToPressure(int ADC) {
    return (ADC * 5000 / (45 * 4095)) * 1000; // Convert ADC value to pressure (Pa)
}

float calculateVelocity() {
    float a = sqrt(GAMMA * R * temp);
    return ((2 * a * a) / (GAMMA - 1)) * (pow(((dynamicPressure / staticPressure) + 1), (1 - (1 / GAMMA))) - 1);
}

void receiveEvent(int bytes) {
    int i = 0;
    while (Wire.available() && i < sizeof(received_data) - 1) {
        received_data[i++] = Wire.read();
    }
    received_data[i] = '\0'; // Null terminate the string
    Serial.print("Received from Master: ");
    Serial.println(received_data);
    
    sscanf(received_data, "%d,%f,%f,%f,%f,%f,%f", &currStage, &altitude, &staticPressure, &temp, &euler_angles[0], &euler_angles[1], &euler_angles[2]);
}

void requestEvent() {
    String data = String(pitot1_pressure) + "," + String(pitot2_pressure); // Use correct variables
    Wire.write(data.c_str());
}

void storeToEEPROM() {
    EEPROM.put(eeprom_address, pitot1_pressure);
    eeprom_address += sizeof(pitot1_pressure);
    EEPROM.put(eeprom_address, pitot2_pressure);
    eeprom_address += sizeof(pitot2_pressure);
    EEPROM.put(eeprom_address, altitude);
    eeprom_address += sizeof(altitude);
    
    if (eeprom_address >= EEPROM_SIZE) eeprom_address = 0; // Loop back if EEPROM is full
    EEPROM.commit();
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SLAVE_ADDR); // Start I2C as a slave
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    
    analogReadResolution(12);
    pinMode(PITOT1_PIN, INPUT);
    pinMode(PITOT2_PIN, INPUT);
    
    airbrakeServo.attach(servoPin, 500, 2500); // Servo pulse width range is 500 to 2500 µs
    airbrakeServo.write(RETRACT_ANGLE); // Initial position at RETRACT_ANGLE
    EEPROM.begin(EEPROM_SIZE); // Initialize EEPROM only once
}

void loop() {
    pitot1_ADC = analogRead(PITOT1_PIN);
    pitot2_ADC = analogRead(PITOT2_PIN);
    
    pitot1_pressure = convToPressure(pitot1_ADC);
    pitot2_pressure = convToPressure(pitot2_ADC);
    
    dynamicPressure = (pitot1_pressure + pitot2_pressure) / 2.0;

    velocity = calculateVelocity();

    apogee = predictApogee(velocity, apogee);
    if (altitude > 1981 && currStage == COASTING) controlAirbrakes();
    
    storeToEEPROM();
    
    delay(100); // Adjust delay as needed
}
