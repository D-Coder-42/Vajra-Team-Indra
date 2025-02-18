#ifndef CONTROLS_H
#define CONTROLS_H

#include <math.h>
#include "ESP32Servo.h"

// Mathematical constant PI
#define PI 3.14159265f

// Rocket physical properties
const float DRY_MASS = 25280.00;                 // Dry mass in kg
const float FRONTAL_AREA = PI * (14.4) * (14.4) / (10000); // Frontal area in m²
const float Cd = 0.44;                           // Drag coefficient of rocket

// Coefficients
const float p1 = 3.4338e-09;
const float p2 = -1.0595e-04;
const float p3 = 1.1645;
const float x = p1 / 3;
const float y = p2 / 2;
const float z = p3;

// Function prototypes
float intV(float vel);                           // Integrate velocity
float intS(float s);                             // Integrate displacement
float inverseFunc(float s);                      // Inverse function for position
float predictApogee();                           // Predict apogee
void controlAirbrakes();                         // Airbrake control logic
void extendAirbrakes();                          // Extend airbrakes
void retractAirbrakes();                         // Retract airbrakes

#endif // CONTROLS_H
