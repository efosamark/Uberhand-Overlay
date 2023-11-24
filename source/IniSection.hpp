#include <map>
#include <unordered_map>

using IniKeyValue = std::unordered_map<std::string, std::string>;
using IniSectionInput = std::unordered_map<std::string, IniKeyValue>;

IniSectionInput readIniFile(const std::string& filename) {
    IniSectionInput iniData;
    std::ifstream inFile(filename);
    std::string line;
    std::string currentSection;

    while (std::getline(inFile, line)) {
        // Handle section lines
        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            iniData[currentSection] = {};
        }
        // Handle key-value lines
        else if (!currentSection.empty()) {
            size_t eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = line.substr(0, eqPos);
                key = trim(key);
                std::string value = line.substr(eqPos + 1);
                value = trim(value);
                iniData[currentSection][key] = value;
            }
        }
    }
    inFile.close();
    return iniData;
}

// Write the IniSectionInput structure back to a file
void writeIniFile(const std::string& filename, const IniSectionInput& iniData) {
    std::ofstream outFile(filename);
    for (const auto& [section, kvPairs] : iniData) {
        outFile << "[" << section << "]\n";
        for (const auto& [key, value] : kvPairs) {
            outFile << key << " = " << value << "\n";
        }
        outFile << "\n";
    }
    outFile.close();
}

// Update values in the INI data
void updateIniData(IniSectionInput& iniData, const IniSectionInput& updates) {
    for (const auto& [section, kvPairs] : updates) {
        for (const auto& [key, value] : kvPairs) {
            //logMessage(iniData[section][key]);
            iniData[section][key] = value;
        }
    }
}


std::vector<std::string> splitSections(const std::string& str) {
    std::vector<std::string> result;
    std::string temp;
    size_t pos = 0, lastPos = 0;

    while ((pos = str.find("}}},", lastPos)) != std::string::npos) {
        temp = str.substr(lastPos, pos + 3 - lastPos); // keep the "}}}"
        result.push_back(temp);
        lastPos = pos + 5; // skip over the "}}}," for the next iteration
    }
    // Append last substring if there's no "}}}," at the end of the input string
    temp = str.substr(lastPos);
    if (!temp.empty()) {
        result.push_back(temp);
    }
    return result;
}


IniSectionInput parseDesiredData(const std::string& input) {
    IniSectionInput desiredData;
    std::vector<std::string> sections = splitSections(input);

    for (auto& part : sections) {
        std::istringstream iss(part);
        std::string section, key, value;

        // Extract section
        std::getline(iss, section, ',');
        section = section.substr(1, section.find(' ') - 1); // Removing '{' and optional space

        desiredData[section] = {};

        // Extract key-value pairs
        while (std::getline(iss, key, ',')) {
            std::getline(iss, value, '}');

            // Cleaning up key and value strings
            key = trim(key.substr(key.find_first_not_of(" {"))); // Removing leading whitespace and '{'

            desiredData[trim(section)][key] = trim(value);
            iss.ignore(3); // Skipping "}, {" sequence
        }
    }

    return desiredData;
}