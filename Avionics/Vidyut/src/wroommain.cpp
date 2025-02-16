#include "IIM42653.h"
#include "LPS22HH.h"
#include "FuGPS.h"
#include "madgwickFilter.h"
#include "iir.h"
#include "RYLR993.h"
#include <cmath>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Wire.h>

// Pin Definitions
#define RYLR_RX     1
#define RYLR_TX     2
#define SD_SCK      3
#define SD_MOSI     4
#define SD_MISO     5
#define SD_SS       6
#define IIM_SCK     7
#define IIM_MOSI    8
#define IIM_MISO    9
#define IIM_SS      10
#define LPS_SDA     11
#define LPS_SCL     12
#define GNSS_RX     13
#define GNSS_TX     14
#define MINI_SDA    15
#define MINI_SCL    16

// Constants
#define ALPHA -0.0065f
#define R 287.06f
#define g0 9.80665f
#define T_1 299.45f
#define P_1 100483.97f

#define PRESSURE_BUFFER_SIZE 15
#define MINI_I2C_ADDR 0x08

// Structure for pressure buffer
typedef struct {
    float *buffer;
    int size;
    int head;
} PressureBuffer;

PressureBuffer* pb;

// Structure for Euler angles
typedef struct {
    float roll;
    float pitch;
    float yaw;
} EulerAngles;

EulerAngles orient;

// Enum for different flight stages
typedef enum {
    STANDBY,
    LAUNCH,
    COASTING,
    APOGEE,
    MAIN,
    RECOVERY
} FlightStage;

// Global variables
FlightStage currStage = STANDBY;

// Declare two I2C instances
TwoWire IIM_I2C = TwoWire(0); // I2C port 0
TwoWire MINI_I2C = TwoWire(1); // I2C port 1

IIM42653 imu;
LPS22HH pres;
RYLR993 RLYR(Serial1, RYLR_RX, RYLR_TX, 57600);
Iir::Butterworth::LowPass<2> butterworthFilter;
Storage store;
FuGPS gps;

const float cutoffFrequency = 2.0f;
const float sampleRate = 1000.0f;

IIM42653_axis_t accel_data;
IIM42653_axis_t gyro_data;
float pressure, temperature;
float altAGL = 0.0;
float filteredPressure = 0.0;
bool apogeeByPressure = false;
bool apogeeByOrientation = false;

// Task handles
TaskHandle_t dataCollectionTaskHandle;
TaskHandle_t computationTaskHandle;

// Function prototypes
void dataCollectionTask(void *parameter);
void computationTask(void *parameter);
bool apogeeDetected();

// Transitions the flight state based on sensor data and conditions
void stateTransition() {
    switch (currStage) {
        case STANDBY:
            if (accel_data.y > 10) currStage = LAUNCH;
            store.storeToSD();
            break;
        case LAUNCH:
            if (accel_data.y < 0) currStage = COASTING;
            store.storeToSD();
            break;
        case COASTING:
            if (apogeeDetected()) currStage = APOGEE;
            digitalWrite(DROGUE, HIGH);
            store.storeToSD();
            break;
        case APOGEE:
            if (altAGL < 457) currStage = MAIN;
            digitalWrite(TD, HIGH);
            digitalWrite(DROGUE, LOW);
            store.storeToSD();
            break;
        case MAIN:
            if (gyro_data.x == 0 && gyro_data.y == 0 && gyro_data.z == 0) currStage = RECOVERY;
            digitalWrite(TD, LOW);
            store.storeToSD();
            break;
        case RECOVERY:
            break;
    }
}

// Retrieves GPS data from the serial port and encodes it
void getGPSData() {
    while (Serial2.available()) {
        char c = Serial2.read();
        gps.encode(c);
    }
}

// Retrieves accelerometer and gyroscope data from the IMU
void getIMUData() {
    imu.get_accel_data(&accel_data);
    imu.get_gyro_data(&gyro_data);
}

// Retrieves pressure and temperature data from the pressure sensor
void getPressureData() {
    pressure = pres.readPressure();
    temperature = pres.readTemperature();
}

// Calculates the altitude above ground level (AGL) using pressure data
void calculateAltitude() {
    altAGL = (T_1 / ALPHA) * (pow((filteredPressure / P_1), -((ALPHA * R) / g0)) - 1);
}

// Filters the pressure data using a Butterworth filter and adds it to a buffer
void filterPressure() {
    filteredPressure = butterworthFilter.filter(pressure);
    addReading(filteredPressure);
}

