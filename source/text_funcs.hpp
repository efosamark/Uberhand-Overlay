#pragma once

#include "tesla.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <utility>

size_t lineWidth(const std::string& line, const int fontSize = 19, const bool monospace = false)
{
    size_t width = 0;
    for (const auto character : line) {
        auto glyph = tsl::gfx::Renderer::get().getGlyph(character, fontSize, monospace);
        width += static_cast<size_t>(glyph->xAdvance * glyph->currFontSize);
    }
    return width;
}

std::pair<std::string, int> readTextFromFile(const std::string& filePath)
{
    // log("Entered readTextFromFile");

    std::stringstream lines;
    std::string currentLine;
    std::ifstream file(filePath);
    std::vector<std::string> words;
    int lineCount = 0;
    const size_t maxRowWidth = 363; // magic number. width of a tesla List drawing area
    const size_t whitespaceWidth = lineWidth(" ");

    std::string line;
    while (std::getline(file, line)) {
        if (line == "\r" || line.empty()) {
            lines << std::endl; // Preserve empty lines
            lineCount++;
            continue;
        }

        std::istringstream lineStream(line);
        std::string word;
        std::string currentLine;
        size_t currentLineWidth = 0;

        while (lineStream >> word) {
            if (currentLine.empty()) {
                currentLine = word;
                currentLineWidth = lineWidth(currentLine);
            } else if (const auto wordWidth = lineWidth(word); currentLineWidth + whitespaceWidth + wordWidth <= maxRowWidth) {
                currentLine += " " + word;
                currentLineWidth = currentLineWidth + whitespaceWidth + wordWidth;
            } else {
                lines << currentLine << std::endl;
                currentLine = word;
                currentLineWidth = lineWidth(currentLine);
                lineCount++;
            }
        }

        if (!currentLine.empty()) {
            lines << currentLine << std::endl;
            lineCount++;
        }
    }

    file.close();
    return std::make_pair(lines.str(), lineCount);
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