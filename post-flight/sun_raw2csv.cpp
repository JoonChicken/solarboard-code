/***
 * Converts a raw binary file outputted by the sun to the sd card to a regular csv
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string.h>

struct BarometerData {
    float baro_temp;
    float pressure;
};

struct IMUData {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
};

struct INAData {
    int32_t current_raw;
    float current;
};

struct SolarBoardData {
    int32_t time_ms;
    BarometerData baro_data;
    IMUData imu_data;
    INAData ina1_data;
    INAData ina2_data;
    INAData ina3_data;
    INAData ina4_data;
};
size_t SBDATA_SIZE = sizeof(SolarBoardData);


int main() {
    std::string infilename;
    std::ifstream in;
    bool firsttime = true;
    while (firsttime || in.fail()) {
        if (!firsttime) {
            std::cout << "ERROR: File does not exist. Try again\n";
        }

        std::cout << "Enter filename to convert .raw file produced by The Sun to a .csv\n> ";
        std::cin >> infilename;
        in.open(infilename);
        firsttime = false;
    }

    std::string outfilename = infilename.substr(0, infilename.find(".")) + ".csv";
    std::cout << "outfilename will be called \"" << outfilename << "\"\n";
    std::ofstream out;
    out.open(outfilename);

    
    std::string num_bytes_str;
    std::getline(in, num_bytes_str);
    int num_bytes = std::stoi(num_bytes_str);
    std::cout << "Size is: " << num_bytes << " bytes.";
    if (num_bytes == SBDATA_SIZE) {
        std::cout << " Matches with known SolarBoardData size\n";
    } else {
        std::cout << "\nERROR: Mismatch between data size in SolarBoardData and in file metadata\n";
        return 1;
    } 


    std::string header;
    std::getline(in, header);
    std::cout << "Header is: \"" << header << "\"\n";
    out << header << "\n";


    SolarBoardData sbdata;
    while (in.read((char *)&sbdata, SBDATA_SIZE)) {
        out << sbdata.time_ms << ", "
            << sbdata.ina1_data.current << ", " << sbdata.ina2_data.current << ", " << sbdata.ina3_data.current << ", " << sbdata.ina4_data.current << ", "
            << sbdata.imu_data.ax << ", " << sbdata.imu_data.ay << ", " << sbdata.imu_data.az << ", "
            << sbdata.imu_data.gx << ", " << sbdata.imu_data.gy << ", " << sbdata.imu_data.gz << ", "
            << sbdata.baro_data.baro_temp << ", " << sbdata.baro_data.pressure << "\n";
    }

    std::cout << "Write finished. Cleaning...\n";

    in.close();
    out.close();
    return 0;
}