#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio> // Added for FILE and fopen
#include <cstring> // Added for std::memcmp
#include <sys/stat.h> // Added for stat

// Hex-editing commands
std::string asciiToHex(const std::string& asciiStr) {
    std::string hexStr;
    hexStr.reserve(asciiStr.length() * 2); // Reserve space for the hexadecimal string

    for (char c : asciiStr) {
        unsigned char uc = static_cast<unsigned char>(c); // Convert char to unsigned char
        char hexChar[3]; // Buffer to store the hexadecimal representation (2 characters + null terminator)

        // Format the unsigned char as a hexadecimal string and append it to the result
        std::snprintf(hexChar, sizeof(hexChar), "%02X", uc);
        hexStr += hexChar;
    }

    if (hexStr.length() % 2 != 0) {
        hexStr = '0' + hexStr;
    }

    return hexStr;
}

std::string decimalToHex(const std::string& decimalStr) {
    // assumes the decimal is a 32-bit integer
    std::string hexadecimal = (std::stringstream{} << std::hex << std::setw(8) << std::setfill('0') << std::stoi(decimalStr)).str();

    return hexadecimal;
}

std::string decimalToReversedHex(const std::string& decimalStr, int order = 2) {
    std::string hexadecimal = decimalToHex(decimalStr);

    // Reverse the hexadecimal string in groups of order
    std::string reversedHex;
    reversedHex.reserve(hexadecimal.size());
    for (int i = hexadecimal.length() - order; i >= 0; i -= order) {
        reversedHex += hexadecimal.substr(i, order);
    }

    return reversedHex;
}

std::vector<size_t> findHexDataOffsetsF(FILE* const file, const std::string& hexData);

std::vector<size_t> findHexDataOffsets(const std::string& filePath, const std::string& hexData) {
    // Open the file for reading in binary mode
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        logMessage("Failed to open the file.");
        return {};
    }

    // Check the file size
    struct stat fileStatus;
    if (stat(filePath.c_str(), &fileStatus) != 0) {
        logMessage("Failed to retrieve file size.");
        fclose(file);
        return {};
    }

    const std::vector<size_t> offsets = findHexDataOffsetsF(file, hexData);

    fclose(file);
    return offsets;
}

std::vector<size_t> findHexDataOffsetsF(FILE* const file, const std::string& hexData) {
    // Convert the hex data string to binary data
    std::vector<unsigned char> binaryData;
    for (std::size_t i = 0; i < hexData.length(); i += 2) {
        std::string byteString = hexData.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
        binaryData.push_back(byte);
    }

    // Read the file in chunks to find the offsets where the hex data is located
    const std::size_t bufferSize = 1024;
    unsigned char buffer[bufferSize];
    size_t offset = 0;
    size_t bytesRead = 0;
    std::vector<size_t> offsets;
    while ((bytesRead = fread(buffer, sizeof(unsigned char), bufferSize, file)) > 0) {
        for (size_t i = 0; i < bytesRead; i++) {
            if (std::memcmp(buffer + i, binaryData.data(), binaryData.size()) == 0) {
                std::streampos currentOffset = offset + i;
                offsets.push_back(currentOffset);
            }
        }
        offset += bytesRead;
    }

    return offsets;
}

std::string readHexDataAtOffsetF(FILE* const file, const size_t offset, size_t length);

std::string readHexDataAtOffset(const std::string& filePath, const std::string& hexData, const size_t offsetFromData, size_t length) {
    // logMessage("Entered readHexDataAtOffset");

    // Open the file for reading in binary mode
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        logMessage("Failed to open the file.");
        return "";
    }

    const std::vector<std::size_t> dataOffsets = findHexDataOffsetsF(file, hexData);
    if (dataOffsets.empty()) {
        logMessage("readHexDataAtOffset: data \"" + hexData + "\" not found.");
        fclose(file);
        return "";
    }

    const size_t offset = dataOffsets[0] + offsetFromData;
    const std::string result = readHexDataAtOffsetF(file, offset, length);

    fclose(file);
    return result;
}

std::string readHexDataAtOffsetF(FILE* const file, const size_t offset, const size_t length) {
    // logMessage("Entered readHexDataAtOffsetF");

    if (fseek(file, offset, SEEK_SET) != 0) {
        logMessage("Error seeking to offset.");
        return "";
    }

    char hexBuffer[length];
    std::stringstream hexStream;
    if (fread(hexBuffer, 1, length, file) == length) {
        for (size_t i = 0; i < length; ++i) {
            hexStream << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(hexBuffer[i]);
        }
    } else {
        if (feof(file)) {
            logMessage("End of file reached.");
        } else if (ferror(file)) {
            logMessage("Error reading data from file: ");
            logMessage(std::to_string(errno)); // Print the error description
        }
    }

    std::string result = "";
    char lowerToUpper;
    while (hexStream.get(lowerToUpper)) {
        result += std::toupper(lowerToUpper);
    }

    // logMessage("Hex data at offset:" + result);

    return result;
}