// Detects if apogee has been reached based on pressure and orientation
bool apogeeDetected() {
  if (apogeeByPressure == false && isAscending()) {
    timePressureApogee = millis();
  }

  if (isAscending()) apogeeByPressure = true;
  if ((85 <= orient.pitch && orient.pitch <= 95) || (85 <= orient.yaw && orient.yaw <= 95))
    apogeeByOrientation = true;

  return apogeeByPressure && (apogeeByOrientation || (millis() - timePressureApogee >= 3000));
}

// Initializes the pressure buffer
bool initializeBuffer() {
    pb = (PressureBuffer*)malloc(sizeof(PressureBuffer));
    pb->buffer = (float*)malloc(PRESSURE_BUFFER_SIZE * sizeof(float));
    if (!pb->buffer) {
        Serial.println("Failed to allocate memory for pressure buffer");
        return false;
    }
    pb->size = 0;
    pb->head = 0;
    return true;
}

// Adds a pressure reading to the pressure buffer
void addReading(float pressure) {
    if (!pb || !pb->buffer) {
        Serial.println("Error: Buffer not initialized. Call initializeBuffer() first.");
        return;
    }
    int index = (pb->head + pb->size) % PRESSURE_BUFFER_SIZE;
    pb->buffer[index] = pressure;
    if (pb->size == PRESSURE_BUFFER_SIZE) {
        pb->head = (pb->head + 1) % PRESSURE_BUFFER_SIZE;
    } else {
        pb->size++;
    }
}

// Determines if the pressure readings are ascending in the buffer
bool isAscending() {
    if (pb->size <= 1) {
        return true;
    }
    int prevIndex = pb->head;
    for (int i = 1; i < pb->size; ++i) {
        int currIndex = (pb->head + i) % PRESSURE_BUFFER_SIZE;
        if (pb->buffer[currIndex] < pb->buffer[prevIndex]) {
            return false;
        }
        prevIndex = currIndex;
    }
    return true;
}

// Calibrates the initial orientation using accelerometer data
void calibrateInitialOrientation() {
  float sum_ax = 0.0f, sum_ay = 0.0f, sum_az = 0.0f;
  int count = 0;

  while (count < num) {
    getIMUData();
    sum_ax += accel_data.x;
    sum_ay += accel_data.y;
    sum_az += accel_data.z;
    count++;
  }

  // Calculate average acceleration values
  float avg_ax = sum_ax / num;
  float avg_ay = sum_ay / num;
  float avg_az = sum_az / num;

  // Calculate initial roll and pitch (assuming X-axis is along the rocket axis)
  float initial_roll  = atan2f(avg_ay, avg_az); // Roll around X-axis
  float initial_pitch = atan2f(-avg_ax, sqrtf(avg_ay * avg_ay + avg_az * avg_az)); // Pitch around Y-axis
  initial_yaw_offset = 0.0f; //Initialize to 0

  // Create a quaternion representing the initial orientation
  struct quaternion initial_q;
  float cy = cosf(initial_yaw_offset * 0.5f);
  float sy = sinf(initial_yaw_offset * 0.5f);
  float cp = cosf(initial_pitch * 0.5f);
  float sp = sinf(initial_pitch * 0.5f);
  float cr = cosf(initial_roll * 0.5f);
  float sr = sinf(initial_roll * 0.5f);

  initial_q.q1 = cr * cp * cy + sr * sp * sy;
  initial_q.q2 = sr * cp * cy - cr * sp * sy;
  initial_q.q3 = cr * sp * cy + sr * cp * sy;
  initial_q.q4 = cr * cp * sy - sr * sp * cy;

  // Normalize the quaternion
  quat_Normalization(&initial_q);

  // Set the initial quaternion estimate (q_est)
  q_est = initial_q;
}

// Calculates vertical acceleration based on Euler angles and accelerometer data
float calculateVerticalAccel(float roll, float pitch, float yaw, float x_accel, float y_accel, float z_accel) {
 // Create rotation matrix (ZYX convention)
  float cosRoll = cosf(roll);
  float sinRoll = sinf(roll);
  float cosPitch = cosf(pitch);
  float sinPitch = sinf(pitch);
  float cosYaw = cosf(yaw);
  float sinYaw = sinf(yaw);... //Rotation Matrix from body frame to world frame (ZYX convention)
  float rotationMatrix[3][3] = {
      {cosPitch * cosYaw, cosPitch * sinYaw, -sinPitch},
      {sinRoll * sinPitch * cosYaw - cosRoll * sinYaw, sinRoll * sinPitch * sinYaw + cosRoll * cosYaw, sinRoll * cosPitch},
      {cosRoll * sinPitch * cosYaw + sinRoll * sinYaw, cosRoll * sinPitch * sinYaw - sinRoll * cosYaw, cosRoll * cosPitch}
  };

  // Transform acceleration vector from body frame to world frame
  float world_z_accel = rotationMatrix[2][0] * x_accel + rotationMatrix[2][1] * y_accel + rotationMatrix[2][2] * z_accel;
  return world_z_accel;
}

