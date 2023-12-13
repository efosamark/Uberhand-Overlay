#pragma once
#include <switch.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>
#include <get_funcs.hpp>
#include <path_funcs.hpp>
#include <ini_funcs.hpp>
#include <hex_funcs.hpp>
#include <download_funcs.hpp>
#include <json_funcs.hpp>
#include <text_funcs.hpp>
#include <jansson.h>

#define SpsmShutdownMode_Normal 0
#define SpsmShutdownMode_Reboot 1

#define KEY_A HidNpadButton_A
#define KEY_B HidNpadButton_B
#define KEY_X HidNpadButton_X
#define KEY_Y HidNpadButton_Y
#define KEY_L HidNpadButton_L
#define KEY_R HidNpadButton_R
#define KEY_ZL HidNpadButton_ZL
#define KEY_ZR HidNpadButton_ZR
#define KEY_PLUS HidNpadButton_Plus
#define KEY_MINUS HidNpadButton_Minus
#define KEY_DUP HidNpadButton_Up
#define KEY_DDOWN HidNpadButton_Down
#define KEY_DLEFT HidNpadButton_Left
#define KEY_DRIGHT HidNpadButton_Right
#define KEY_SL HidNpadButton_AnySL
#define KEY_SR HidNpadButton_AnySR
#define KEY_LSTICK HidNpadButton_StickL
#define KEY_RSTICK HidNpadButton_StickR
#define KEY_UP (HidNpadButton_Up | HidNpadButton_StickLUp | HidNpadButton_StickRUp)
#define KEY_DOWN (HidNpadButton_Down | HidNpadButton_StickLDown | HidNpadButton_StickRDown)
#define KEY_LEFT (HidNpadButton_Left | HidNpadButton_StickLLeft | HidNpadButton_StickRLeft)
#define KEY_RIGHT (HidNpadButton_Right | HidNpadButton_StickLRight | HidNpadButton_StickRRight)
#define touchPosition const HidTouchState
#define touchInput &touchPos
#define JoystickPosition HidAnalogStickState

// String path variables
const std::string configFileName = "config.ini";
const std::string settingsPath = "sdmc:/config/uberhand/";
const std::string settingsConfigIniPath = settingsPath + configFileName;
const std::string packageDirectory = "sdmc:/switch/.packages/";
const std::string overlayDirectory = "sdmc:/switch/.overlays/";
const std::string teslaSettingsConfigIniPath = "sdmc:/config/tesla/"+configFileName;
const std::string overlaysIniFilePath = settingsPath + "overlays.ini";
const std::string packagesIniFilePath = settingsPath + "packages.ini";
const std::string checkmarkChar = "\uE14B";

bool applied = false;
bool deleted = false;
bool resetValue = false;
std::string prevValue = "";
enum ShiftFocusMode {
    UpNum,
    DownNum,
    UpMax,
    DownMax
};

void scrollListItems(tsl::Gui* gui, ShiftFocusMode mode) {
    int i = 0;
    int scrollItemNum = 0;
    tsl::FocusDirection dir;
    switch (mode)
    {
        case ShiftFocusMode::UpNum:
            scrollItemNum = 4;
            dir = tsl::FocusDirection::Up;
            break;
        case ShiftFocusMode::DownNum:
            scrollItemNum = 4;
            dir = tsl::FocusDirection::Down;
            break;
        case ShiftFocusMode::UpMax:
            scrollItemNum = 10000;
            dir = tsl::FocusDirection::Up;
            break;
        case ShiftFocusMode::DownMax:
            scrollItemNum = 10000;
            dir = tsl::FocusDirection::Down;
            break;
        default:
            return;
    }
    do {
        gui->requestFocus(gui->getTopElement(), dir);
        i++;
    } while (i < scrollItemNum);
}

void copyTeslaKeyComboTouberhand() {
    std::string keyCombo;
    std::map<std::string, std::map<std::string, std::string>> parsedData;
    
    if (isFileOrDirectory(teslaSettingsConfigIniPath)) {
        parsedData = getParsedDataFromIniFile(teslaSettingsConfigIniPath);
        if (parsedData.count("tesla") > 0) {
            auto& teslaSection = parsedData["tesla"];
            if (teslaSection.count("key_combo") > 0) {
                keyCombo = teslaSection["key_combo"];
            }
        }
    }
    
    if (!keyCombo.empty()){
        if (isFileOrDirectory(settingsConfigIniPath)) {
            parsedData = getParsedDataFromIniFile(settingsConfigIniPath);
            if (parsedData.count("uberhand") > 0) {
                auto& uberhandSection = parsedData["uberhand"];
                if (uberhandSection.count("key_combo") == 0) {
                    // Write the key combo to the destination file
                    setIniFileValue(settingsConfigIniPath, "uberhand", "key_combo", keyCombo);
                }
            }
        }
    }
    tsl::impl::parseOverlaySettings();
}



