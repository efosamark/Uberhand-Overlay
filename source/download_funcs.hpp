#pragma once
#include <cstdio>
#include <curl/curl.h>
#include <zlib.h>
#include <zzip/zzip.h>
#include "string_funcs.hpp"
#include "get_funcs.hpp"
#include "path_funcs.hpp"
#include "debug_funcs.hpp"
#include "json_funcs.hpp"

const char* userAgent = "Mozilla/5.0 (Nintendo Switch; WebApplet) AppleWebKit/609.4 (KHTML, like Gecko) NF/6.0.2.21.3 NintendoBrowser/5.1.0.22474";

size_t writeCallbackFile(void* contents, size_t size, size_t nmemb, FILE* file) {
    // Callback function to write received data to a file
    size_t written = fwrite(contents, size, nmemb, file);
    return written;
}

size_t writeCallbackJson(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    } catch(std::bad_alloc &e) {
        // Handle memory allocation exceptions
        return 0;
    }
    return newLength;
}

SafeJson loadJsonFromUrl(const std::string& url) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    readBuffer.reserve(5 * 1024);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallbackJson);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            return nullptr;
        }
    }

    curl_global_cleanup();

    // Parse JSON data
    json_error_t error;
    json_t* root = json_loads(readBuffer.c_str(), 0, &error);

    if (!root) {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return nullptr;
    }

    return root;
}


bool downloadFile(const std::string& url, const std::string& toDestination) {
    std::string destination = toDestination;
    // Check if the destination ends with "/"
    if (destination.back() == '/') {
        createDirectory(destination);
        
        // Extract the filename from the URL
        size_t lastSlash = url.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string filename = url.substr(lastSlash + 1);
            destination += filename;
        } else {
            log("Invalid URL: %s", url.c_str());
            return false;
        }
    } else {
        createDirectory(destination.substr(0, destination.find_last_of('/'))+"/");
    }
    

    const int MAX_RETRIES = 3;
    int retryCount = 0;
    CURL* curl = nullptr;

    while (retryCount < MAX_RETRIES) {
        curl = curl_easy_init();
        if (curl) {
            // Successful initialization, break out of the loop
            break;
        } else {
            // Failed initialization, increment retry count and try again
            retryCount++;
            log("Error initializing curl. Retrying...");
        }
    }
    if (!curl) {
        // Failed to initialize curl after multiple attempts
        log("Error initializing curl after multiple retries.");
        return false;
    }
    
    FILE* file = fopen(destination.c_str(), "wb");
    if (!file) {
        log("Error opening file: %s", destination.c_str());
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallbackFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    // Set a user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);

    // Enable following redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // If you have a cacert.pem file, you can set it as a trusted CA
    // curl_easy_setopt(curl, CURLOPT_CAINFO, "sdmc:/config/uberhand/cacert.pem");

    CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) {
        log("Error downloading file: %s", curl_easy_strerror(result));
        curl_easy_cleanup(curl);
        fclose(file);
        // Delete the file if nothing was written to it
        std::remove(destination.c_str());
        return false;
    }

    curl_easy_cleanup(curl);
    // Check if the file is empty
    long fileSize = ftell(file);
    if (fileSize == 0) {
        log("Error downloading file: Empty file");
        std::remove(destination.c_str());
        fclose(file);
        return false;
    }
    fclose(file);
    return true;
}


bool unzipFile(const std::string& zipFilePath, const std::string& toDestination) {
    zzip_error_t errorCode{ ZZIP_NO_ERROR };
    ZZIP_DIR* dir = zzip_dir_open(zipFilePath.c_str(), &errorCode);
    if (!dir) {
        log("Error opening file: %s; error: %s", zipFilePath.c_str(), zzip_strerror(errorCode));
        return false;
    }

    bool success = true;
    ZZIP_DIRENT entry;
    while (zzip_dir_read(dir, &entry)) {
        if (entry.d_name[0] == '\0') continue;  // Skip empty entries

        std::string fileName = entry.d_name;
        std::string extractedFilePath = toDestination + fileName;
        
        // Extract the directory path from the extracted file path
        std::string directoryPath;
        if (extractedFilePath.back() != '/') {
            directoryPath = extractedFilePath.substr(0, extractedFilePath.find_last_of('/'))+"/";
        } else {
            directoryPath = extractedFilePath;
        }
        
        createDirectory(directoryPath);
        
        if (isDirectory(extractedFilePath)) {
            continue;
        }
        
        if (isDirectory(directoryPath)) {
            //log("directoryPath: success");
        } else {
            log("directoryPath: failure");
        }
        
        //log(std::string("directoryPath: ") + directoryPath);

        ZZIP_FILE* file = zzip_file_open(dir, entry.d_name, 0);
        if (file) {
            FILE* outputFile = fopen(extractedFilePath.c_str(), "wb");
            if (outputFile) {
                zzip_ssize_t bytesRead;
                const zzip_ssize_t bufferSize = 8192;
                char buffer[bufferSize];

                while ((bytesRead = zzip_file_read(file, buffer, bufferSize)) > 0) {
                    fwrite(buffer, 1, bytesRead, outputFile);
                }

                fclose(outputFile);
            } else {
                log("Error opening output file: %s", extractedFilePath.c_str());
                success = false;
            }

            zzip_file_close(file);
        } else {
            log("Error opening file in zip: %s", entry.d_name);
            success = false;
        }
    }

    zzip_dir_close(dir);
    return success;
}