bool hexEditByOffset(const std::string& filePath, const size_t offset, const std::string& hexData) {
    // Open the file for reading and writing in binary mode
    FILE* file = fopen(filePath.c_str(), "rb+");
    if (!file) {
        logMessage("Failed to open the file.");
        return false;
    }

    // Move the file pointer to the specified offset
    if (fseek(file, offset, SEEK_SET) != 0) {
        logMessage("Failed to move the file pointer.");
        fclose(file);
        return false;
    }

    // Convert the hex data string to binary data
    std::vector<unsigned char> binaryData;
    for (std::size_t i = 0; i < hexData.length(); i += 2) {
        std::string byteString = hexData.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
        binaryData.push_back(byte);
    }

    // Calculate the number of bytes to be replaced
    const std::size_t bytesToReplace = binaryData.size();

    // Read the existing data from the file
    std::vector<unsigned char> existingData(bytesToReplace);
    if (fread(existingData.data(), sizeof(unsigned char), bytesToReplace, file) != bytesToReplace) {
        logMessage("Failed to read existing data from the file.");
        fclose(file);
        return false;
    }

    // Move the file pointer back to the offset
    if (fseek(file, offset, SEEK_SET) != 0) {
        logMessage("Failed to move the file pointer.");
        fclose(file);
        return false;
    }

    // Write the replacement binary data to the file
    if (fwrite(binaryData.data(), sizeof(unsigned char), bytesToReplace, file) != bytesToReplace) {
        logMessage("Failed to write data to the file.");
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
    //logMessage("Hex editing completed.");
}

// Is used when mutiple write iterrations are required to reduce the number of file open/close requests
bool hexEditByOffsetF(const std::string& filePath, std::map <std::string,std::string> data) {

    // Open the file for reading and writing in binary mode
    FILE* file = fopen(filePath.c_str(), "rb+");
    if (!file) {
        logMessage("Failed to open the file.");
        return false;
    }

    for(const auto& ov : data) {
        
        // Convert the offset string to std::streampos
        std::streampos offset = std::stoll(ov.first);
        std::string hexData = ov.second;

        // Move the file pointer to the specified offset
        if (fseek(file, offset, SEEK_SET) != 0) {
            logMessage("Failed to move the file pointer.");
            fclose(file);
            return false;
        }

        // Convert the hex data string to binary data
        std::vector<unsigned char> binaryData;
        for (std::size_t i = 0; i < hexData.length(); i += 2) {
            std::string byteString = hexData.substr(i, 2);
            unsigned char byte = static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
            binaryData.push_back(byte);
        }

        // Calculate the number of bytes to be replaced
        std::size_t bytesToReplace = binaryData.size();

        // Read the existing data from the file
        std::vector<unsigned char> existingData(bytesToReplace);
        if (fread(existingData.data(), sizeof(unsigned char), bytesToReplace, file) != bytesToReplace) {
            logMessage("Failed to read existing data from the file.");
            fclose(file);
            return false;
        }

        // Move the file pointer back to the offset
        if (fseek(file, offset, SEEK_SET) != 0) {
            logMessage("Failed to move the file pointer.");
            fclose(file);
            return false;
        }

        // Write the replacement binary data to the file
        if (fwrite(binaryData.data(), sizeof(unsigned char), bytesToReplace, file) != bytesToReplace) {
            logMessage("Failed to write data to the file.");
            fclose(file);
            return false;
        }
    }

    fclose(file);
    return true;
    //logMessage("Hex editing completed.");
}

bool hexEditFindReplace(const std::string& filePath, const std::string& hexDataToReplace, const std::string& hexDataReplacement, const std::string& occurrence = "0") {
    const std::vector<size_t> offsets = findHexDataOffsets(filePath, hexDataToReplace);
    if (!offsets.empty()) {
        if (occurrence == "0") {
            // Replace all occurrences
            for (const size_t offset : offsets) {
                //logMessage("offsetStr: "+offsetStr);
                //logMessage("hexDataReplacement: "+hexDataReplacement);
                hexEditByOffset(filePath, offset, hexDataReplacement);
            }
        }
        else {
            // Convert the occurrence string to an integer
            std::size_t index = std::stoul(occurrence);
            if (index > 0 && index <= offsets.size()) {
                // Replace the specified occurrence/index
                auto offset = offsets[index - 1];
                //logMessage("offsetStr: "+offsetStr);
                //logMessage("hexDataReplacement: "+hexDataReplacement);
                hexEditByOffset(filePath, offset, hexDataReplacement);
            }
            else {
                return false;
                // Invalid occurrence/index specified
                logMessage("Invalid occurrence/index specified.");
            }
        }
        return true;
        //std::cout << "Hex data replaced successfully." << std::endl;
    }
    else {
        return false;
        logMessage("Hex data to replace not found.");
    }
}

bool hexEditCustOffset(const std::string& filePath, const size_t offsetFromCust, const std::string& hexDataReplacement) {
    const std::string CUST = "43555354";
    const std::vector<size_t> custOffsets = findHexDataOffsets(filePath, CUST);
    if (!custOffsets.empty()) {
        const size_t offset = offsetFromCust + custOffsets[0]; // count from "C" letter
        hexEditByOffset(filePath, offset, hexDataReplacement);
    }
    else {
        return false;
        logMessage("CUST not found." );
    }
    return true;
}

int reversedHexToInt(const std::string& hex_str) {
    std::string reversedHex;

    for (int i = hex_str.length() - 2; i >= 0; i -= 2) {
        reversedHex += hex_str.substr(i, 2);
    }
    std::istringstream iss(reversedHex);
    iss >> std::hex;

    int result;
    iss >> result;

    return result;
}