// Initializes the RYLR module with a given address
void RYLRinit(uint16_t add) {
  RLYR.begin();
  RLYR.setAddress(add);
}

// Sends data to the Mini via I2C
void sendDataToMini() {
    char dataBuffer[50]; // Buffer to hold formatted data string
    snprintf(dataBuffer, sizeof(dataBuffer), "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f",currStage, 
             altAGL, pressure, temperature, orient.roll, orient.pitch, orient.yaw);

    MINI_I2C.beginTransmission(MINI_I2C_ADDR);
    MINI_I2C.write(dataBuffer);  // Send data as string
    MINI_I2C.endTransmission();
    
    Serial.print("Sent to Mini: ");
    Serial.println(dataBuffer);
}

// Collects sensor data and transmits it
void dataCollection() {
    getIMUData();
    getPressureData();
    store.storeData(pressure, accel_data.x, accel_data.y, accel_data.z, gyro_data.x, gyro_data.y, gyro_data.z, temperature);
    getGPSData();
    String data = String(currStage) + ";" + String(pressure) + ";" + String(accel_data.x) + ";" + String(accel_data.y) + ";" + String(accel_data.z)+ ";" + String(gyro_data.x) + ";" + String(gyro_data.y) + ";" + String(gyro_data.z) + ";" + String(gps.longitude) + ";" + String(gps.latitude);
    RYLR.sendMessage(0,data);
    vTaskDelay(pdMS_TO_TICKS(10));
}

// Task running on core 0 for data collection and transmission
void core0Task(void *parameter) {
    while (1) {
        switch(currStage) {
            case STANDBY:
            break;
            case RECOVERY:
            getGPSData();
            RYLR.sendMessage(String(gps.latitude) + "," + String(gps.longitude));
            break;
            default:
            dataCollection();
            sendDataToMini();
        }
    }
}

// Task running on core 1 for computation
void core1Task(void *parameter) {
    while (1) {
        switch(currStage) {
            case STANDBY:
            break;
            case RECOVERY:
            break;
            default:
            computationTask();
        }
    }
}

// Performs computation tasks such as filtering pressure, calculating altitude, and transitioning states
void computationTask(void *parameter) {
    filterPressure();
    calculateAltitude();
    stateTransition();
}

// Initializes the sensors, modules, and FreeRTOS tasks
void setup() {
    Serial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

    // Initialize I2C instances
    IIM_I2C.begin(IIM_SDA, IIM_SCL, 100000); // SDA, SCL, clock frequency
    MINI_I2C.begin(MINI_SDA, MINI_SCL, 100000);

    // Initialize IMU with the dedicated I2C port
    if (!imu.begin(IIM_I2C, IIM42653_SET_DEV_ADDR)) {
        Serial.println("Failed to initialize IIM42653!");
        while (1);
    }
    Serial.println("IIM42653 initialized successfully!");
    imu.accelerometer_enable();
    imu.gyroscope_enable();
    calibrateInitialOrientation();

    if (!pres.begin()) {
        Serial.println("Failed to initialize LPS22HH!");
        while (1);
    }
    Serial.println("LPS22HH initialized successfully!");
    pres.setODR(75);
    pres.calibrateSensor();

    RYLRinit(56);
    Serial2.begin(9600, SERIAL_8N1, GNSS_RX, GNSS_TX);

    butterworthFilter.setup(sampleRate, cutoffFrequency);
    initializeBuffer();
    store.begin();
    
    // Create FreeRTOS tasks for dual-core execution
    xTaskCreatePinnedToCore(core0Task, "Data Collection", 4096, NULL, 1, &dataCollectionTaskHandle, 0);
    xTaskCreatePinnedToCore(core1Task, "Computation", 4096, NULL, 1, &computationTaskHandle, 1);
}

// The main loop (empty)
void loop() {
}