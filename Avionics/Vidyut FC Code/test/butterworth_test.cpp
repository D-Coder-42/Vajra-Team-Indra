#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#define IIR_NOTCH
#define IIR_BUTTERWORTH  // Enable Butterworth filter in iir1 library
#include "Iir.h"          // Include the IIR filter library

// Function to read pressure data from a CSV file
std::vector<double> readPressureData(const std::string& filename) {
    std::vector<double> pressureData;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return pressureData;
    }

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;
        int columnIndex = 0;

        while (std::getline(ss, value, ',')) {
            if (columnIndex == 1) {  // Assuming pressure data is in the second column
                try {
                    pressureData.push_back(std::stod(value));
                } catch (...) {
                    std::cerr << "Invalid data: " << value << std::endl;
                }
            }
            columnIndex++;
        }
    }

    file.close();
    return pressureData;
}

// Function to write filtered pressure data to a CSV file
void writeFilteredData(const std::string& filename, const std::vector<double>& filteredData) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    for (const auto& value : filteredData) {
        file << value << std::endl;
    }

    file.close();
}

int main() {
    // Read pressure data
    std::vector<double> pressureData = readPressureData("no_hopes_l.csv");

    if (pressureData.empty()) {
        std::cerr << "No pressure data found." << std::endl;
        return 1;
    }

    // Create a Butterworth filter (order 2, sample rate 100 Hz, cutoff 10 Hz)
    Iir::Butterworth::LowPassBase<2> butterworthFilter;
    butterworthFilter.setup(50.0, 10.0);  // Adjust sample rate and cutoff as needed

    // Apply the filter
    std::vector<double> filteredData;
    for (const auto& pressure : pressureData) {
        double filteredValue = butterworthFilter.filter(pressure);
        filteredData.push_back(filteredValue);
    }

    // Write filtered data
    writeFilteredData("filteredpressure.csv", filteredData);
    std::cout << "Filtered pressure data has been written to filteredpressure.csv" << std::endl;

    return 0;
}