// Safety conditions
// List of protected folders
const std::vector<std::string> protectedFolders = {
    "sdmc:/Nintendo/",
    "sdmc:/emuMMC/",
    "sdmc:/atmosphere/",
    "sdmc:/bootloader/",
    "sdmc:/switch/",
    "sdmc:/config/",
    "sdmc:/"
};
const std::vector<std::string> ultraProtectedFolders = {
    "sdmc:/Nintendo/",
    "sdmc:/emuMMC/"
};
bool isDangerousCombination(const std::string& patternPath) {
    // List of obviously dangerous patterns
    const std::vector<std::string> dangerousCombinationPatterns = {
        "*",         // Deletes all files/directories in the current directory
        "*/"         // Deletes all files/directories in the current directory
    };

    // List of obviously dangerous patterns
    const std::vector<std::string> dangerousPatterns = {
        "..",     // Attempts to traverse to parent directories
        "~"       // Represents user's home directory, can be dangerous if misused
    };

    // Check if the patternPath is an ultra protected folder
    for (const std::string& ultraProtectedFolder : ultraProtectedFolders) {
        if (patternPath.find(ultraProtectedFolder) == 0) {
            return true; // Pattern path is an ultra protected folder
        }
    }

    // Check if the patternPath is a protected folder
    for (const std::string& protectedFolder : protectedFolders) {
        if (patternPath == protectedFolder) {
            return true; // Pattern path is a protected folder
        }

        // Check if the patternPath starts with a protected folder and includes a dangerous pattern
        if (patternPath.find(protectedFolder) == 0) {
            std::string relativePath = patternPath.substr(protectedFolder.size());

            // Split the relativePath by '/' to handle multiple levels of wildcards
            std::vector<std::string> pathSegments;
            std::string pathSegment;

            for (char c : relativePath) {
                if (c == '/') {
                    if (!pathSegment.empty()) {
                        pathSegments.push_back(pathSegment);
                        pathSegment.clear();
                    }
                } else {
                    pathSegment += c;
                }
            }

            if (!pathSegment.empty()) {
                pathSegments.push_back(pathSegment);
            }

            for (const std::string& pathSegment : pathSegments) {
                // Check if the pathSegment includes a dangerous pattern
                for (const std::string& dangerousPattern : dangerousPatterns) {
                    if (pathSegment.find(dangerousPattern) != std::string::npos) {
                        return true; // Pattern path includes a dangerous pattern
                    }
                }
            }
        }

        // Check if the patternPath is a combination of a protected folder and a dangerous pattern
        for (const std::string& dangerousPattern : dangerousCombinationPatterns) {
            if (patternPath == protectedFolder + dangerousPattern) {
                return true; // Pattern path is a protected folder combined with a dangerous pattern
            }
        }
    }

    // Check if the patternPath is a dangerous pattern
    if (patternPath.find("sdmc:/") == 0) {
        std::string relativePath = patternPath.substr(6); // Remove "sdmc:/"

        // Split the relativePath by '/' to handle multiple levels of wildcards
        std::vector<std::string> pathSegments;
        std::string pathSegment;

        for (char c : relativePath) {
            if (c == '/') {
                if (!pathSegment.empty()) {
                    pathSegments.push_back(pathSegment);
                    pathSegment.clear();
                }
            } else {
                pathSegment += c;
            }
        }

        if (!pathSegment.empty()) {
            pathSegments.push_back(pathSegment);
        }

        for (const std::string& pathSegment : pathSegments) {
            // Check if the pathSegment includes a dangerous pattern
            for (const std::string& dangerousPattern : dangerousPatterns) {
                if (pathSegment == dangerousPattern) {
                    return true; // Pattern path is a dangerous pattern
                }
            }
        }
    }

    // Check if the patternPath includes a wildcard at the root level
    if (patternPath.find(":/") != std::string::npos) {
        std::string rootPath = patternPath.substr(0, patternPath.find(":/") + 2);
        if (rootPath.find('*') != std::string::npos) {
            return true; // Pattern path includes a wildcard at the root level
        }
    }

    // Check if the provided path matches any dangerous patterns
    for (const std::string& pattern : dangerousPatterns) {
        if (patternPath.find(pattern) != std::string::npos) {
            return true; // Path contains a dangerous pattern
        }
    }

    return false; // Pattern path is not a protected folder, a dangerous pattern, or includes a wildcard at the root level
}



