#pragma once
#include <switch.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fnmatch.h>

#define SpsmShutdownMode_Normal 0
#define SpsmShutdownMode_Reboot 1

// For loggging messages and debugging
//void logMessage(const std::string& message) {
//    std::time_t currentTime = std::time(nullptr);
//    std::string logEntry = std::asctime(std::localtime(&currentTime));
//    logEntry += message+"\n";
//
//    FILE* file = fopen("sdmc:/config/ultrahand/log.txt", "a");
//    if (file != nullptr) {
//        fputs(logEntry.c_str(), file);
//        fclose(file);
//    }
//}


// Reboot command
void reboot() {
    //logMessage("Rebooting...\n");
    spsmInitialize();
    spsmShutdown(SpsmShutdownMode_Reboot);
    //logMessage("Reboot failed..\n");
}


// Shutdown command
void shutdown() {
    //logMessage("Shutting down...\n");
    spsmInitialize();
    spsmShutdown(SpsmShutdownMode_Normal);
    //logMessage("Shutdown failed..\n");
}


// Function to read the content of a file
std::string readFileContent(const std::string& filePath) {
    std::string content;
    FILE* file = fopen(filePath.c_str(), "rb");
    if (file) {
        struct stat fileInfo;
        if (stat(filePath.c_str(), &fileInfo) == 0 && fileInfo.st_size > 0) {
            content.resize(fileInfo.st_size);
            fread(&content[0], 1, fileInfo.st_size, file);
        }
        fclose(file);
    }
    return content;
}





std::vector<std::string> getSubdirectories(const std::string& directoryPath) {
    std::vector<std::string> subdirectories;

    DIR* dir = opendir(directoryPath.c_str());
    if (dir != nullptr) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string entryName = entry->d_name;

            // Exclude current directory (.) and parent directory (..)
            if (entryName != "." && entryName != "..") {
                struct stat entryStat;
                std::string fullPath = directoryPath + "/" + entryName;

                if (stat(fullPath.c_str(), &entryStat) == 0 && S_ISDIR(entryStat.st_mode)) {
                    subdirectories.push_back(entryName);
                }
            }
        }

        closedir(dir);
    }

    return subdirectories;
}

// Function to create a directory if it doesn't exist
void createDirectory(const std::string& directoryPath) {
    struct stat st;
    if (stat(directoryPath.c_str(), &st) != 0) {
        mkdir(directoryPath.c_str(), 0777);
    }
}




bool moveFileOrDirectory(const std::string& sourcePath, const std::string& destinationPath) {
    struct stat fileInfo;
    if (stat(sourcePath.c_str(), &fileInfo) == 0) {
        // Source file or directory exists

        if (S_ISREG(fileInfo.st_mode)) {
            // Source path is a regular file

            // Extract the source file name from the path
            size_t lastSlashPos = sourcePath.find_last_of('/');
            std::string fileName = (lastSlashPos != std::string::npos)
                                       ? sourcePath.substr(lastSlashPos + 1)
                                       : sourcePath;

            // Create the destination file path
            std::string destinationFile = destinationPath + "/" + fileName;

            if (std::rename(sourcePath.c_str(), destinationFile.c_str()) == 0) {
                // Rename successful
                return true;
            }
        } else if (S_ISDIR(fileInfo.st_mode)) {
            // Source path is a directory

            if (std::rename(sourcePath.c_str(), destinationPath.c_str()) == 0) {
                // Rename successful
                return true;
            }
        }
    }

    return false;  // Rename unsuccessful or source file/directory doesn't exist
}


bool moveFilesByPattern(const std::string& sourcePathPattern, const std::string& destinationPath) {
    std::string sourceDirectory;
    std::string pattern;

    // Separate the directory path and pattern
    size_t lastSlashPos = sourcePathPattern.find_last_of('/');
    if (lastSlashPos != std::string::npos) {
        sourceDirectory = sourcePathPattern.substr(0, lastSlashPos);
        pattern = sourcePathPattern.substr(lastSlashPos + 1);
    } else {
        sourceDirectory = ".";
        pattern = sourcePathPattern;
    }

    // Open the source directory
    DIR* dir = opendir(sourceDirectory.c_str());
    if (dir == nullptr) {
        return false;  // Failed to open source directory
    }

    // Iterate through the directory entries
    dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename != "." && filename != "..") {
            // Check if the filename matches the pattern
            if (fnmatch(pattern.c_str(), filename.c_str(), 0) == 0) {
                std::string sourceFile = sourceDirectory + "/" + filename;
                std::string destinationFile = destinationPath + "/" + filename;

                if (std::rename(sourceFile.c_str(), destinationFile.c_str()) != 0) {
                    closedir(dir);
                    return false;  // Failed to move file
                }
            }
        }
    }

    closedir(dir);
    return true;  // All files matching the pattern moved successfully
}






 // Perform copy action from "fromFile" to file or directory
