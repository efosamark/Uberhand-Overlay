#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <utility>

std::pair<std::string, int> readTextFromFile(const std::string& filePath)
{
    // log("Entered readTextFromFile");

    std::string lines;
    std::string currentLine;
    std::ifstream file(filePath);
    std::vector<std::string> words;
    int lineCount = 0;
    size_t maxRowLength = 35;

    std::string line;
    while (std::getline(file, line)) {
        if (line == "\r" || line.empty()) {
            lines += "\n"; // Preserve empty lines
            lineCount++;
            continue;
        }

        std::istringstream lineStream(line);
        std::string word;
        std::string currentLine;

        while (lineStream >> word) {
            if (currentLine.empty()) {
                currentLine = word;
            } else if (currentLine.length() + 1 + word.length() <= maxRowLength) {
                currentLine += " " + word;
            } else {
                lines += currentLine + "\n";
                currentLine = word;
                lineCount++;
            }
        }

        if (!currentLine.empty()) {
            lines += currentLine + "\n";
            lineCount++;
        }
    }

    file.close();
    return std::make_pair(lines, lineCount);
}

bool write_to_file(const std::string& file_path, const std::string& line)
{
    std::ifstream file(file_path);
    if (!file.is_open()) {
        // File doesn't exist, create it
        std::ofstream output_file(file_path);
        if (!output_file.is_open()) {
            log("Error opening file: %s", file_path.c_str());
            return false;
        }
        output_file << line << std::endl;
        return true;
    }

    std::string existing_line;
    std::vector<std::string> existing_lines;
    while (std::getline(file, existing_line)) {
        existing_lines.push_back(existing_line);
    }
    file.close();

    if (std::find(existing_lines.begin(), existing_lines.end(), line) != existing_lines.end()) {
        return true; // Line already exists, no need to write it again
    }

    existing_lines.push_back(line);

    std::ofstream output_file(file_path);
    if (!output_file.is_open()) {
        log("Error opening file: %s", file_path.c_str());
        return false;
    }

    for (const auto& existing_line : existing_lines) {
        output_file << existing_line << std::endl;
    }
    return true;
}

bool remove_txt(const std::string& file_path, const std::string& pattern)
{
    std::vector<std::string> lines;
    std::ifstream file(file_path);
    if (!file.is_open()) {
        log("File %s not found", file_path.c_str());
        return true;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(pattern) == std::string::npos) {
            lines.push_back(line);
        }
    }
    file.close();

    std::ofstream output_file(file_path);
    if (!output_file.is_open()) {
        log("File %s can't be created", file_path.c_str());
        return true;
    }

    for (const auto& line : lines) {
        output_file << line << std::endl;
    }
    return true;
}