// Main interpreter
int interpretAndExecuteCommand(const std::vector<std::vector<std::string>>& commands) {
    std::string commandName, jsonPath, sourcePath, destinationPath, desiredSection, desiredKey, desiredNewKey, desiredValue, offset, hexDataToReplace, hexDataReplacement, fileUrl, occurrence;
    bool catchErrors = false;

    for (auto& unmodifiedCommand : commands) {
        
        // Check the command and perform the appropriate action
        if (unmodifiedCommand.empty()) {
            // Empty command, do nothing
            continue;
        }

        // Get the command name (first part of the command)
        commandName = unmodifiedCommand[0];
        //logMessage(commandName);
        //logMessage(command[1]);
        
        
        std::vector<std::string> command;
        
        // Modify the command to replace {json_data} placeholder if jsonPath is available
        if (!jsonPath.empty()) {
            std::vector<std::string> modifiedCommand;
            for (const std::string& commandArg : unmodifiedCommand) {
                if (commandArg.find("{json_data(") != std::string::npos) {
                    // Create a copy of the string and modify it
                    std::string modifiedArg = commandArg;
                    modifiedArg = replaceJsonSourcePlaceholder(modifiedArg, jsonPath);
                    // Use modifiedArg as needed
                    modifiedCommand.push_back(modifiedArg);
                } else {
                    modifiedCommand.push_back(commandArg);
                }
            }
            command = modifiedCommand;
        } else {
            command = unmodifiedCommand;
        }
        
        
        // if (commandName == "json-set-current") {
        //     if (command.size() >= 2) {
        //         jsonPath = preprocessPath(command[1]);
        //         offset = removeQuotes(command[2]);
        //         editJSONfile(jsonPath.c_str(), offset);
        //     }
        if (commandName == "catch_errors") {
            catchErrors = true;
        } else if (commandName == "ignore_errors") {
            catchErrors = false;
        } else if (commandName == "back") {
            return 1;
        } else if (commandName == "json_data") {
            if (command.size() >= 2) {
                jsonPath = preprocessPath(command[1]);
            }
        } else if (commandName == "make" || commandName == "mkdir") {
            // Make direcrory command
            if (command[1] != "") {
                if (command.size() >= 2) {
                    sourcePath = preprocessPath(command[1]);
                    createDirectory(sourcePath);
                }
            } else if (catchErrors) {
                    logMessage("Warning in " + commandName + " command: path is empty. Command is ignored");
            }
            // Perform actions based on the command name
        } else if (commandName == "copy" || commandName == "cp") {
            // Copy command
            if (command.size() >= 3) {
                if (command[1] != "" && command[2] != "") {
                    sourcePath = preprocessPath(command[1]);
                    destinationPath = preprocessPath(command[2]);
                    bool result;
                    if (sourcePath.find('*') != std::string::npos) {
                    // Delete files or directories by pattern
                    result = copyFileOrDirectoryByPattern(sourcePath, destinationPath);
                    } else {
                        result = copyFileOrDirectory(sourcePath, destinationPath);
                    }
                    if (!result && catchErrors) {
                        logMessage("Error in " + commandName + " command");
                        return -1;
                    }
                } else if (catchErrors) {
                    logMessage("Warning in " + commandName + " command: source or target is empty. Command is ignored");
                }
            }
        } else if (commandName == "mirror_copy" || commandName == "mirror_cp") {
            // Copy command
            if (command.size() >= 2) {
                bool result;
                sourcePath = preprocessPath(command[1]);
                if (command.size() >= 3) {
                    destinationPath = preprocessPath(command[2]);
                    result = mirrorCopyFiles(sourcePath, destinationPath);
                } else {
                    result = mirrorCopyFiles(sourcePath);
                }
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "delete" || commandName == "del") {
            // Delete command
            if (command.size() >= 2) {
                bool result;
                if (command[1] != "") {
                sourcePath = preprocessPath(command[1]);
                    if (!isDangerousCombination(sourcePath)) {
                        if (sourcePath.find('*') != std::string::npos) {
                            // Delete files or directories by pattern
                            result = deleteFileOrDirectoryByPattern(sourcePath);
                        } else {
                            result = deleteFileOrDirectory(sourcePath);
                        }
                        if (!result && catchErrors) {
                            logMessage("There is no " + command[1] + " file.");
                        }
                    }
                } else if (catchErrors) {
                    logMessage("Warning in " + commandName + " command: path is empty. Command is ignored");
                }
            }
        } else if (commandName == "mirror_delete" || commandName == "mirror_del") {
            if (command.size() >= 2) {
                bool result;
                sourcePath = preprocessPath(command[1]);
                if (command.size() >= 3) {
                    destinationPath = preprocessPath(command[2]);
                    result = mirrorDeleteFiles(sourcePath, destinationPath);
                } else {
                    result = mirrorDeleteFiles(sourcePath);
                }
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "rename" || commandName == "move" || commandName == "mv") {
            // Rename command
            if (command.size() >= 3) {
                bool result;
                sourcePath = preprocessPath(command[1]);
                destinationPath = preprocessPath(command[2]);
                //logMessage("sourcePath: "+sourcePath);
                //logMessage("destinationPath: "+destinationPath);
                
                if (!isDangerousCombination(sourcePath)) {
                    if (sourcePath.find('*') != std::string::npos) {
                        // Move files by pattern
                        result = moveFilesOrDirectoriesByPattern(sourcePath, destinationPath);
                    } else {
                        // Move single file or directory
                        result = moveFileOrDirectory(sourcePath, destinationPath);
                    }
                    if (!result && catchErrors) {
                        logMessage("Error in " + commandName + " command");
                        return -1;
                    }
                } else if (catchErrors) {
                    //logMessage( "Dangerous combo.");
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            } else if (catchErrors) {
                logMessage("Error in " + commandName + " command");
                return -1;
                //logMessage( "Invalid move command.");
                logMessage("Invalid move command. Usage: move <source_path> <destination_path>");
            }

        } else if (commandName == "set-ini-val" || commandName == "set-ini-value") {
            // Edit command
            if (command.size() == 3) {
                sourcePath = preprocessPath(command[1]);
                // logMessage(command[2]);
                IniSectionInput iniData = readIniFile(sourcePath);
                IniSectionInput desiredData = parseDesiredData(command[2]);
                updateIniData(iniData, desiredData);
                writeIniFile(sourcePath, iniData);

            } else if (command.size() >= 5) {
                desiredValue = "";
                sourcePath = preprocessPath(command[1]);

                desiredSection = removeQuotes(command[2]);
                desiredKey = removeQuotes(command[3]);

                for (size_t i = 4; i < command.size(); ++i) {
                    desiredValue += command[i];
                    if (i < command.size() - 1) {
                        desiredValue += " ";
                    }
                }

                bool result = setIniFileValue(sourcePath.c_str(), desiredSection.c_str(), desiredKey.c_str(), desiredValue.c_str());
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "set-ini-key") {
            // Edit command
            if (command.size() >= 5) {
                sourcePath = preprocessPath(command[1]);

                desiredSection = removeQuotes(command[2]);
                desiredKey = removeQuotes(command[3]);

                for (size_t i = 4; i < command.size(); ++i) {
                    desiredNewKey += command[i];
                    if (i < command.size() - 1) {
                        desiredNewKey += " ";
                    }
                }

                bool result  = setIniFileKey(sourcePath.c_str(), desiredSection.c_str(), desiredKey.c_str(), desiredNewKey.c_str());
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "hex-by-offset") {
            // Edit command
            if (command.size() >= 4) {
                sourcePath = preprocessPath(command[1]);

                offset = removeQuotes(command[2]);
                hexDataReplacement = removeQuotes(command[3]);

                bool result  = hexEditByOffset(sourcePath.c_str(), offset.c_str(), hexDataReplacement.c_str());
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "hex-by-swap") {
            // Edit command - Hex data replacement with occurrence
            if (command.size() >= 4) {
                sourcePath = preprocessPath(command[1]);
                bool result;
                hexDataToReplace = removeQuotes(command[2]);
                hexDataReplacement = removeQuotes(command[3]);

                if (command.size() >= 5) {
                    occurrence = removeQuotes(command[4]);
                    result  = hexEditFindReplace(sourcePath, hexDataToReplace, hexDataReplacement, occurrence);
                } else {
                    result  = hexEditFindReplace(sourcePath, hexDataToReplace, hexDataReplacement);
                }
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "hex-by-string") {
            // Edit command - Hex data replacement with occurrence
            if (command.size() >= 4) {
                sourcePath = preprocessPath(command[1]);
                bool result;
                hexDataToReplace = asciiToHex(removeQuotes(command[2]));
                hexDataReplacement = asciiToHex(removeQuotes(command[3]));
                //logMessage("hexDataToReplace: "+hexDataToReplace);
                //logMessage("hexDataReplacement: "+hexDataReplacement);
                
                // Fix miss-matched string sizes
                if (hexDataReplacement.length() < hexDataToReplace.length()) {
                    // Pad with spaces at the end
                    hexDataReplacement += std::string(hexDataToReplace.length() - hexDataReplacement.length(), '\0');
                } else if (hexDataReplacement.length() > hexDataToReplace.length()) {
                    // Add spaces to hexDataToReplace at the far right end
                    hexDataToReplace += std::string(hexDataReplacement.length() - hexDataToReplace.length(), '\0');
                }
                
                if (command.size() >= 5) {
                    occurrence = removeQuotes(command[4]);
                    result  = hexEditFindReplace(sourcePath, hexDataToReplace, hexDataReplacement, occurrence);
                } else {
                    result  = hexEditFindReplace(sourcePath, hexDataToReplace, hexDataReplacement);
                }
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "hex-by-decimal") {
            // Edit command - Hex data replacement with occurrence
            if (command.size() >= 4) {
                sourcePath = preprocessPath(command[1]);
                bool result;
                hexDataToReplace = decimalToHex(removeQuotes(command[2]));
                hexDataReplacement = decimalToHex(removeQuotes(command[3]));
                //logMessage("hexDataToReplace: "+hexDataToReplace);
                //logMessage("hexDataReplacement: "+hexDataReplacement);

                if (command.size() >= 5) {
                    occurrence = removeQuotes(command[4]);
                    result  = hexEditFindReplace(sourcePath, hexDataToReplace, hexDataReplacement, occurrence);
                } else {
                    result  = hexEditFindReplace(sourcePath, hexDataToReplace, hexDataReplacement);
                    
                }
                if (!result && catchErrors) {
                        logMessage("Error in " + commandName + " command");
                        return -1;
                    }
            }
        } else if (commandName == "hex-by-rdecimal") {
            // Edit command - Hex data replacement with occurrence
            if (command.size() >= 4) {
                sourcePath = preprocessPath(command[1]);
                bool result;
                hexDataToReplace = decimalToReversedHex(removeQuotes(command[2]));
                hexDataReplacement = decimalToReversedHex(removeQuotes(command[3]));
                //logMessage("hexDataToReplace: "+hexDataToReplace);
                //logMessage("hexDataReplacement: "+hexDataReplacement);

                if (command.size() >= 5) {
                    occurrence = removeQuotes(command[4]);
                    result  = hexEditFindReplace(sourcePath, hexDataToReplace, hexDataReplacement, occurrence);
                } else {
                    result  = hexEditFindReplace(sourcePath, hexDataToReplace, hexDataReplacement);
                    
                }
                if (!result && catchErrors) {
                        logMessage("Error in " + commandName + " command");
                        return -1;
                    }
            }
        } else if (commandName == "hex-by-cust-offset-dec") {
            // Edit command - Hex data replacement with offset from CUST (decimal)
            if (command.size() >= 3) {
                sourcePath = preprocessPath(command[1]);
                offset = removeQuotes(command[2]);
                hexDataReplacement = decimalToReversedHex(removeQuotes(command[3]));
                bool result  = hexEditCustOffset(sourcePath, offset, hexDataReplacement);
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "hex-by-cust-offset") {
            // Edit command - Hex data replacement with offset from CUST ("raw" hex)
            if (command.size() >= 3) {
                sourcePath = preprocessPath(command[1]);
                offset = removeQuotes(command[2]);
                hexDataReplacement = removeQuotes(command[3]);
                bool result = hexEditCustOffset(sourcePath, offset, hexDataReplacement);
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "download") {
            // Edit command - Hex data replacement with occurrence
            if (command.size() >= 3) {
                fileUrl = preprocessUrl(command[1]);
                destinationPath = preprocessPath(command[2]);
                //logMessage("fileUrl: "+fileUrl);
                bool result = downloadFile(fileUrl, destinationPath);
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "unzip") {
            // Edit command - Hex data replacement with occurrence
            if (command.size() >= 3) {
                sourcePath = preprocessPath(command[1]);
                destinationPath = preprocessPath(command[2]);
                bool result = unzipFile(sourcePath, destinationPath);
                if (!result && catchErrors) {
                    logMessage("Error in " + commandName + " command");
                    return -1;
                }
            }
        } else if (commandName == "reboot") {
            // Reboot command
            splExit();
            fsdevUnmountAll();
            spsmShutdown(SpsmShutdownMode_Reboot);
        } else if (commandName == "shutdown") {
            // Reboot command
            splExit();
            fsdevUnmountAll();
            spsmShutdown(SpsmShutdownMode_Normal);
        } else if (commandName == "backup") {
            // Generate backup
            generateBackup();
        }
    }
    return 0;
}

tsl::PredefinedColors defineColor(std::string strColor) {
    // logMessage ("string color: " + strColor);
    if (strColor == "Green") {
        return tsl::PredefinedColors::Green;
    } else if (strColor == "Red") {
        return tsl::PredefinedColors::Red;
    } else if (strColor == "White") {
        return tsl::PredefinedColors::White;
    } else if (strColor == "Orange") {
        return tsl::PredefinedColors::Orange;
    } else if (strColor == "Gray") {
        return tsl::PredefinedColors::Gray;
    } else {
        return tsl::PredefinedColors::DefaultText;
    } 
}

std::pair<std::string, int> dispCustData(const std::string jsonPath, std::string kipPath = "/atmosphere/kips/loader.kip", bool spacing = false) {

    std::string custOffset = "";
    std::string currentHex = "";
    std::string extent = "";
    std::string output = "";
    std::string state = "";
    std::string name = "";
    std::string offset  = "";
    std::string increment  = "";
    bool allign = false;
    int checkDefault = 0;
    int length = 0;
    int lineCount = 1;
    bool tableShiftMode = false;
    int tableState;
    std::vector<std::string> baseList;
    std::vector<std::string> baseIncList;

    // kipPath = std::string("/atmosphere/kips/loader.kip");

    if (!isFileOrDirectory(jsonPath)) {
        return std::make_pair(output, lineCount);
    }
    json_t* jsonData = readJsonFromFile(jsonPath);
    if (jsonData) {
        size_t arraySize = json_array_size(jsonData);

        std::vector<std::string> offsetStrs = findHexDataOffsets(kipPath.c_str(), "43555354"); // 43555354 is a CUST
        custOffset = offsetStrs[0];
        FILE* file = fopen(kipPath.c_str(), "rb");
        if (!file) {
            logMessage("Failed to open the loader.kip.");
            return std::make_pair(output, lineCount);
        }

        for (size_t i = 0; i < arraySize; ++i) {
            json_t* item = json_array_get(jsonData, i);
            if (item && json_is_object(item)) {
                json_t* keyValue = json_object_get(item, "name");
                // logMessage(json_string_value(keyValue));
                std::string tabBaseCheck;
                if (keyValue)
                    tabBaseCheck = json_string_value(keyValue);
                if (tabBaseCheck == "TABLE_BASE") {
                    tableShiftMode = true;
                    std::string tableStateStr = readHexDataAtOffsetF(file, custOffset, "43555354", "44", 1);
                    tableState = reversedHexToInt(tableStateStr);
                    //logMessage(tableStateStr);

                    json_t* j_base = json_object_get(item, "base");
                    std::string base = json_string_value(j_base);
                    std::istringstream iss2(base);
                    std::string baseItem;

                    if (base.find(',') != std::string::npos) {
                        // Split the string by commas and store each offset in a vector
                        while (std::getline(iss2, baseItem, ',')) {
                            baseList.push_back(baseItem);
                        }
                    }

                    json_t* j_base_inc = json_object_get(item, "base_increment");
                    if (!j_base_inc) {
                        tableShiftMode = false;
                        continue;
                    }
                    std::string base_inc = json_string_value(j_base_inc);
                    std::istringstream iss(base_inc);
                    std::string baseIncItem;

                    if (base_inc.find(',') != std::string::npos) {
                        // Split the string by commas and store each offset in a vector
                        while (std::getline(iss, baseIncItem, ',')) {
                            baseIncList.push_back(baseIncItem);
                        }
                    }
                } else {
                    if (keyValue && json_is_string(keyValue)) {
                        json_t* j_offset    = json_object_get(item, "offset");
                        json_t* j_length    = json_object_get(item, "length");
                        json_t* j_extent    = json_object_get(item, "extent");
                        json_t* j_state     = json_object_get(item, "state");
                        json_t* j_increment = json_object_get(item, "increment");

                        if (j_state) {
                            state = json_string_value(j_state);
                        } else {
                            state = "";
                        }

                        if (state != "filler" || state.empty()) {
                            if (!j_offset || !j_length) {
                                return std::make_pair(output, lineCount);
                            }
                            name = json_string_value(keyValue);
                            offset = json_string_value(j_offset);
                            length = std::stoi(json_string_value(j_length));
                            if (j_extent) {
                                extent = json_string_value(j_extent);
                            }
                            else {
                                extent = "";
                            }

                            if (offset.find(',') != std::string::npos) {
                                std::istringstream iss(offset);
                                std::string offsetItem;
                                std::vector<std::string> offsetList;

                                // Split the string by commas and store each offset in a vector
                                while (std::getline(iss, offsetItem, ',')) {
                                    offsetList.push_back(offsetItem);
                                }

                                currentHex = "";
                                for (const std::string& offsetItem : offsetList) {
                                    std::string tempHex = readHexDataAtOffsetF(file, custOffset, "43555354", offsetItem.c_str(), length); // Read the data from kip with offset starting from 'C' in 'CUST'
                                    unsigned int intValue = reversedHexToInt(tempHex);
                                    currentHex += std::to_string(intValue) + '-';
                                }
                                currentHex.pop_back();
                                output += name + ": " + currentHex;
                            } else {
                                json_t* j_default = json_object_get(item, "default");
                                if (j_default) {
                                checkDefault = std::stoi(json_string_value(j_default));
                                if (checkDefault == 1) {
                                    std::string offsetDef = std::to_string(std::stoi(offset) + length);
                                    currentHex = readHexDataAtOffsetF(file, custOffset, "43555354", offsetDef.c_str(), length); // Read next <length> hex chars from specified offset
                                }
                            }
                                if (allign) {
                                // Format the string to have two columns; Calculate number of spaces needed
                                size_t found = output.rfind('\n');
                                int numreps = 33 - (output.length() - found - 1) - name.length() - length - 4;
                                if (!extent.empty()) {
                                    numreps -= extent.length();
                                }
                                output.append(numreps, ' ');
                                allign = false;
                            }

                                if (checkDefault && currentHex != "000000") {
                                    output += name + ": " + "Default";
                                    extent = "";
                                    checkDefault = 0;
                                } else {
                                    if (tableShiftMode) {
                                        //logMessage(std::to_string(std::stoi(baseList[tableState]) + (std::stoi(offset) * std::stoi(baseIncList[tableState]))));
                                        std::string findFreq = std::to_string(std::stoi(baseList[tableState]) + (std::stoi(offset) * std::stoi(baseIncList[tableState])));
                                        currentHex = readHexDataAtOffsetF(file, custOffset, "43555354", findFreq.c_str(), length); // Read the data from kip with offset starting from 'C' in 'CUST'
                                    }
                                    else
                                        currentHex = readHexDataAtOffsetF(file, custOffset, "43555354", offset.c_str(), length); // Read the data from kip with offset starting from 'C' in 'CUST'
                                    unsigned int intValue = reversedHexToInt(currentHex);
                                    if (j_increment) { // Add increment value from the JSON to the displayed value
                                        intValue += std::stoi(json_string_value(j_increment));
                                    }
                                    if (intValue > 1500) {
                                        intValue = intValue/1000;
                                    }
                                    output += name + ": " + std::to_string(intValue);
                                    if (intValue < 100) 
                                        extent = "";
                                }
                            }

                            if (!extent.empty()) {
                                output += extent;
                            }
                            if (state != "no_skip"){
                                output += '\n';
                                lineCount++;
                            } else {
                                allign = true;
                            }
                        } else { // When state = filler
                            std::string name = json_string_value(keyValue);
                            if (spacing) {
                                output += '\n';
                                lineCount++;
                            }
                            output += name;
                            output += '\n';
                            lineCount++;
                        }
                    }
                }
            }
        }
        fclose(file);
        json_decref(jsonData);
    }
    return std::make_pair(output, lineCount);
}

std::pair<std::string, int> dispRAMTmpl(std::string dataPath, std::string selectedItem) {

    std::stringstream output;
    std::string name = "";
    std::string value  = "";
    int nItems = 0;
    int lineNum = 0;


    if (!isFileOrDirectory(dataPath)) {
        return std::make_pair(output.str(), nItems/2);
    }
    json_t* jsonData = readJsonFromFile(dataPath);
    if (jsonData) {
        size_t arraySize = json_array_size(jsonData);

        for (size_t i = 0; i < arraySize; ++i) {
            json_t* item = json_array_get(jsonData, i);
            json_t* keyValue = json_object_get(item, "name");

            if (json_string_value(keyValue) == selectedItem) {
                output << "These RAM settings will be applied:\n\n\n-------------------------------------------------------------------\n";
                const char *key;
                json_t *value;
                json_object_foreach(item, key, value) {
                    int spaces[5] = {4,9,6,6,6}; //TODO: remove hardcode; redo text display processing
                    if (strcmp(key, "name") != 0 && strcmp(key, "t_offsets")) {
                        output << key << ": " << json_string_value(value) << std::string(spaces[lineNum], ' ');
                        nItems++;
                        if (strlen(key) > 5 && strlen(json_string_value(value))> 1) {
                            output << "\n-------------------------------------------------------------------\n";
                            nItems += 2;
                        } else if (nItems % 2 == 0) { //every second item
                            lineNum++;
                            output << "\n-------------------------------------------------------------------\n";
                            nItems += 2;
                        }
                    }
                }
            }
        }
    }
    return std::make_pair(output.str(), nItems/2+5);
}

bool verifyIntegrity (std::string check) {

    bool verified = false;

    std::transform(check.begin(), check.end(), check.begin(), ::tolower);
    
    for (size_t i = 0; i < check.length() - 4; ++i) {

        if (static_cast<int>(check[i]) == 117 && static_cast<int>(check[i + 1]) == 108 && static_cast<int>(check[i + 2]) == 116 && static_cast<int>(check[i + 3]) == 114 && static_cast<int>(check[i + 4]) == 97) {
            verified = true;
            break; 
        }
    }

    return verified;
}


void removeLastNumericWord(std::string& str) {
    // Iterate through the string from the end
    for (int i = str.length() - 1; i >= 0; --i) {
        if (str[i] == ' ' && std::isdigit(str[i + 1])) {
            std::string lastWord = str.substr(i + 1); // Extract the last word
            str.resize(i); // Remove the last word if it's numeric
            break;
        }
    }
}

std::vector<std::string> parseString(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, delimiter)) {
        result.push_back(token);
    }

    return result;
}

std::string getversion(std::string path) {
    json_t* json = readJsonFromFile(preprocessPath(path));
    json_t* error = json_object_get(json, "message");
    if (json_string_length(error) != 0) {
        json_decref(json);
        logMessage("API limit reached");
        return "ApiLimit";
    }
    json_t* tarballUrlObj = json_object_get(json_array_get(json, 0), "tarball_url");
    if (tarballUrlObj && json_is_string(tarballUrlObj)) {
        const std::string tarballUrl = json_string_value(tarballUrlObj);
        json_decref(json);
        json_decref(tarballUrlObj);
        return getSubstringAfterLastSlash(tarballUrl);
    }
    json_decref(json);
    return "Error";
}

std::string getLinkOnLatest(std::string path, int dEntry = 1) {
    json_t* json = readJsonFromFile(preprocessPath(path));
    json_t* assets = json_object_get(json_array_get(json, 0), "assets");
    json_t* link = json_object_get(json_array_get(assets, dEntry-1), "browser_download_url");
    if (link && json_is_string(link)) {
        const std::string linkS = json_string_value(link);
        json_decref(json);
        json_decref(assets);
        json_decref(link);
        return linkS;
    }
    json_decref(json);
    return "Error";
}


std::map<std::string, std::string> packageUpdateCheck(std::string subConfigIniPath) {
    std::map<std::string, std::string> packageInfo;
    PackageHeader packageHeader = getPackageHeaderFromIni("sdmc:/switch/.packages/" + subConfigIniPath);
    if (packageHeader.version != "" && packageHeader.github != "") {
        packageInfo["localVer"] = packageHeader.version;
        packageInfo["link"] = packageHeader.github;
        packageInfo["name"] = subConfigIniPath.substr(0, subConfigIniPath.find("/config.ini"));
        downloadFile(packageInfo["link"], "sdmc:/config/uberhand/downloads/temp.json");
        packageInfo["repoVer"] = getversion("sdmc:/config/uberhand/downloads/temp.json");
        if (packageInfo["repoVer"] == "ApiLimit") {
            deleteFileOrDirectory("sdmc:/config/uberhand/downloads/temp.json");
            packageInfo.clear();
            return packageInfo;
        }
        if (packageInfo["repoVer"][0] == 'v') {
            packageInfo["repoVer"] = packageInfo["repoVer"].substr(1);
        }
        //logMessage("672: "+ getLinkOnLatest("/config/uberhand/downloads/temp.json"));
        packageInfo["link"] = getLinkOnLatest("sdmc:/config/uberhand/downloads/temp.json");
        deleteFileOrDirectory("sdmc:/config/uberhand/downloads/temp.json");
        //logMessage("repoVer " + packageInfo["repoVer"]);
        //logMessage("localVer " + packageInfo["localVer"]);
        packageInfo["type"] = "pkgzip";
    }
    return packageInfo;
}

std::map<std::string, std::string> ovlUpdateCheck(std::map<std::string, std::string> currentOverlay) {
    std::map<std::string, std::string> ovlItemToUpdate;
    downloadFile(currentOverlay["link"], "sdmc:/config/uberhand/downloads/temp.json");
    ovlItemToUpdate["repoVer"]= getversion("sdmc:/config/uberhand/downloads/temp.json");
    if (ovlItemToUpdate["repoVer"] == "ApiLimit") {
        deleteFileOrDirectory("sdmc:/config/uberhand/downloads/temp.json");
        ovlItemToUpdate.clear();
        return ovlItemToUpdate;
    }
    //logMessage("repoVerovl: "+ovlItemToUpdate["repoVer"]);
    if (ovlItemToUpdate["repoVer"][0] == 'v') {
            ovlItemToUpdate["repoVer"] = ovlItemToUpdate["repoVer"].substr(1);
        }
    if (currentOverlay["localVer"] != ovlItemToUpdate["repoVer"]) {
        ovlItemToUpdate["link"] = getLinkOnLatest("/config/uberhand/downloads/temp.json", std::stoi(currentOverlay["downloadEntry"]));
        ovlItemToUpdate["name"] = currentOverlay["name"];
        if (getExtension(getFileNameFromURL(ovlItemToUpdate["link"])) == "zip") {
            ovlItemToUpdate["type"] = "ovlzip";
        } else {
            ovlItemToUpdate["type"] = "ovl";
        }
        deleteFileOrDirectory("sdmc:/config/uberhand/downloads/temp.json");
        return ovlItemToUpdate;
    }
    deleteFileOrDirectory("sdmc:/config/uberhand/downloads/temp.json");
    ovlItemToUpdate.clear();
    return ovlItemToUpdate;
}