void copyFile(const std::string& fromFile, const std::string& toFile) {
    struct stat toFileInfo;
    if (stat(toFile.c_str(), &toFileInfo) == 0 && S_ISDIR(toFileInfo.st_mode)) {
        // Destination is a directory
        size_t lastSlashPos = fromFile.find_last_of('/');
        std::string fileName = (lastSlashPos != std::string::npos)
                                   ? fromFile.substr(lastSlashPos + 1)
                                   : fromFile;
        std::string toFilePath = toFile + "/" + fileName;

        FILE* srcFile = fopen(fromFile.c_str(), "rb");
        FILE* destFile = fopen(toFilePath.c_str(), "wb");
        if (srcFile && destFile) {
            char buffer[1024];
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), srcFile)) > 0) {
                fwrite(buffer, 1, bytesRead, destFile);
            }
            fclose(srcFile);
            fclose(destFile);
            //std::cout << "File copied successfully." << std::endl;
        } else {
            //std::cerr << "Error opening files or performing copy action." << std::endl;
        }
    } else {
        // Destination is a file or doesn't exist
        FILE* srcFile = fopen(fromFile.c_str(), "rb");
        FILE* destFile = fopen(toFile.c_str(), "wb");
        if (srcFile && destFile) {
            char buffer[1024];
            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, sizeof(buffer), srcFile)) > 0) {
                fwrite(buffer, 1, bytesRead, destFile);
            }
            fclose(srcFile);
            fclose(destFile);
            //std::cout << "File copied successfully." << std::endl;
        } 
    }
}


bool isDirectory(const std::string& path) {
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) == 0) {
        return S_ISDIR(pathStat.st_mode);
    }
    return false;
}

bool deleteDirectory(const std::string& path) {
    if (!isDirectory(path)) {
        // Not a directory
        return false;
    }

    FsFileSystem *fs = fsdevGetDeviceFileSystem("sdmc");
    if (fs == nullptr) {
        // Error getting file system
        return false;
    }

    Result ret = fsFsDeleteDirectoryRecursively(fs, path.c_str());
    if (R_FAILED(ret)) {
        // Error deleting directory
        return false;
    }

    return true;
}



void deleteFile(const std::string& pathToDelete) {
    struct stat pathStat;
    if (stat(pathToDelete.c_str(), &pathStat) == 0) {
        if (S_ISREG(pathStat.st_mode)) {
            if (std::remove(pathToDelete.c_str()) == 0) {
                // Deletion successful
            }
        } else if (S_ISDIR(pathStat.st_mode)) {
            if (deleteDirectory(pathToDelete)) {
                // Deletion successful
            }
        }
    }
}


// Trim leading and trailing whitespaces from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r\f\v");
    size_t last = str.find_last_not_of(" \t\n\r\f\v");
    if (first == std::string::npos || last == std::string::npos)
        return "";
    return str.substr(first, last - first + 1);
}

// Check if a string starts with a given prefix
bool startsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.length(), prefix) == 0;
}

std::string removeQuotes(const std::string& str) {
    std::size_t firstQuote = str.find_first_of('\'');
    std::size_t lastQuote = str.find_last_of('\'');
    if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote < lastQuote) {
        return str.substr(firstQuote + 1, lastQuote - firstQuote - 1);
    }
    return str;
}

void editIniFile(const std::string& fileToEdit, const std::string& desiredSection,
                 const std::string& desiredKey, const std::string& desiredValue) {
    FILE* configFile = fopen(fileToEdit.c_str(), "r");
    if (!configFile) {
        printf("Failed to open the INI file.\n");
        return;
    }
    std::string trimmedLine;
    std::string tempPath = fileToEdit + ".tmp";
    FILE* tempFile = fopen(tempPath.c_str(), "w");

    if (tempFile) {
        std::string currentSection;
        std::string formattedDesiredValue;
        char line[256];
        while (fgets(line, sizeof(line), configFile)) {
            trimmedLine = trim(std::string(line));

            // Check if the line represents a section
            if (trimmedLine[0] == '[' && trimmedLine[trimmedLine.length() - 1] == ']') {
                currentSection = removeQuotes(trim(std::string(trimmedLine.c_str() + 1, trimmedLine.length() - 2)));
                //logMessage("currentSection: " + currentSection);
            }

            formattedDesiredValue = removeQuotes(desiredSection);
            // Check if the line is in the desired section
            if (trim(currentSection) == trim(formattedDesiredValue)) {  // ITS NOT ENTERING AT ALLL
                //logMessage("trimmedLine/desiredKey: "+trimmedLine+" "+desiredKey);
                // Check if the line starts with the desired key
                if (startsWith(trimmedLine, desiredKey)) {
                    // Overwrite the value with the desired value
                    formattedDesiredValue = removeQuotes(desiredValue);
                    fprintf(tempFile, "%s = %s\n", desiredKey.c_str(), formattedDesiredValue.c_str());
                    //logMessage(desiredKey+"="+desiredValue);
                    continue;  // Skip writing the original line
                }
            }

            fprintf(tempFile, "%s", line);
        }

        fclose(configFile);
        fclose(tempFile);
        remove(fileToEdit.c_str());  // Delete the old configuration file
        rename(tempPath.c_str(), fileToEdit.c_str());  // Rename the temp file to the original name

        //printf("INI file updated successfully.\n");
    } else {
        //printf("Failed to create temporary file.\n");
    }
}





