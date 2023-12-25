#pragma once
#include <string>

constexpr const char* WhitespaceCharacters = " \t\n\r\f\v";

// Trim leading and trailing whitespaces from a string
static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(WhitespaceCharacters);
    size_t last = str.find_last_not_of(WhitespaceCharacters);
    if (first == std::string::npos || last == std::string::npos) {
        return "";
    }
    return str.substr(first, last - first + 1);
}

// In-place trimming of whitespace characters for rvalue strings
static inline std::string trim(std::string&& str) {
    const size_t first = str.find_first_not_of(WhitespaceCharacters);
    if (first == std::string::npos) {
        return "";
    }
    str.erase(0, first);
    str.erase(str.find_last_not_of(WhitespaceCharacters) + 1);
    return str;
}

// In-place trimming of whitespace characters
static inline void trimInPlace(std::string& str) {
    const size_t first = str.find_first_not_of(WhitespaceCharacters);
    if (first == std::string::npos) {
        str.erase();
        return;
    }
    str.erase(0, first);
    str.erase(str.find_last_not_of(WhitespaceCharacters) + 1);
}

std::string removeQuotes(const std::string& str) {
    std::size_t firstQuote = str.find_first_of("'\"");
    std::size_t lastQuote = str.find_last_of("'\"");
    if (firstQuote != std::string::npos && lastQuote != std::string::npos && firstQuote < lastQuote) {
        return str.substr(firstQuote + 1, lastQuote - firstQuote - 1);
    }
    return str;
}


std::string replaceMultipleSlashes(const std::string& input) {
    std::string output;
    bool previousSlash = false;

    for (char c : input) {
        if (c == '/') {
            if (!previousSlash) {
                output.push_back(c);
            }
            previousSlash = true;
        } else {
            output.push_back(c);
            previousSlash = false;
        }
    }

    return output;
}

std::string removeLeadingSlash(const std::string& pathPattern) {
    if (!pathPattern.empty() && pathPattern[0] == '/') {
        return pathPattern.substr(1);
    }
    return pathPattern;
}

std::string removeEndingSlash(const std::string& pathPattern) {
    if (!pathPattern.empty() && pathPattern.back() == '/') {
        return pathPattern.substr(0, pathPattern.length() - 1);
    }
    return pathPattern;
}


std::string preprocessPath(const std::string& path) {
    std::string formattedPath = replaceMultipleSlashes(removeQuotes(path));
    if (formattedPath.compare(0, 5, "sdmc:") != 0) {
        return std::string("sdmc:") + formattedPath;
    } else {
        return formattedPath;
    }
}


std::string preprocessUrl(const std::string& path) {
    std::string formattedPath = removeQuotes(path);
    if ((formattedPath.compare(0, 7, "http://") == 0) || (formattedPath.compare(0, 8, "https://") == 0)) {
        return formattedPath;
    } else {
        return std::string("https://") + formattedPath;
    }
}

std::string dropExtension(const std::string& filename) {
    size_t lastDotPos = filename.find_last_of(".");
    if (lastDotPos != std::string::npos) {
        return filename.substr(0, lastDotPos);
    }
    return filename;
}

std::string getExtension(const std::string& filename) {
    size_t lastDotPos = filename.find_last_of(".");
    if (lastDotPos != std::string::npos) {
        return filename.substr(lastDotPos + 1);
    }
    return filename;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.length(), prefix) == 0;
}

// Path functions
bool isDirectory(const std::string& path) {
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) == 0) {
        return S_ISDIR(pathStat.st_mode);
    }
    return false;
}
bool isFileOrDirectory(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

std::string getSubstringAfterLastSlash(const std::string& str) {
    size_t lastSlashPos = str.rfind('/');
    
    if (lastSlashPos != std::string::npos) {
        return str.substr(lastSlashPos + 1);
    }
    
    return "";
}