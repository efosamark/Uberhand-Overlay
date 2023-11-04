#define NDEBUG
#define STBTT_STATIC
#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include <utils.hpp>


// Overlay booleans
static bool defaultMenuLoaded = true;
static std::string package = "";
bool applied = false;
bool deleted = false;
bool resetValue = false;
std::string prevValue = "";

class HelpOverlay : public tsl::Gui {
private:
    std::string helpPath;
    bool isInSection, inQuotes;

public:
    HelpOverlay(std::string helpPath) : helpPath(helpPath) {}
    ~HelpOverlay() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("HelpOverlay");

        std::pair<std::string, int> textDataPair;
        constexpr int lineHeight = 20;  // Adjust the line height as needed
        constexpr int fontSize = 19;    // Adjust the font size as needed

        auto rootFrame = new tsl::elm::OverlayFrame("Help", "Uberhand Package");
        auto list = new tsl::elm::List();

        if (!isFileOrDirectory(helpPath)) {
            list->addItem(new tsl::elm::CustomDrawer([lineHeight, fontSize](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString("Text file not found.\nContact the package dev.", false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
            }), fontSize + lineHeight);
            rootFrame->setContent(list);
            return rootFrame;
        } else {
            textDataPair = readTextFromFile(helpPath);
            std::string textdata = textDataPair.first;
            int textsize = textDataPair.second;
            if (!textdata.empty()) {
                list->addItem(new tsl::elm::CustomDrawer([lineHeight, fontSize, textdata](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                renderer->drawString(textdata.c_str(), false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
                }), fontSize * textsize + lineHeight);
                rootFrame->setContent(list);
                return rootFrame;
            }
        }
            return rootFrame;
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
};

class InfoOverlay : public tsl::Gui {
private:
    std::vector<std::string> kipInfoCommand;


public:
    InfoOverlay(std::vector<std::string> kipInfoCommand) : kipInfoCommand(kipInfoCommand) {}
    ~InfoOverlay() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("InfoOverlay");

        std::pair<std::string, int> textDataPair;
        constexpr int lineHeight = 20;  // Adjust the line height as needed
        constexpr int fontSize = 19;    // Adjust the font size as needed

        auto rootFrame = new tsl::elm::OverlayFrame("Backup Management", "Uberhand Package","",false,"\uE0E1  Back     \uE0E0  Apply     \uE0E2  Delete");
        auto list = new tsl::elm::List();

        textDataPair = dispCustData(kipInfoCommand[2], kipInfoCommand[1]);
        std::string textdata = textDataPair.first;
        int textsize = textDataPair.second;
        if (!textdata.empty()) {
            list->addItem(new tsl::elm::CustomDrawer([lineHeight, fontSize, textdata](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(textdata.c_str(), false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
            }), fontSize * textsize + lineHeight);
            rootFrame->setContent(list);
        }
            return rootFrame;
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
         if (keysDown & KEY_A) {
            copyFileOrDirectory(this->kipInfoCommand[1], "/atmosphere/kips/loader.kip");
            applied = true;
            tsl::goBack();
            return true;
         }
         if (keysDown & KEY_X) {
            deleteFileOrDirectory(this->kipInfoCommand[1]);
            tsl::goBack();
            applied = false;
            deleted = true;
            return true;
         }
        return false;
    }
};

class ConfigOverlay : public tsl::Gui {
private:
    std::string filePath, specificKey;
    bool isInSection, inQuotes;

public:
    ConfigOverlay(const std::string& file, const std::string& key = "") : filePath(file), specificKey(key) {}
    ~ConfigOverlay() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("ConfigOverlay");
        auto rootFrame = new tsl::elm::OverlayFrame(getNameFromPath(filePath), "Uberhand Config");
        auto list = new tsl::elm::List();

        std::string configFile = filePath + "/" + configFileName;

        std::string fileContent = getFileContents(configFile);
        if (!fileContent.empty()) {
            std::string line;
            std::istringstream iss(fileContent);
            std::string currentCategory;
            isInSection = false;
            while (std::getline(iss, line)) {
                if (line.empty() || line.find_first_not_of('\n') == std::string::npos) {
                    continue;
                }

                if (line.front() == '[' && line.back() == ']') {
                    if (!specificKey.empty()) {
                        if (line.substr(1, line.size() - 2) == specificKey) {
                            currentCategory = line.substr(1, line.size() - 2);
                            isInSection = true;
                            list->addItem(new tsl::elm::CategoryHeader(currentCategory));
                        } else {
                            currentCategory.clear();
                            isInSection = false;
                        }
                    } else {
                        currentCategory = line.substr(1, line.size() - 2);
                        isInSection = true;
                        list->addItem(new tsl::elm::CategoryHeader(currentCategory));
                    }
                } else if (isInSection) {
                    auto listItem = new tsl::elm::ListItem(line);
                    listItem->setClickListener([line, this, listItem](uint64_t keys) {
                        if (keys & KEY_A) {
                            std::istringstream iss(line);
                            std::string part;
                            std::vector<std::vector<std::string>> commandVec;
                            std::vector<std::string> commandParts;
                            inQuotes = false;

                            while (std::getline(iss, part, '\'')) {
                                if (!part.empty()) {
                                    if (!inQuotes) {
                                        std::istringstream argIss(part);
                                        std::string arg;
                                        while (argIss >> arg) {
                                            commandParts.emplace_back(arg);
                                        }
                                    } else {
                                        commandParts.emplace_back(part);
                                    }
                                }
                                inQuotes = !inQuotes;
                            }

                            commandVec.emplace_back(std::move(commandParts));
                            int result = interpretAndExecuteCommand(commandVec);
                            if (result == 0) {
                                listItem->setValue("DONE", tsl::PredefinedColors::Green);
                            } else if (result == 1) {
                                tsl::goBack();
                            } else {
                                listItem->setValue("FAIL", tsl::PredefinedColors::Red);
                            }
                            return true;
                        } else if (keys && (listItem->getValue() == "DONE" || listItem->getValue() == "FAIL")) {
                            listItem->setValue("");
                        }
                        return false;
                    });
                    list->addItem(listItem);
                }
            }
        } else {
            list->addItem(new tsl::elm::ListItem("Failed to open file: " + configFile));
        }

        rootFrame->setContent(list);
        return rootFrame;
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
};

class SelectionOverlay : public tsl::Gui {
private:
    std::string filePath, specificKey, pathPattern, pathPatternOn, pathPatternOff, jsonPath, jsonKey, itemName, parentDirName, lastParentDirName, textPath;
    std::vector<std::string> filesList, filesListOn, filesListOff, filterList, filterOnList, filterOffList;
    std::vector<std::vector<std::string>> commands;
    bool toggleState = false;
    json_t* jsonData;

public:
    SelectionOverlay(const std::string& file, const std::string& key = "", const std::vector<std::vector<std::string>>& cmds = {}) 
        : filePath(file), specificKey(key), commands(cmds) {}
    ~SelectionOverlay() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("SelectionOverlay");

        size_t fifthSlashPos = filePath.find('/', filePath.find('/', filePath.find('/', filePath.find('/') + 1) + 1) + 1);
        bool hasHelp = false;
        std::string helpPath = "";
        std::string menuName = "";
        if (fifthSlashPos != std::string::npos) {
            // Extract the substring up to the fourth slash
            helpPath = filePath.substr(0, fifthSlashPos);
            if (!specificKey.empty()) {
                menuName = specificKey;
                removeLastNumericWord(menuName);
                helpPath += "/Help/" + getNameWithoutPrefix(getNameFromPath(filePath)) + "/" + menuName.substr(1) + ".txt";
            } else {
                helpPath += "/Help/" + getNameWithoutPrefix(getNameFromPath(filePath)) + ".txt";
            }
            if (isFileOrDirectory(helpPath)) {
               hasHelp = true; 
            } else {
                helpPath = "";
            }

        }
        auto rootFrame = new tsl::elm::OverlayFrame(specificKey.empty() ? getNameWithoutPrefix(getNameFromPath(filePath)) : specificKey.substr(1),
                                                    "Uberhand Package", "", hasHelp);
        auto list = new tsl::elm::List();

        bool kipInfo = false;
        bool useFilter = false;
        bool useSource = false;
        bool useJson = false;
        bool useText = false;
        bool useToggle = false;
        bool useSplitHeader = false;
        bool markCurKip = false;
        bool markCurIni = false;
        std::string sourceIni = "";
        std::string sectionIni = "";
        std::string keyIni = "";
        std::string offset = "";
        std::pair<std::string, int> textDataPair;

        constexpr int lineHeight = 20;  // Adjust the line height as needed
        constexpr int fontSize = 19;    // Adjust the font size as needed
        
        for (const auto& cmd : commands) {
            if (cmd.size() > 1) {
                if (cmd[0] == "split") {
                    useSplitHeader = true;
                } else if (cmd[0] == "filter") {
                    filterList.push_back(cmd[1]);
                    useFilter = true;
                } else if (cmd[0] == "filter_on") {
                    filterOnList.push_back(cmd[1]);
                    useToggle = true;
                } else if (cmd[0] == "filter_off") {
                    filterOffList.push_back(cmd[1]);
                    useToggle = true;
                } else if (cmd[0] == "source") {
                    pathPattern = cmd[1];
                    useSource = true;
                } else if (cmd[0] == "source_on") {
                    pathPatternOn = cmd[1];
                    useToggle = true;
                } else if (cmd[0] == "source_off") {
                    pathPatternOff = cmd[1];
                    useToggle = true;
                } else if (cmd[0] == "json_source") {
                    jsonPath = preprocessPath(cmd[1]);
                    if (cmd.size() > 2) {
                        jsonKey = cmd[2]; //json display key
                    }
                    useJson = true;
                } else if (cmd[0] == "kip_info") {
                    jsonPath = preprocessPath(cmd[1]);
                    kipInfo = true;
                } else if (cmd[0] == "json_mark_cur_kip") {
                    jsonPath = preprocessPath(cmd[1]);
                    if (cmd.size() > 2) {
                        jsonKey = cmd[2]; //json display key
                    }
                    useJson = true;
                    if (cmd.size() > 3) {
                        offset = cmd[3];
                        markCurKip = true;
                    }
                } else if (cmd[0] == "json_mark_cur_ini") {
                    jsonPath = preprocessPath(cmd[1]);
                    if (cmd.size() > 2) {
                        jsonKey = cmd[2]; //json display key
                    }
                    useJson = true;
                    if (cmd.size() > 5) { // Enough parts are provided to mark the current ini value
                        sourceIni  = preprocessPath(cmd[3]);
                        sectionIni = cmd[4];
                        keyIni     = cmd[5];
                        markCurIni = true;
                    }
                } else if (cmd[0] == "text_source") {
                        textPath = preprocessPath(cmd[1]);
                        useText = true;
                }
            } 
        }

        // Get the list of files matching the pattern
        if (!useToggle) {
            if (useText) {
                if (!isFileOrDirectory(textPath)) {
                    list->addItem(new tsl::elm::CustomDrawer([lineHeight, fontSize](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                    renderer->drawString("Text file not found.\nContact the package dev.", false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
                    }), fontSize + lineHeight);
                    rootFrame->setContent(list);
                    return rootFrame;
                } else {
                    textDataPair = readTextFromFile(textPath);
                    std::string textdata = textDataPair.first;
                    int textsize = textDataPair.second;
                    if (!textdata.empty()) {
                        list->addItem(new tsl::elm::CustomDrawer([lineHeight, fontSize, textdata](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                        renderer->drawString(textdata.c_str(), false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
                        }), fontSize * textsize + lineHeight);
                        rootFrame->setContent(list);
                        return rootFrame;
                    }
                }
            } else if (useJson) {
                if (!isFileOrDirectory(jsonPath)) {
                    list->addItem(new tsl::elm::CustomDrawer([lineHeight, fontSize](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                    renderer->drawString("JSON file not found.\nContact the package dev.", false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
                    }), fontSize + lineHeight);
                    rootFrame->setContent(list);
                    return rootFrame;
                } else {
                    std::string currentHex = ""; // Is used to mark current value from the kip
                    bool detectSize = true;
                    bool searchCurrent = markCurKip || markCurIni ? true : false;
                    // create list of data in the json 
                    jsonData = readJsonFromFile(jsonPath);
                    if (jsonData && json_is_array(jsonData)) {
                        size_t arraySize = json_array_size(jsonData);
                        for (size_t i = 0; i < arraySize; ++i) {
                            json_t* item = json_array_get(jsonData, i);
                            if (item && json_is_object(item)) {
                                json_t* keyValue = json_object_get(item, jsonKey.c_str());
                                if (keyValue && json_is_string(keyValue)) {
                                    std::string name;
                                    json_t* hexValue = json_object_get(item, "hex");
                                    json_t* colorValue = json_object_get(item, "color");
                                    if (markCurKip && hexValue && searchCurrent) {
                                        char* hexValueStr = (char*)json_string_value(hexValue);
                                        if (detectSize) {
                                            size_t hexLength = strlen(hexValueStr);
                                            currentHex = readHexDataAtOffset("/atmosphere/kips/loader.kip", "43555354", offset, hexLength/2); // Read the data from kip with offset starting from 'C' in 'CUST'
                                            detectSize = false;
                                        }
                                        if (hexValueStr == currentHex) {
                                            name = std::string(json_string_value(keyValue)) + " - Current";
                                            searchCurrent = false;
                                        }
                                        else {
                                            name = json_string_value(keyValue);
                                        }
                                    } else if (markCurIni && hexValue && searchCurrent) {
                                        char* iniValueStr = (char*)json_string_value(hexValue);
                                        std::string iniValue = readIniValue(sourceIni, sectionIni, keyIni);
                                        if (iniValueStr == iniValue) {
                                            name = std::string(json_string_value(keyValue)) + " - Current";
                                            searchCurrent = false;
                                        }
                                        else {
                                            name = json_string_value(keyValue);
                                        }
                                    } else {
                                            name = json_string_value(keyValue);
                                    }
                                    if (colorValue) {
                                        name = name + " ::" + json_string_value(colorValue);
                                    }
                                    filesList.push_back(name);
                                }
                            }
                        }
                    }
                }
            } else if (useFilter || useSource) {
                filesList = getFilesListByWildcards(pathPattern);
                std::sort(filesList.begin(), filesList.end(), [](const std::string& a, const std::string& b) {
                    return getNameFromPath(a) < getNameFromPath(b);
                });
            } else if (kipInfo) {
                if (!isFileOrDirectory(jsonPath)) {
                    list->addItem(new tsl::elm::CustomDrawer([lineHeight, fontSize](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                    renderer->drawString("TMPL file not found.\nContact the package dev.", false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
                    }), fontSize + lineHeight);
                    rootFrame->setContent(list);
                    return rootFrame;
                } else {
                    textDataPair = dispCustData(jsonPath);
                    std::string textdata = textDataPair.first;
                    int textsize = textDataPair.second;
                    if (!textdata.empty()) {
                        list->addItem(new tsl::elm::CustomDrawer([lineHeight, fontSize, textdata](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                        renderer->drawString(textdata.c_str(), false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
                        }), fontSize * textsize + lineHeight);
                        rootFrame->setContent(list);
                        return rootFrame;
                    }
                }
            }
        } else {
            filesListOn = getFilesListByWildcards(pathPatternOn);
            filesListOff = getFilesListByWildcards(pathPatternOff);
            
            // Apply On Filter
            for (const auto& filterOnPath : filterOnList) {
                removeEntryFromList(filterOnPath, filesListOn);
            }
            // Apply Off Filter
            for (const auto& filterOnPath : filterOffList) {
                removeEntryFromList(filterOnPath, filesListOff);
            }
            
            
            // remove filterOnPath from filesListOn
            // remove filterOffPath from filesListOff
            
            
            filesList.reserve(filesListOn.size() + filesListOff.size());
            filesList.insert(filesList.end(), filesListOn.begin(), filesListOn.end());
            filesList.insert(filesList.end(), filesListOff.begin(), filesListOff.end());
            if (useSplitHeader) {
                std::sort(filesList.begin(), filesList.end(), [](const std::string& a, const std::string& b) {
                    std::string parentDirA = getParentDirNameFromPath(a);
                    std::string parentDirB = getParentDirNameFromPath(b);
                
                    // Compare parent directory names
                    if (parentDirA != parentDirB) {
                        return parentDirA < parentDirB;
                    } else {
                        // Parent directory names are the same, compare filenames
                        std::string filenameA = getNameFromPath(a);
                        std::string filenameB = getNameFromPath(b);
                
                        // Compare filenames
                        return filenameA < filenameB;
                    }
                });
            } else {
                std::sort(filesList.begin(), filesList.end(), [](const std::string& a, const std::string& b) {
                    return getNameFromPath(a) < getNameFromPath(b);
                });
            }

            
        }
        
        // Apply filter
        for (const auto& filterPath : filterList) {
            removeEntryFromList(filterPath, filesList);
        }
        
        
        if (!useSplitHeader){
            list->addItem(new tsl::elm::CategoryHeader(specificKey.substr(1)));
        }
        
        // Add each file as a menu item
        int count = 0;
        for (const std::string& file : filesList) {
            //if (file.compare(0, filterPath.length(), filterPath) != 0){
            itemName = getNameFromPath(file);
            if (!isDirectory(preprocessPath(file))) {
                itemName = dropExtension(itemName);
            }
            parentDirName = getParentDirNameFromPath(file);
            if (useSplitHeader && (lastParentDirName.empty() || (lastParentDirName != parentDirName))){
                list->addItem(new tsl::elm::CategoryHeader(removeQuotes(parentDirName)));
                lastParentDirName = parentDirName.c_str();
            }

            if (!useToggle) {
                std::string color = "Default";
                if (useJson) { // For JSON wildcards
                    size_t pos = file.find(" - ");
                    size_t colorPos = file.find(" ::");
                    std::string footer = "";
                    std::string optionName = file;
                    if (colorPos != std::string::npos) {
                        color = file.substr(colorPos + 3);
                        optionName = file.substr(0, colorPos);
                    }
                    if (pos != std::string::npos) {
                        footer = optionName.substr(pos + 2); // Assign the part after "&&" as the footer
                        optionName = optionName.substr(0, pos); // Strip the "&&" and everything after it
                    }
                    auto listItem = new tsl::elm::ListItem(optionName);
                    listItem->setValue(footer);
                    listItem->setClickListener([count, this, listItem, helpPath](uint64_t keys) { // Add 'command' to the capture list
                        if (keys & KEY_A) {
                            // Replace "{json_source}" with file in commands, then execute
                            if (listItem->getValue() != "DELETED") {
                                std::string countString = std::to_string(count);
                                std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(commands, countString, false, true, true);
                                int result = interpretAndExecuteCommand(modifiedCommands);
                                if (result == 0) {
                                    listItem->setValue("DONE", tsl::PredefinedColors::Green);
                                } else if (result == 1) {
                                    applied = true;
                                    tsl::goBack();
                                } else {
                                    listItem->setValue("FAIL", tsl::PredefinedColors::Red);
                                }
                            }
                            return true;
                        } else if (keys & KEY_Y && !helpPath.empty()) {
                            tsl::changeTo<HelpOverlay>(helpPath);
                        } else if (keys && (listItem->getValue() == "DONE" || listItem->getValue() == "FAIL")) {
                            listItem->setValue("");
                        }
                        return false;
                    });
                    if (color.compare(0, 1, "#") == 0){
                        listItem->setColor(tsl::PredefinedColors::Custom, color);
                    } else {
                        listItem->setColor(defineColor(color));  
                    }
                    list->addItem(listItem);
                } else {
                    auto listItem = new tsl::elm::ListItem(itemName);
                    bool listBackups = false;
                    std::vector<std::string> kipInfoCommand;
                    for (const std::vector<std::string>& row : commands) {
                        // Iterate over the inner vector (row)
                        for (const std::string& cell : row) {
                            if (cell == "kip_info") {
                                kipInfoCommand = row;
                                listBackups = true;
                            }
                        }
                    }
                    std::string concatenatedString;

                    // Iterate through the vector and append each string with a space
                    for (auto & str : kipInfoCommand) {
                        if (str == "{source}") {
                            str = replacePlaceholder(str, "{source}", file);
                        }
                    }
                    for (const std::string& str : kipInfoCommand) {
                        concatenatedString += str + " ";
                    }
                    if (!listBackups) {
                        listItem->setClickListener([file, this, listItem](uint64_t keys) { // Add 'command' to the capture list
                            if (keys & KEY_A) {
                                // Replace "{source}" with file in commands, then execute
                                if (!prevValue.empty()) {
                                    listItem->setValue(prevValue);
                                }
                                std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(commands, file);
                                int result = interpretAndExecuteCommand(modifiedCommands);
                                if (result == 0) {
                                    listItem->setValue("DONE", tsl::PredefinedColors::Green);
                                } else if (result == 1) {
                                    applied = true;
                                    tsl::goBack();
                                } else {
                                    listItem->setValue("FAIL", tsl::PredefinedColors::Red);
                                }
                                return true;
                            } else if (keys && (listItem->getValue() == "DONE" || listItem->getValue() == "FAIL")) {
                                listItem->setValue("");
                            }
                            return false;
                        });
                        list->addItem(listItem);
                    } else {
                        listItem->setClickListener([file, this, listItem, kipInfoCommand](uint64_t keys) { // Add 'command' to the capture list
                            if (keys & KEY_A) {
                                if (listItem->getValue() != "DELETED")
                                    tsl::changeTo<InfoOverlay>(kipInfoCommand);
                            }
                            return false;
                        });
                        list->addItem(listItem);
                    }
                }
            } else { // for handiling toggles
                auto toggleListItem = new tsl::elm::ToggleListItem(itemName, false, "On", "Off");

                // Set the initial state of the toggle item
                bool toggleStateOn = std::find(filesListOn.begin(), filesListOn.end(), file) != filesListOn.end();
                toggleListItem->setState(toggleStateOn);

                toggleListItem->setStateChangedListener([toggleListItem, file, toggleStateOn, this](bool state) {
                    if (!state) {
                        // Toggle switched to On
                        if (toggleStateOn) {
                            std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(commands, file, true);
                            interpretAndExecuteCommand(modifiedCommands);
                        } else {
                            // Handle the case where the command should only run in the source_on section
                            // Add your specific code here
                        }
                    } else {
                        // Toggle switched to Off
                        if (!toggleStateOn) {
                            std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(commands, file, true, false);
                            interpretAndExecuteCommand(modifiedCommands);
                        } else {
                            // Handle the case where the command should only run in the source_off section
                            // Add your specific code here
                        }
                    }
                });

                list->addItem(toggleListItem);
            } 
            count++;
        }
        rootFrame->setContent(list);
        return rootFrame;
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        } else if ((applied || deleted) && !keysDown) {
            deleted = false;
            tsl::elm::ListItem* focusedItem = dynamic_cast<tsl::elm::ListItem*>(this-> getFocusedElement());
            if (prevValue.empty())
                prevValue = focusedItem->getValue();
            if (applied) {
                resetValue = true;
                focusedItem->setValue("APPLIED", tsl::PredefinedColors::Green);
            }
            else
                focusedItem->setValue("DELETED", tsl::PredefinedColors::Red);
            applied = false;
            deleted = false;
            return true;
        } else if (resetValue && keysDown) {
            tsl::elm::ListItem* focusedItem = dynamic_cast<tsl::elm::ListItem*>(this-> getFocusedElement());
            if (focusedItem->getValue() == "APPLIED") {
                focusedItem->setValue(prevValue);
                prevValue = "";
                resetValue = false;
            }
            return true;
        }
        return false;
    }
};

class SubMenu : public tsl::Gui {
protected:
    std::string subPath, pathReplace, pathReplaceOn, pathReplaceOff;

public:
    SubMenu(const std::string& path) : subPath(path) {}
    ~SubMenu() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("SubMenu");

        int numSlashes = count(subPath.begin(), subPath.end(), '/');
        bool integrityCheck = verifyIntegrity(subPath);
        size_t fifthSlashPos = subPath.find('/', subPath.find('/', subPath.find('/', subPath.find('/') + 1) + 1) + 1);
        bool hasHelp = false;
        std::string helpPath = "";
        if (fifthSlashPos != std::string::npos) {
            // Extract the substring up to the fourth slash
            helpPath = subPath.substr(0, fifthSlashPos);
            helpPath += "/Help/" + getNameWithoutPrefix(getNameFromPath(subPath)) + ".txt";
            if (isFileOrDirectory(helpPath)) {
                hasHelp = true; 
            } else {
                helpPath = "";
            }
        }

        std::string viewPackage = getNameWithoutPrefix(package);
        std::string viewsubPath = getNameWithoutPrefix(getNameFromPath(subPath));

        auto rootFrame = new tsl::elm::OverlayFrame(getNameFromPath(viewsubPath), viewPackage, "" , hasHelp);
        auto list = new tsl::elm::List();

        // Add a section break with small text to indicate the "Commands" section
        // list->addItem(new tsl::elm::CategoryHeader("Commands"));

        // Load options from INI file in the subdirectory
        std::string subConfigIniPath = subPath + "/" + configFileName;
        std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> options = loadOptionsFromIni(subConfigIniPath);
        
        // logMessage("Processing SubMenu");

        // Populate the sub menu with options
        for (const auto& option : options) {
            std::string optionName = option.first;
            std::string footer; 
            bool usePattern = false;
            if (optionName[0] == '>') { // a subdirectory. add a menu item and skip to the next command
                auto subDirectory = optionName.substr(1);
                auto item = new tsl::elm::ListItem(getNameWithoutPrefix(subDirectory));
                item->setValue("\u25B6", tsl::PredefinedColors::White);
                item->setClickListener([&, subDirectory](u64 keys)->bool {
                    if (keys & KEY_A) {
                        tsl::changeTo<SubMenu>(subPath + subDirectory + '/');
                        return true;
                    }
                    return false;
                    });
                list->addItem(item);
                continue;
            }
            else if (optionName[0] == '*') {
                usePattern = true;
                optionName = optionName.substr(1); // Strip the "*" character on the left
                footer = "\u25B6";
            } else {
                size_t pos = optionName.find(" - ");
                if (pos != std::string::npos) {
                    footer = optionName.substr(pos + 2); // Assign the part after "&&" as the footer
                    optionName = optionName.substr(0, pos); // Strip the "&&" and everything after it
                }
            }
            
            // Extract the path pattern from commands
            bool useToggle = false;
            bool isSeparator = false;
            for (const auto& cmd : option.second) {
                if(cmd[0] == "separator"){
                    isSeparator = true;
                    break;
                }
                if (cmd.size() > 1) {
                    if (cmd[0] == "source") {
                        pathReplace = cmd[1];
                    } else if (cmd[0] == "source_on") {
                        pathReplaceOn = cmd[1];
                        useToggle = true;
                    } else if (cmd[0] == "source_off") {
                        pathReplaceOff = cmd[1];
                        useToggle = true;
                    } 
                    //else if (cmd[0] == "json_data") {
                    //    jsonPath = cmd[1];
                    //}
                } 
            }

            if (isSeparator) {
                auto item = new tsl::elm::CategoryHeader(optionName, true);
                list->addItem(item);
            } else if (usePattern || !useToggle){
                auto listItem = static_cast<tsl::elm::ListItem*>(nullptr);
                if ((footer == "\u25B6") || (footer.empty())) {
                    listItem = new tsl::elm::ListItem(optionName, footer);
                } else {
                    listItem = new tsl::elm::ListItem(optionName);
                    listItem->setValue(footer);
                }
                
                //std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(option.second, pathReplace);
                listItem->setClickListener([command = option.second, keyName = option.first, subPath = this->subPath, usePattern, listItem, helpPath](uint64_t keys) {
                    if (keys & KEY_A) {
                        if (usePattern) {
                            tsl::changeTo<SelectionOverlay>(subPath, keyName, command);
                        } else {
                            // Interpret and execute the command
                            int result = interpretAndExecuteCommand(command);
                            if (result == 0) {
                                listItem->setValue("DONE", tsl::PredefinedColors::Green);
                            } else if (result == 1) {
                                tsl::goBack();
                            } else {
                                listItem->setValue("FAIL", tsl::PredefinedColors::Red);
                            }
                        }
                        return true;
                    } else if (keys & KEY_X) {
                        listItem->setValue("");
                        tsl::changeTo<ConfigOverlay>(subPath, keyName);
                        return true;
                    }else if (keys & KEY_Y && !helpPath.empty()) {
                        tsl::changeTo<HelpOverlay>(helpPath);
                    } else if (keys && (listItem->getValue() == "DONE" || listItem->getValue() == "FAIL")) {
                        listItem->setValue("");
                    }
                    return false;
                });

                list->addItem(listItem);
            } else {
                auto toggleListItem = new tsl::elm::ToggleListItem(optionName, false, "On", "Off");
                // Set the initial state of the toggle item
                bool toggleStateOn = isFileOrDirectory(preprocessPath(pathReplaceOn));
                
                toggleListItem->setState(toggleStateOn);

                toggleListItem->setStateChangedListener([toggleStateOn, command = option.second, this](bool state) {
                    if (!state) {
                        // Toggle switched to On
                        if (toggleStateOn) {
                            std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(command, pathReplaceOn, true);
                            interpretAndExecuteCommand(modifiedCommands);
                        } else {
                            // Handle the case where the command should only run in the source_on section
                            // Add your specific code here
                        }
                    } else {
                        // Toggle switched to Off
                        if (!toggleStateOn) {
                            std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(command, pathReplaceOff, true, false);
                            interpretAndExecuteCommand(modifiedCommands);
                        } else {
                            // Handle the case where the command should only run in the source_off section
                            // Add your specific code here
                        }
                    }
                });

                list->addItem(toggleListItem);
            }

        }

        // Package Info
        PackageHeader packageHeader = getPackageHeaderFromIni(subConfigIniPath);

        constexpr int lineHeight = 20;  // Adjust the line height as needed
        constexpr int xOffset = 120;    // Adjust the horizontal offset as needed
        constexpr int fontSize = 16;    // Adjust the font size as needed
        int numEntries = 0;   // Adjust the number of entries as needed
        
        std::string packageSectionString = "";
        std::string packageInfoString = "";
        if (packageHeader.version != "") {
            packageSectionString += "Version\n";
            packageInfoString += (packageHeader.version+"\n").c_str();
            numEntries++;
        }
        if (packageHeader.creator != "") {
            packageSectionString += "Creator(s)\n";
            packageInfoString += (packageHeader.creator+"\n").c_str();
            numEntries++;
        }
        if (packageHeader.about != "") {
            std::string aboutHeaderText = "About\n";
            std::string::size_type aboutHeaderLength = aboutHeaderText.length();
            std::string aboutText = packageHeader.about;
    
            packageSectionString += aboutHeaderText;
            
            // Split the about text into multiple lines with proper word wrapping
            constexpr int maxLineLength = 28;  // Adjust the maximum line length as needed
            std::string::size_type startPos = 0;
            std::string::size_type spacePos = 0;
    
            while (startPos < aboutText.length()) {
                std::string::size_type endPos = std::min(startPos + maxLineLength, aboutText.length());
                std::string line = aboutText.substr(startPos, endPos - startPos);
        
                // Check if the current line ends with a space; if not, find the last space in the line
                if (endPos < aboutText.length() && aboutText[endPos] != ' ') {
                    spacePos = line.find_last_of(' ');
                    if (spacePos != std::string::npos) {
                        endPos = startPos + spacePos;
                        line = aboutText.substr(startPos, endPos - startPos);
                    }
                }
        
                packageInfoString += line + '\n';
                startPos = endPos + 1;
                numEntries++;
        
                // Add corresponding newline to the packageSectionString
                if (startPos < aboutText.length()) {
                    packageSectionString += std::string(aboutHeaderLength, ' ') + '\n';
                }
            }
    
        }

        
        // Remove trailing newline character
        if ((packageSectionString != "") && (packageSectionString.back() == '\n')) {
            packageSectionString = packageSectionString.substr(0, packageSectionString.size() - 1);
        }
        if ((packageInfoString != "") && (packageInfoString.back() == '\n')) {
            packageInfoString = packageInfoString.substr(0, packageInfoString.size() - 1);
        }
        
        
        if ((packageSectionString != "") && (packageInfoString != "")) {
            list->addItem(new tsl::elm::CategoryHeader("Package Info"));
            list->addItem(new tsl::elm::CustomDrawer([lineHeight, xOffset, fontSize, packageSectionString, packageInfoString](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                renderer->drawString(packageSectionString.c_str(), false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
                renderer->drawString(packageInfoString.c_str(), false, x + xOffset, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
            }), fontSize * numEntries + lineHeight);
        }

        if (numSlashes == 5 && integrityCheck) {
            int headerCh[] = {83, 112, 101, 99, 105, 97, 108, 32, 84, 104, 97, 110, 107, 115};
            int length = sizeof(headerCh) / sizeof(headerCh[0]);
            char* charArray = new char[length + 1];
            for (int i = 0; i < length; ++i) {
                charArray[i] = static_cast<char>(headerCh[i]);
            }
            charArray[length] = '\0';
            list->addItem(new tsl::elm::CategoryHeader(charArray));
            int checksum[] = {83, 112, 101, 99, 105, 97, 108, 32, 116, 104, 97, 110, 107, 115, 32, 116, 111, 32, 69, 102, 111, 115, 97, 109, 97, 114, 107, 44, 32, 73, 114, 110, 101, 32, 97, 110, 100, 32, 73, 51, 115, 101, 121, 46, 10, 87, 105, 116, 104, 111, 117, 116, 32, 116, 104, 101, 109, 32, 116, 104, 105, 115, 32, 119, 111, 117, 108, 100, 110, 39, 116, 32, 98, 101, 32, 112, 111, 115, 115, 98, 108, 101, 33};
            length = sizeof(checksum) / sizeof(checksum[0]);
            charArray = new char[length + 1];
            for (int i = 0; i < length; ++i) {
                charArray[i] = static_cast<char>(checksum[i]);
            }
            charArray[length] = '\0';
            list->addItem(new tsl::elm::CustomDrawer([lineHeight, xOffset, fontSize, charArray](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                renderer->drawString(charArray, false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
            }), fontSize * 2 + lineHeight);
        }

        rootFrame->setContent(list);
        
        return rootFrame;
    }

    virtual bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if ((keysDown & KEY_B)) {
            tsl::goBack();
            return true;
        } else if (applied && !keysDown) {
            applied = false;
            resetValue = true;
            tsl::elm::ListItem* focusedItem = dynamic_cast<tsl::elm::ListItem*>(this-> getFocusedElement());
            if (prevValue.empty())
                prevValue = focusedItem->getValue();
            focusedItem->setValue("APPLIED", tsl::PredefinedColors::Green);
            return true;
        } else if (resetValue && keysDown) {
            tsl::elm::ListItem* focusedItem = dynamic_cast<tsl::elm::ListItem*>(this-> getFocusedElement());
            if (focusedItem->getValue() == "APPLIED"){
                focusedItem->setValue(prevValue);
                prevValue = "";
                resetValue = false;
            }
            return true;
        }
        return false;
    }
};

class MainMenu;

class Package : public SubMenu {
public:
    Package(const std::string& path) : SubMenu(path) {}

    tsl::elm::Element* createUI() override {
        package = getNameFromPath(subPath);
        
        auto rootFrame = static_cast<tsl::elm::OverlayFrame*>(SubMenu::createUI());
        rootFrame->setTitle(getNameWithoutPrefix(package));
        rootFrame->setSubtitle("                             "); // FIXME: former subtitle is not fully erased if new string is shorter
        return rootFrame;
    }

    bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if ((keysDown & KEY_B)) {
            tsl::changeTo<MainMenu>();
            return true;
        }
        return false;
    }
};

class MainMenu : public tsl::Gui {
private:
    tsl::hlp::ini::IniData settingsData;
    std::string packageConfigIniPath = packageDirectory + configFileName;
    std::string menuMode, defaultMenuMode, inOverlayString, fullPath, optionName;
    bool useDefaultMenu = false, showOverlayVersions = false, showPackageVersions = true;
public:
    MainMenu() {}
    ~MainMenu() {}

    virtual tsl::elm::Element* createUI() override {
        defaultMenuMode = "overlays";
        menuMode = "overlays";
        
        createDirectory(packageDirectory);
        createDirectory(settingsPath);
        
        
        bool settingsLoaded = false;
        if (isFileOrDirectory(settingsConfigIniPath)) {
            settingsData = getParsedDataFromIniFile(settingsConfigIniPath);
            if (settingsData.count("ultrahand") > 0) {
                auto& ultrahandSection = settingsData["ultrahand"];
                if (ultrahandSection.count("last_menu") > 0) {
                    menuMode = ultrahandSection["last_menu"];
                    if (ultrahandSection.count("default_menu") > 0) {
                        defaultMenuMode = ultrahandSection["default_menu"];
                        if (ultrahandSection.count("in_overlay") > 0) {
                            settingsLoaded = true;
                        }
                    }
                }
                if (ultrahandSection["show_ovl_versions"] == "true") {
                    showOverlayVersions = true;
                } else {
                    setIniFileValue(settingsConfigIniPath, "ultrahand", "show_ovl_versions", "false");
                }
                if (ultrahandSection["show_pack_versions"] == "false") {
                    showPackageVersions = false;
                } else {
                    setIniFileValue(settingsConfigIniPath, "ultrahand", "show_pack_versions", "true");
                }
            }
        }
        if (!settingsLoaded) { // write data if settings are not loaded
            setIniFileValue(settingsConfigIniPath, "ultrahand", "default_menu", defaultMenuMode);
            setIniFileValue(settingsConfigIniPath, "ultrahand", "last_menu", menuMode);
            setIniFileValue(settingsConfigIniPath, "ultrahand", "in_overlay", "false");
        }
        copyTeslaKeyComboToUltraHand();
        //setIniFileValue(settingsConfigIniPath, "ultrahand", "in_overlay", "false");
        
        
        if ((defaultMenuMode == "overlays") || (defaultMenuMode == "packages")) {
            if (defaultMenuLoaded) {
                menuMode = defaultMenuMode.c_str();
                defaultMenuLoaded = false;
            }
        } else {
            defaultMenuMode = "last_menu";
            setIniFileValue(settingsConfigIniPath, "ultrahand", "default_menu", defaultMenuMode);
        }
        
        std::string versionLabel = APP_VERSION+std::string("   (")+envGetLoaderInfo()+std::string(")");
        auto rootFrame = new tsl::elm::OverlayFrame("Uberhand", versionLabel, menuMode);
        auto list = new tsl::elm::List();

        //loadOverlayFiles(list);
        
        int count = 0;
        
        if (menuMode == "overlays") {
            // Load overlay files
            std::vector<std::string> overlayFiles;
            std::vector<std::string> files = getFilesListByWildcard(overlayDirectory+"*.ovl");
            for (const auto& file : files) {
                // Check if the file is an overlay file (*.ovl)
                if (file.substr(file.length() - 4) == ".ovl" && getNameFromPath(file) != "ovlmenu.ovl") {
                    overlayFiles.push_back(file);
                }
            }
            std::sort(overlayFiles.begin(), overlayFiles.end()); // Sort overlay files alphabetically

            if (!overlayFiles.empty()) {
            
                for (const auto& overlayFile : overlayFiles) {
                    if (getNameFromPath(overlayFile) == "ovlmenu.ovl")
                        continue;

                    // Get the path of the overlay file
                    //std::string overlayPath = overlayDirectory + "/" + overlayFile;

                    // Get the name and version of the overlay file
                    auto [result, overlayName, overlayVersion] = getOverlayInfo(overlayFile);
                    if (result != ResultSuccess)
                        continue;

                    // Create a new list item with the overlay name and version
                    
                    std::string fileName = getNameFromPath(overlayFile);
                    if (!fileName.empty()) {
                        if (fileName.substr(0, 2) == "0_") {
                            overlayName = "\u2605 "+overlayName;
                        }
                    }
                    
                    auto* listItem = new tsl::elm::ListItem(overlayName);
                    if (showOverlayVersions)
                        listItem->setValue(overlayVersion);

                    // Add a click listener to load the overlay when clicked upon
                    listItem->setClickListener([overlayFile](s64 key) {
                        if (key & KEY_A) {
                            // Load the overlay here
                            setIniFileValue(settingsConfigIniPath, "ultrahand", "in_overlay", "true"); // this is handled within tesla.hpp
                            tsl::setNextOverlay(overlayFile);
                            //envSetNextLoad(overlayPath, "");
                            tsl::Overlay::get()->close();
                            return true;
                        } else if (key & KEY_PLUS) {
                            std::string fileName = getNameFromPath(overlayFile);
                            if (!fileName.empty()) {
                                if (fileName.substr(0, 2) != "0_") {
                                    std::string newFilePath = getParentDirFromPath(overlayFile) + "0_" + fileName;
                                    moveFileOrDirectory(overlayFile, newFilePath);
                                } else {
                                    fileName = fileName.substr(2); // Remove "0_" from fileName
                                    std::string newFilePath = getParentDirFromPath(overlayFile) + fileName;
                                    moveFileOrDirectory(overlayFile, newFilePath);
                                }
                            }
                            tsl::changeTo<MainMenu>();
                            return true;
                        }
                        return false;
                    });

                    if (count == 0) {
                        list->addItem(new tsl::elm::CategoryHeader("Overlays"));
                    }
                    list->addItem(listItem);
                    count++;
                }
            }
        }
        
        if (menuMode == "packages" ) {
            // Create the directory if it doesn't exist
            createDirectory(packageDirectory);

            // Load options from INI file
            std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> options = loadOptionsFromIni(packageConfigIniPath, true);
        

            // Load subdirectories
            std::vector<std::string> subdirectories = getSubdirectories(packageDirectory);
            
            for (size_t i = 0; i < subdirectories.size(); ++i) {
                std::string& subdirectory = subdirectories[i];
                std::string subPath = packageDirectory + subdirectory + "/";
                std::string starFilePath = subPath + ".star";
            
                if (isFileOrDirectory(starFilePath)) {
                    // Add "0_" to subdirectory within subdirectories
                    subdirectory = "0_" + subdirectory;
                }
            }
            
            std::sort(subdirectories.begin(), subdirectories.end()); // Sort subdirectories alphabetically
            
            count = 0;
            for (const auto& taintedSubdirectory : subdirectories) {
                //bool usingStar = false;
                std::string subdirectory = taintedSubdirectory;
                std::string subdirectoryIcon = "";

                if (subdirectory.find("0_") == 0) {
                    subdirectory = subdirectory.substr(2); // Remove "0_" from the beginning
                    subdirectoryIcon = "\u2605 ";
                }

                std::string subPath = packageDirectory + subdirectory + "/";
                std::string configFilePath = subPath + "config.ini";
            
                if (isFileOrDirectory(configFilePath)) {
                    PackageHeader packageHeader = getPackageHeaderFromIni(subPath + configFileName);
                    if (count == 0) {
                        // Add a section break with small text to indicate the "Packages" section
                        list->addItem(new tsl::elm::CategoryHeader("Packages"));
                    }
                    
                    auto listItem = new tsl::elm::ListItem(subdirectoryIcon + getNameWithoutPrefix(subdirectory));
                    if (showPackageVersions)
                        listItem->setValue(packageHeader.version);
            
                    listItem->setClickListener([this, subPath = packageDirectory + subdirectory + "/"](uint64_t keys) {
                        if (keys & KEY_A) {
                            tsl::changeTo<Package>(subPath);
                            return true;
                        } else if (keys & KEY_PLUS) {
                            std::string starFilePath = subPath + ".star";
                            if (isFileOrDirectory(starFilePath)) {
                                deleteFileOrDirectory(starFilePath);
                            } else {
                                createTextFile(starFilePath, "");
                            }
                            tsl::changeTo<MainMenu>();
                            return true;
                        }
                        return false;
                    });

                    list->addItem(listItem);
                    count++;
                }

            }

        
            count = 0;
            //std::string optionName;
            // Populate the menu with options
            for (const auto& option : options) {
                optionName = option.first;
            
                // Check if it's a subdirectory
                fullPath = packageDirectory + optionName;
                if (count == 0) {
                    // Add a section break with small text to indicate the "Packages" section
                    list->addItem(new tsl::elm::CategoryHeader("Commands"));
                }
                
                //std::string header;
                //if ((optionName == "Shutdown")) {
                //    header = "\uE0F3  ";
                //}
                //else if ((optionName == "Safe Reboot") || (optionName == "L4T Reboot")) {
                //    header = "\u2194  ";
                //}
                //auto listItem = new tsl::elm::ListItem(header+optionName);
                auto listItem = new tsl::elm::ListItem(optionName);
                
                std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(option.second, fullPath);
                listItem->setClickListener([this, command = modifiedCommands, subPath = optionName, listItem](uint64_t keys) {
                    if (keys & KEY_A) {
                        // Check if it's a subdirectory
                        struct stat entryStat;
                        std::string newPath = packageDirectory + subPath;
                        if (stat(fullPath.c_str(), &entryStat) == 0 && S_ISDIR(entryStat.st_mode)) {
                            tsl::changeTo<SubMenu>(newPath);
                        } else {
                            // Interpret and execute the command
                            int result = interpretAndExecuteCommand(command);
                            if (result == 0) {
                                listItem->setValue("DONE", tsl::PredefinedColors::Green);
                            } else if (result == 1) {
                                tsl::goBack();
                            } else {
                                listItem->setValue("FAIL", tsl::PredefinedColors::Red);
                            }
                        }

                        return true;
                    } else if (keys && (listItem->getValue() == "DONE" || listItem->getValue() == "FAIL")) {
                            listItem->setValue("");
                    }
                    return false;
                });

                list->addItem(listItem);
                count++;
            }
        }

        rootFrame->setContent(list);

        return rootFrame;
    }

    virtual bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if ((keysDown & KEY_DRIGHT) && !(keysHeld & ~KEY_DRIGHT)) {
            if (menuMode != "packages") {
                setIniFileValue(settingsConfigIniPath, "ultrahand", "last_menu", "packages");
                tsl::changeTo<MainMenu>();
                return true;
            }
        }
        if ((keysDown & KEY_DLEFT) && !(keysHeld & ~KEY_DLEFT)) {
            if (menuMode != "overlays") {
                setIniFileValue(settingsConfigIniPath, "ultrahand", "last_menu", "overlays");
                tsl::changeTo<MainMenu>();
                return true;
            }
        }
        if (keysDown & KEY_B) {
            tsl::Overlay::get()->close();
            return true;
        }
        return false;
    }
};

class Overlay : public tsl::Overlay {
public:
    
    virtual void initServices() override {
        fsdevMountSdmc();
        splInitialize();
        spsmInitialize();
        ASSERT_FATAL(socketInitializeDefault());
        ASSERT_FATAL(nifmInitialize(NifmServiceType_User));
        ASSERT_FATAL(timeInitialize());
        ASSERT_FATAL(smInitialize());
    }

    virtual void exitServices() override {
        socketExit();
        nifmExit();
        timeExit();
        smExit();
        spsmExit();
        splExit();
        fsdevUnmountAll();
    }

    virtual void onShow() override {
        //if (rootFrame != nullptr) {
        //    tsl::Overlay::get()->getCurrentGui()->removeFocus();
        //    rootFrame->invalidate();
        //    tsl::Overlay::get()->getCurrentGui()->requestFocus(rootFrame, tsl::FocusDirection::None);
        //}
    }   // Called before overlay wants to change from invisible to visible state
    virtual void onHide() override {}   // Called before overlay wants to change from visible to invisible state

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        return initially<MainMenu>();  // Initial Gui to load. It's possible to pass arguments to its constructor like this
    }
};



int main(int argc, char* argv[]) {
    return tsl::loop<Overlay, tsl::impl::LaunchFlags::None>(argc, argv);
}
