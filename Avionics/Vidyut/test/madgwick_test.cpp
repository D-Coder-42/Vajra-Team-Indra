#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "madgwickFilter.h"

#define DELTA_T 0.01f

int main() {
    std::ifstream inputFile("no_hopes_l.csv");
    std::ofstream outputFile("euler_angles_output.csv");

    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open input file!\n";
        return 1;
    }

    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open output file!\n";
        return 1;
    }

    // Write header to output file
    outputFile << "roll,pitch,yaw" << std::endl;

    std::string line;
    std::getline(inputFile, line); // Skip the header line in the input file

    float roll = 0.0f, pitch = 0.0f, yaw = 0.0f; // Define roll, pitch, and yaw outside the loop

    while (std::getline(inputFile, line)) {
        std::stringstream ss(line);
        std::vector<float> values;
        std::string token;

        // Read each column value from the line
        while (std::getline(ss, token, ',')) {
            values.push_back(std::stof(token));
        }

        if (values.size() != 8) {
            std::cerr << "Error: Incorrect data format in input file!\n";
            continue;
        }

        // Extract accelerometer and gyroscope values
        float ax = values[4], ay = values[5], az = values[6];
        // float gx = values[3], gy = values[4], gz = values[5];
        float  gx = 0, gy = 0, gz = 0;

        // Call imu_filter with the current data
        imu_filter(ax, ay, az, gx, gy, gz);

        // Compute Euler angles
        eulerAngles(q_est, &roll, &pitch, &yaw);

        // Write the Euler angles to the output file
        outputFile << roll << "," << pitch << "," << yaw << std::endl;
    }

    inputFile.close();
    outputFile.close();

    std::cout << "Processing complete. Output saved to euler_angles_output.csv.\n";
    return 0;
}
