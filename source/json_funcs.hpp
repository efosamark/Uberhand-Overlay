#pragma once

#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <jansson.h>

class SafeJson {
public:
    SafeJson(json_t* json) : _json(json) {}
    SafeJson(const SafeJson& rh) : _json(rh._json) { json_incref(_json); };
    SafeJson(SafeJson&& rh) : _json(rh._json) { rh._json = nullptr; }
    ~SafeJson() { if (_json) { json_decref(_json); } }
    operator json_t* () { return _json; }
    operator bool() { return nullptr != _json; }
    auto operator->() { return _json; }
private:
    json_t* _json{ nullptr };
};

SafeJson readJsonFromFile(const std::string& filePath) {
    // Check if the file exists
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) != 0) {
        logMessage("ERROR: readJsonFromFile: failed to get stat for file \"" + filePath + '"');
        return nullptr;
    }

    // Open the file
    FILE* file = fopen(filePath.c_str(), "r");
    if (!file) {
        logMessage("ERROR: readJsonFromFile: failed to open file \"" + filePath + '"');
        return nullptr;
    }

    json_error_t error;
    json_t* root = json_loadf(file, JSON_DECODE_ANY, &error);
    if (!root) {
        fclose(file);
        logMessage("ERROR: readJsonFromFile: failed to load file as json \"" + filePath + '"');
        return nullptr;
    }

    // Clean up
    fclose(file);

    return root;
}

// int editJSONfile (const char* jsonFilePath, const std::string& offsetStr) {
//     // logMessage("Entered editJSONfile");

//     // Step 0: Read the contents of loader.kip to determine the hex value
//     std::string selectedHex = readHexDataAtOffset("/atmosphere/kips/loader.kip", "43555354", offsetStr);
//     logMessage("selectedHex: ");
//     logMessage(selectedHex);
//     selectedHex = "600027";

//     // Step 1: Read the JSON file into a Jansson JSON object
//     json_t *root;
//     json_error_t error;

//     FILE* jsonFile = fopen(jsonFilePath, "r");
//     if (!jsonFile) {
//         logMessage("Failed to open JSON file.");
//         return 1;
//     }

//     root = json_loadf(jsonFile, 0, &error);
//     fclose(jsonFile);

//     if (!root) {
//         logMessage("Error parsing JSON:");
//         logMessage(error.text);
//         return 1;
//     }

//     // Step 2: Iterate through the JSON array and find the object with the desired hex value
//     if (json_is_array(root)) {
//         size_t arraySize = json_array_size(root);
//         for (size_t i = 0; i < arraySize; ++i) {
//             json_t *arrayItem = json_array_get(root, i);
//             json_t *hexValue = json_object_get(arrayItem, "hex");
//             if (json_is_string(hexValue) && strcmp(json_string_value(hexValue), selectedHex.c_str()) == 0) {
//                 json_t *nameValue = json_object_get(arrayItem, "name");
//                 std::string selName = json_string_value(nameValue);
//                 selName += "- current";

//                 json_object_set_new(arrayItem, "name", json_string(selName.c_str()));
//                 break; // Exit the loop once the object is found and updated
//             }
//         }
//     } else {
//         // logMessage("JSON file does not contain an array at the root level.");
//         json_decref(root);
//         return 1;
//     }
//     int dotPosition = std::string(jsonFilePath).rfind('.');
//     const std::string selJsonFile = std::string(jsonFilePath).substr(0, dotPosition) + "-sel.json";
//     logMessage(selJsonFile);

//     // Step 3: Write the updated JSON object back to the file
//     logMessage(std::to_string(json_dump_file(root, selJsonFile.c_str(), JSON_INDENT(4))));

//     // Cleanup
//     json_decref(root);

//     // logMessage("JSON file updated successfully.");
//     return 0;
// }
