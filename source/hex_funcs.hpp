#pragma once

#include <sys/stat.h> // Added for stat

#include <algorithm>
#include <cstdio> // Added for FILE and fopen
#include <cstring> // Added for std::memcmp
#include <string>
#include <vector>

// Hex-editing commands
std::string asciiToHex(const std::string& asciiStr)
{
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

std::string decimalToHex(const std::string& decimalStr)
{
    // assumes the decimal is a 32-bit integer
    return (std::stringstream {} << std::hex << std::setw(8) << std::setfill('0') << std::stoi(decimalStr)).str();
}

std::string decimalToReversedHex(const std::string& decimalStr, int order = 2)
{
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

std::vector<size_t> findHexDataOffsets(const std::string& filePath, const std::string& hexData)
{
    // Open the file for reading in binary mode
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        log("Failed to open the file.");
        return {};
    }

    // Check the file size
    struct stat fileStatus;
    if (stat(filePath.c_str(), &fileStatus) != 0) {
        log("Failed to retrieve file size.");
        fclose(file);
        return {};
    }

    const std::vector<size_t> offsets = findHexDataOffsetsF(file, hexData);

    fclose(file);
    return offsets;
}

std::vector<size_t> findHexDataOffsetsF(FILE* const file, const std::string& hexData)
{
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

std::string readHexDataAtOffset(const std::string& filePath, const std::string& hexData, const size_t offsetFromData, size_t length)
{
    // log("Entered readHexDataAtOffset");

    // Open the file for reading in binary mode
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        log("Failed to open the file.");
        return "";
    }

    const std::vector<std::size_t> dataOffsets = findHexDataOffsetsF(file, hexData);
    if (dataOffsets.empty()) {
        log("readHexDataAtOffset: data \"%s\" not found.", hexData.c_str());
        fclose(file);
        return "";
    }

    const size_t offset = dataOffsets[0] + offsetFromData;
    const std::string result = readHexDataAtOffsetF(file, offset, length);

    fclose(file);
    return result;
}

FILE* openFile(const std::string& filePath)
{

    // Open the file for reading in binary mode
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        log("Failed to open the file.");
        return nullptr;
    }
    return file;
}

long findCustOffset(FILE* const file)
{
    const std::vector<std::size_t> dataOffsets = findHexDataOffsetsF(file, "43555354");
    if (dataOffsets.empty()) {
        log("readHexDataAtOffset: data \"%s\" not found.", "CUST");
        return -1;
    }
    return dataOffsets[0];
}

std::string readHexDataAtOffset(FILE* const file, const std::string& hexData, const size_t offsetFromData, size_t length, int custOffset = -1)
{
    // log("Entered readHexDataAtOffset");

    if (!file) {
        log("Failed to open the file.");
        return "";
    }

    if (custOffset == -1) {
        const std::vector<std::size_t> dataOffsets = findHexDataOffsetsF(file, hexData);
        if (dataOffsets.empty()) {
            log("readHexDataAtOffset: data \"%s\" not found.", hexData.c_str());
            return "";
        }
        custOffset = dataOffsets[0];
    }

    const size_t offset = custOffset + offsetFromData;
    const std::string result = readHexDataAtOffsetF(file, offset, length);

    fseek(file, 0, SEEK_SET);

    return result;
}

std::string readHexDataAtOffsetF(FILE* const file, const size_t offset, const size_t length)
{
    // log("Entered readHexDataAtOffsetF");

    if (fseek(file, offset, SEEK_SET) != 0) {
        log("Error seeking to offset.");
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
            log("End of file reached.");
        } else if (ferror(file)) {
            log("Error reading data from file: %s", strerror(errno));
        }
    }

    std::string result = "";
    char lowerToUpper;
    while (hexStream.get(lowerToUpper)) {
        result += std::toupper(lowerToUpper);
    }

    // log("Hex data at offset:" + result);

    return result;
}

