#include "Controls.h"

extern Servo airbrakeServo; // Declare external servo for airbrakes

// Extend airbrakes
void extendAirbrakes() {
    airbrakeServo.write(EXTEND_ANGLE); // Move servo to extend position
}

// Retract airbrakes
void retractAirbrakes() {
    airbrakeServo.write(RETRACT_ANGLE); // Move servo to retract position
}

// Integrate velocity to calculate aerodynamic drag
float intV(float vel) {
    return (2 * DRY_MASS) / FRONTAL_AREA * log(vel);
}

// Integrate displacement for aerodynamic calculations
float intS(float s) {
    return p1 * pow(s, 3) / 3 + p2 * pow(s, 2) / 2 + p3 * s;
}

// Calculate position using inverse function based on displacement
float inverseFunc(float s) {
    float part1 = s / (2 * x) - (y * y * y) / (27 * x * x * x) + (y * z) / (6 * x * x);
    float part2 = (y * y) / (9 * x * x) - z / (3 * x);

    float intTerm = sqrt(part1 * part1 - part2 * part2 * part2);
    float sigma1 = pow((intTerm + part1), (1.0 / 3.0));

    return (part2 / sigma1 - y / (3 * x) + sigma1);
}

// Predict the apogee based on current velocity and altitude
float predictApogee(float vel, float alt) {
    float apogeeFunc = intV(vel) - intV(0) + intS(alt);
    return inverseFunc(apogeeFunc);
}

// Control airbrakes based on predicted apogee
void controlAirbrakes() {
    const float APOGEE_THRESHOLD = 10100 * 0.3048; // Convert feet to meters
    if (predictApogee() > APOGEE_THRESHOLD) {
        extendAirbrakes(); // Deploy airbrakes if above threshold
    } else {
        retractAirbrakes(); // Retract airbrakes if below threshold
    }
}