void interpretAndExecuteCommand(const std::vector<std::vector<std::string>>& commands) {
    for (const auto& command : commands) {
        // Check the command and perform the appropriate action
        if (command.empty()) {
            // Empty command, do nothing
            continue;
        }

        // Get the command name (first part of the command)
        std::string commandName = command[0];

        if (commandName == "make" || commandName == "mkdir") {
            std::string toDirectory = "sdmc:" + command[1];
            createDirectory(toDirectory);

            // Perform actions based on the command name
        } else if (commandName == "copy" || commandName == "cp") {
            // Copy command
            if (command.size() >= 3) {
                std::string fromPath = "sdmc:" + command[1];
                std::string toPath = "sdmc:" + command[2];
                copyFile(fromPath, toPath);
            }
        } else if (commandName == "delete" || commandName == "del") {
            // Delete command
            if (command.size() >= 2) {
                std::string fileToDelete = "sdmc:" + command[1];
                deleteFile(fileToDelete);
            }
        } else if (commandName == "rename" || commandName == "move" || commandName == "mv") {
            // Rename command
            if (command.size() >= 3) {

                std::string sourcePath = "sdmc:" + command[1];
                std::string destinationPath = "sdmc:" + command[2];

                if (command[1].back() == '/') {
                    // Remove trailing '/' from source path
                    sourcePath.pop_back();
                }

                if (command[2].back() == '/') {
                    // Remove trailing '/' from destination path
                    destinationPath.pop_back();
                }

                if (sourcePath.find('*') != std::string::npos) {
                    // Move files by pattern
                    if (moveFilesByPattern(sourcePath, destinationPath)) {
                        //std::cout << "Files moved successfully." << std::endl;
                    } else {
                        //std::cout << "Failed to move files." << std::endl;
                    }
                } else {
                    // Move single file or directory
                    if (moveFileOrDirectory(sourcePath, destinationPath)) {
                        //std::cout << "File or directory moved successfully." << std::endl;
                    } else {
                        //std::cout << "Failed to move file or directory." << std::endl;
                    }
                }
            } else {
                //std::cout << "Invalid move command. Usage: move <source_path> <destination_path>" << std::endl;
            }

        } else if (commandName == "edit-ini") {
            // Edit command
            if (command.size() >= 5) {
                std::string fileToEdit = "sdmc:" + command[1];

                std::string desiredSection = command[2];
                std::string desiredKey = command[3];

                std::string desiredValue;
                for (size_t i = 4; i < command.size(); ++i) {
                    desiredValue += command[i];
                    if (i < command.size() - 1) {
                        desiredValue += " ";
                    }
                }

                editIniFile(fileToEdit.c_str(), desiredSection.c_str(), desiredKey.c_str(), desiredValue.c_str());
            }
        } else if (commandName == "reboot") {
            // Reboot command
            reboot();
        } else if (commandName == "shutdown") {
            // Reboot command
            shutdown();
        }
    }
}


std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> loadOptionsFromIni(const std::string& configIniPath) {
    std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> options;

    FILE* configFile = fopen(configIniPath.c_str(), "r");
    if (!configFile) {
        // Write the default INI file
        FILE* configFileOut = fopen(configIniPath.c_str(), "w");
        fprintf(configFileOut, "[make directories]\n");
        fclose(configFileOut);
        configFile = fopen(configIniPath.c_str(), "r");
    }

    char line[256];
    std::string currentOption;
    std::vector<std::vector<std::string>> commands;

    while (fgets(line, sizeof(line), configFile)) {
        std::string trimmedLine = line;
        trimmedLine.erase(trimmedLine.find_last_not_of("\r\n") + 1);  // Remove trailing newline character

        if (trimmedLine.empty() || trimmedLine[0] == '#') {
            // Skip empty lines and comment lines
            continue;
        } else if (trimmedLine[0] == '[' && trimmedLine.back() == ']') {
            // New option section
            if (!currentOption.empty()) {
                // Store previous option and its commands
                options.emplace_back(std::move(currentOption), std::move(commands));
                commands.clear();
            }
            currentOption = trimmedLine.substr(1, trimmedLine.size() - 2);  // Extract option name
        } else {
            // Command line
            std::istringstream iss(trimmedLine);
            std::vector<std::string> commandParts;
            std::string part;
            while (iss >> part) {
                commandParts.push_back(part);
            }
            commands.push_back(std::move(commandParts));
        }
    }

    // Store the last option and its commands
    if (!currentOption.empty()) {
        options.emplace_back(std::move(currentOption), std::move(commands));
    }

    fclose(configFile);
    return options;
}

std::string getFolderName(const std::string& path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash + 1 < path.length()) {
        return path.substr(lastSlash + 1);
    }
    return path;
}