bool hexEditByOffset(const std::string& filePath, const size_t offset, const std::string& hexData)
{
    // Open the file for reading and writing in binary mode
    FILE* file = fopen(filePath.c_str(), "rb+");
    if (!file) {
        log("Failed to open the file.");
        return false;
    }

    // Move the file pointer to the specified offset
    if (fseek(file, offset, SEEK_SET) != 0) {
        log("Failed to move the file pointer.");
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
        log("Failed to read existing data from the file.");
        fclose(file);
        return false;
    }

    // Move the file pointer back to the offset
    if (fseek(file, offset, SEEK_SET) != 0) {
        log("Failed to move the file pointer.");
        fclose(file);
        return false;
    }

    // Write the replacement binary data to the file
    if (fwrite(binaryData.data(), sizeof(unsigned char), bytesToReplace, file) != bytesToReplace) {
        log("Failed to write data to the file.");
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

// Is used when mutiple write iterrations are required to reduce the number of file open/close requests
bool hexEditByOffsetF(const std::string& filePath, const std::map<std::string, std::string>& data)
{

    // Open the file for reading and writing in binary mode
    FILE* file = fopen(filePath.c_str(), "rb+");
    if (!file) {
        log("Failed to open the file.");
        return false;
    }

    for (const auto& ov : data) {

        // Convert the offset string to std::streampos
        std::streampos offset = std::stoll(ov.first);
        std::string hexData = ov.second;

        // Move the file pointer to the specified offset
        if (fseek(file, offset, SEEK_SET) != 0) {
            log("Failed to move the file pointer.");
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
            log("Failed to read existing data from the file.");
            fclose(file);
            return false;
        }

        // Move the file pointer back to the offset
        if (fseek(file, offset, SEEK_SET) != 0) {
            log("Failed to move the file pointer.");
            fclose(file);
            return false;
        }

        // Write the replacement binary data to the file
        if (fwrite(binaryData.data(), sizeof(unsigned char), bytesToReplace, file) != bytesToReplace) {
            log("Failed to write data to the file.");
            fclose(file);
            return false;
        }
    }

    fclose(file);
    return true;
    //log("Hex editing completed.");
}

bool hexEditFindReplace(const std::string& filePath, const std::string& hexDataToReplace, const std::string& hexDataReplacement, const std::string& occurrence = "0")
{
    const std::vector<size_t> offsets = findHexDataOffsets(filePath, hexDataToReplace);
    if (!offsets.empty()) {
        if (occurrence == "0") {
            // Replace all occurrences
            for (const size_t offset : offsets) {
                //log("offsetStr: "+offsetStr);
                //log("hexDataReplacement: "+hexDataReplacement);
                hexEditByOffset(filePath, offset, hexDataReplacement);
            }
        } else {
            // Convert the occurrence string to an integer
            std::size_t index = std::stoul(occurrence);
            if (index > 0 && index <= offsets.size()) {
                // Replace the specified occurrence/index
                auto offset = offsets[index - 1];
                //log("offsetStr: "+offsetStr);
                //log("hexDataReplacement: "+hexDataReplacement);
                hexEditByOffset(filePath, offset, hexDataReplacement);
            } else {
                return false;
                // Invalid occurrence/index specified
                log("Invalid occurrence/index specified.");
            }
        }
        return true;
        //std::cout << "Hex data replaced successfully." << std::endl;
    } else {
        return false;
        log("Hex data to replace not found.");
    }
}

bool hexEditCustOffset(const std::string& filePath, const size_t offsetFromCust, const std::string& hexDataReplacement)
{
    const std::string CUST = "43555354";
    const std::vector<size_t> custOffsets = findHexDataOffsets(filePath, CUST);
    if (!custOffsets.empty()) {
        const size_t offset = offsetFromCust + custOffsets[0]; // count from "C" letter
        hexEditByOffset(filePath, offset, hexDataReplacement);
    } else {
        return false;
        log("CUST not found.");
    }
    return true;
}

int reversedHexToInt(const std::string& hex_str) {
    uint32_t result = 0;
    int len = hex_str.length();

    for (int i = 0; i < len; i += 2) {
        char byte_str[] = { hex_str[len - 2 - i], hex_str[len - 1 - i], '\0' };
        uint32_t byte_value = std::strtoul(byte_str, nullptr, 16);
        result = (result << 8) | byte_value;
    }

    return static_cast<int32_t>(result);
}

std::string findCurrentKip(const std::string& jsonPath, const std::string& offset, FILE* kipFile, int custOffset)
{
    std::string searchKey;
    const char* valueStr;
    auto jsonData = readJsonFromFile(jsonPath);
    
    if (jsonData && json_is_array(jsonData)) {
        // log("jsonData is gotten");
        size_t arraySize = json_array_size(jsonData);
        // log("arraySize = %ld", arraySize);
        if (arraySize < 2) {
            return "\u25B6";
        }
        json_t* item = json_array_get(jsonData, 1);
        if (item && json_is_object(item)) {
            json_t* hexValue = json_object_get(item, "hex");
            json_t* decValue = json_object_get(item, "dec");
            int hexLength = 0;
            // if (hexValue) {log("hexvalue");}
            // if (decValue) {log("decValue");}
            if ((hexValue && json_is_string(hexValue))) {
                valueStr = json_string_value(hexValue);
                searchKey = "hex";
                hexLength = strlen(valueStr) / 2;
                hexLength = std::max(hexLength, 1);
            } else if ((decValue && json_is_string(decValue))) {
                valueStr = json_string_value(decValue);
                searchKey = "dec";
                hexLength = 4;
            } else {
                return "\u25B6";
            }
            // log("hexLength = %d", hexLength);
            // log("searchKey = %s", searchKey.c_str());
            std::string currentHex;
            try {
                const std::string CUST = "43555354";
                currentHex = readHexDataAtOffset(kipFile, CUST, std::stoul(offset), hexLength, custOffset);
            } catch (const std::invalid_argument& ex) {
                log("ERROR - %s:%d - invalid offset value: \"%s\" in \"%s\"", __func__, __LINE__, offset.c_str(), jsonPath.c_str());
            }
            // log("currentHex = %s", currentHex.c_str());
            if (!currentHex.empty()) {
                if (searchKey == "dec") {
                    currentHex = std::to_string(reversedHexToInt(currentHex));
                }
                for (size_t i = 0; i < arraySize; ++i) {
                    json_t* item = json_array_get(jsonData, i);
                    if (item && json_is_object(item)) {
                        json_t* searchItem = json_object_get(item, searchKey.c_str());
                        if (searchItem && json_is_string(searchItem)) {
                            if (json_string_value(searchItem) == currentHex) {
                                json_t* name = json_object_get(item, "name");
                                std::string cur_name = json_string_value(name);
                                size_t footer_pos = cur_name.find(" - ");
                                if (footer_pos != std::string::npos) {
                                    cur_name.resize(footer_pos);
                                }
                                return cur_name;
                            }
                        }
                    }
                }
            }
        }
    }
    return "\u25B6";
}