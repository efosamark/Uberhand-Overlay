#define NDEBUG
#define STBTT_STATIC
#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include <FanSliderOverlay.hpp>
#include <KipInfoOverlay.hpp>
#include <JsonInfoOverlay.hpp>
#include <utils.hpp>


// Overlay booleans
static bool defaultMenuLoaded = true;
static std::string package = "";
bool enableConfigNav = false;
bool showCurInMenu   = false;
std::string kipVersion = "";
bool DownloadProcessing = false;

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

    bool adjuct_top, adjuct_bot = false;

    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (this->adjuct_top) { // Adjust cursor to the top item after jump bot-top
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            this->adjuct_top = false;
        }
        if (this->adjuct_bot) {  // Adjust cursor to the top item after jump top-bot
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            this->adjuct_bot = false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DDOWN) || (keysDown & KEY_DOWN)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::UpMax);
                this->adjuct_top = true; // Go one item 
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            }
            return false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DUP) || (keysDown & KEY_UP)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::DownMax);
                this->adjuct_bot = true;
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            }
            return false;
        }
        if (keysDown & KEY_L) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpNum);
            return true;
        }
        if (keysDown & KEY_R) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownNum);
            return true;
        }if (keysDown & KEY_ZL) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpMax);
            return true;
        }
        if (keysDown & KEY_ZR) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownMax);
            return true;
        }
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        return false;
    }
    virtual std::string getClass() {return "ConfigOverlay";}
};

class SelectionOverlay : public tsl::Gui {
private:
    std::string filePath, specificKey, pathPattern, pathPatternOn, pathPatternOff, jsonPath, jsonKey, itemName, parentDirName, lastParentDirName, textPath, footer;
    std::vector<std::string> filesList, filesListOn, filesListOff, filterList, filterOnList, filterOffList;
    std::vector<std::vector<std::string>> commands;
    bool toggleState = false;
    bool searchCurrent;
    tsl::elm::ListItem* savedItem = nullptr;

public:
    SelectionOverlay(const std::string& file, const std::string& key = "", const std::vector<std::vector<std::string>>& cmds = {}, std::string footer = "") 
        : filePath(file), specificKey(key), footer(footer), commands(cmds) {}
    ~SelectionOverlay() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("SelectionOverlay");

        bool hasHelp = false;
        std::string helpPath;
        std::string menuName;
        size_t fifthSlashPos = filePath.find('/', filePath.find('/', filePath.find('/', filePath.find('/') + 1) + 1) + 1);
        if (fifthSlashPos != std::string::npos) {
            // Extract the substring up to the fourth slash
            helpPath = filePath.substr(0, fifthSlashPos);
            if (!specificKey.empty()) {
                menuName = specificKey;
                removeLastNumericWord(menuName);
                helpPath += "/Help/" + getNameFromPath(filePath) + "/" + menuName + ".txt";
            } else {
                helpPath += "/Help/" + getNameFromPath(filePath) + ".txt";
            }
            if (isFileOrDirectory(helpPath)) {
               hasHelp = true; 
            } else {
                helpPath = "";
            }
        }
        auto rootFrame = new tsl::elm::OverlayFrame(specificKey.empty() ? getNameFromPath(filePath) : specificKey,
                                                    "Uberhand Package", "", hasHelp, footer);
        auto list = new tsl::elm::List();

        bool ramInfo = false;
        bool useFilter = false;
        bool useSource = false;
        bool useJson = false;
        bool useText = false;
        bool useToggle = false;
        bool useSplitHeader = false;
        bool markCurKip = false;
        bool markCurIni = false;
        std::string ramPath = "";
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
                } else if (cmd[0] == "ram_info") {
                    ramPath = preprocessPath(cmd[1]);
                    ramInfo = true;
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
                    searchCurrent = markCurKip || markCurIni ? true : false;
                    // create list of data in the json 
                    json_t* jsonData = readJsonFromFile(jsonPath);
                    if (jsonData && json_is_array(jsonData)) {
                        size_t arraySize = json_array_size(jsonData);
                        for (size_t i = 0; i < arraySize; ++i) {
                            json_t* item = json_array_get(jsonData, i);
                            if (item && json_is_object(item)) {
                                json_t* keyValue = json_object_get(item, jsonKey.c_str());
                                if (keyValue && json_is_string(keyValue)) {
                                    std::string name;
                                    json_t* hexValue = json_object_get(item, "hex");
                                    json_t* decValue = json_object_get(item, "dec");
                                    bool hexOrDec = (hexValue || decValue);
                                    json_t* colorValue = json_object_get(item, "color");
                                    if (markCurKip && hexOrDec && searchCurrent) {
                                        std::string valueStr;
                                        if (hexValue) {
                                            valueStr = json_string_value(hexValue);
                                        } else if (decValue) {
                                            valueStr = json_string_value(decValue);
                                        }
                                        size_t hexLength = strlen(valueStr.c_str())/2;
                                        if (detectSize) {
                                            detectSize = false;
                                            currentHex = readHexDataAtOffset("/atmosphere/kips/loader.kip", "43555354", offset, hexLength); // Read the data from kip with offset starting from 'C' in 'CUST'
                                            if (decValue) {
                                                currentHex = std::to_string(reversedHexToInt(currentHex));
                                            }
                                        }
                                        if (valueStr == currentHex) {
                                            name = std::string(json_string_value(keyValue)) + " - " + checkmarkChar;
                                            searchCurrent = false;
                                        }
                                        else {
                                            name = json_string_value(keyValue);
                                        }
                                    } else if (markCurIni && hexValue && searchCurrent) {
                                        char* iniValueStr = (char*)json_string_value(hexValue);
                                        std::string iniValue = readIniValue(sourceIni, sectionIni, keyIni);
                                        if (iniValueStr == iniValue) {
                                            name = std::string(json_string_value(keyValue)) +  " - " + checkmarkChar;
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
                    if (jsonData) {
                        json_decref(jsonData);
                    }
                }
            } else if (useFilter || useSource) {
                filesList = getFilesListByWildcards(pathPattern);
                std::sort(filesList.begin(), filesList.end(), [](const std::string& a, const std::string& b) {
                    return getNameFromPath(a) < getNameFromPath(b);
                });
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
        
        // Add each file as a menu item
        int count = 0;
        std::string jsonSep = "";
        bool isFirst = true;
        bool hasSep = false;
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
                    size_t pos2 = file.find("`");
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
                    if (pos2 == 0) { // separator
                        if (isFirst) {
                            jsonSep = optionName.substr(1);
                            hasSep = true;
                        }
                        else {
                            auto item = new tsl::elm::CategoryHeader(optionName.substr(1), true);
                            list->addItem(item);
                        }
                    }
                    else {
                        isFirst = false;
                        auto listItem = new tsl::elm::ListItem(optionName);
                        if (ramInfo) {
                            listItem->setClickListener([count, this, listItem, helpPath, ramPath](uint64_t keys) { // Add 'command' to the capture list
                                if (keys & KEY_A) {
                                    tsl::changeTo<JsonInfoOverlay>(ramPath, listItem->getText());
                                    return true;
                                } else if (keys & KEY_Y && !helpPath.empty()) {
                                    tsl::changeTo<HelpOverlay>(helpPath);
                                } else if (keys && (listItem->getValue() == "DONE" || listItem->getValue() == "FAIL")) {
                                    listItem->setValue("");
                                }
                                return false;
                            });
                            list->addItem(listItem);
                        } else {
                            listItem->setValue(footer);
                            listItem->setClickListener([count, this, listItem, helpPath](uint64_t keys) { // Add 'command' to the capture list
                                if (keys & KEY_A) {
                                    if (listItem->getValue() == "APPLIED" && !prevValue.empty()) {
                                        listItem->setValue(prevValue);
                                        prevValue = "";
                                        resetValue = false;
                                    }
                                    if (listItem->getValue() != "DELETED") {
                                        // Replace "{json_source}" with file in commands, then execute
                                        std::string countString = std::to_string(count);
                                        std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(commands, countString, false, true, true);
                                        int result = interpretAndExecuteCommand(modifiedCommands);
                                        if (result == 0) {
                                            listItem->setValue("DONE", tsl::PredefinedColors::Green);
                                        } else if (result == 1) {
                                            applied = true;
                                            prevValue = listItem->getText();
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
                            // Find the curent list item to jump to
                            size_t checkmarkPos = listItem->getValue().find(checkmarkChar);
                            if (checkmarkPos != std::string::npos) {
                                savedItem = listItem;
                            }
                            list->addItem(listItem);
                        }
                    }
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
                        listItem->setClickListener([file, this, listItem](uint64_t keys) {
                            if (keys & KEY_A) {
                                if (listItem->getValue() == "APPLIED" && !prevValue.empty()) {
                                    listItem->setValue(prevValue);
                                    prevValue = "";
                                    resetValue = false;
                                }
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
                        listItem->setClickListener([&, file, this, listItem, kipInfoCommand](uint64_t keys) { // Add 'command' to the capture list
                            if (keys & KEY_A) {
                                if (listItem->getValue() != "DELETED") {
                                    tsl::changeTo<KipInfoOverlay>(kipInfoCommand);
                                }
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
        if (hasSep) {
            if (!jsonSep.empty()) {
                list->addItem(new tsl::elm::CategoryHeader(jsonSep), 0, 0);
            }
        } else if (!useSplitHeader){
            list->addItem(new tsl::elm::CategoryHeader(specificKey), 0, 0);
        }
        rootFrame->setContent(list);
        return rootFrame;
    }

    bool adjuct_top, adjuct_bot = false;

    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (this->savedItem != nullptr) {
            if (this->savedItem != this->getFocusedElement()) {
                while(this->savedItem != this->getFocusedElement()) {
                    this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
                }
            }
            this->savedItem = nullptr;
        }
        if (this->adjuct_top) { // Adjust cursor to the top item after jump bot-top
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            this->adjuct_top = false;
        }
        if (this->adjuct_bot) {  // Adjust cursor to the top item after jump top-bot
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            this->adjuct_bot = false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DDOWN) || (keysDown & KEY_DOWN)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::UpMax);
                this->adjuct_top = true; // Go one item 
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            }
            return false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DUP) || (keysDown & KEY_UP)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::DownMax);
                this->adjuct_bot = true;
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            }
            return false;
        }
        if (keysDown & KEY_L) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpNum);
            return true;
        }
        if (keysDown & KEY_R) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownNum);
            return true;
        }if (keysDown & KEY_ZL) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpMax);
            return true;
        }
        if (keysDown & KEY_ZR) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownMax);
            return true;
        }
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        } else if ((applied || deleted) && !keysDown) {
            deleted = false;
            tsl::elm::ListItem* focusedItem = dynamic_cast<tsl::elm::ListItem*>(this->getFocusedElement());
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
            if (this->getFocusedElement()->getClass()  == "ListItem" ){
                tsl::elm::ListItem* focusedItem = dynamic_cast<tsl::elm::ListItem*>(this->getFocusedElement());
                if (focusedItem->getClass() == "ListItem" && focusedItem->getValue() == "APPLIED") {
                    focusedItem->setValue(prevValue);
                    prevValue = "";
                    resetValue = false;
                }
            }
            return true;
        }
        return false;
    }

    virtual std::string getClass() {return "SelectionOverlay";}
};


class SubMenu : public tsl::Gui {
protected:
    std::string subPath, pathReplace, pathReplaceOn, pathReplaceOff;

public:
    SubMenu(const std::string& path) : subPath(path) {}
    ~SubMenu() {}

    std::string findCurrent(std::string jsonPath, std::string offset) {
        std::string dispValue, searchKey;
        size_t hexLength;

        json_t* jsonData = readJsonFromFile(jsonPath);
        if (jsonData && json_is_array(jsonData)) {
            size_t arraySize = json_array_size(jsonData);
            if (arraySize < 2) {
                json_decref(jsonData);
                return "\u25B6";
            }
            json_t* item = json_array_get(jsonData, 1);
            if (item && json_is_object(item)) {
                std::string name;
                json_t* hexValue = json_object_get(item, "hex");
                json_t* decValue = json_object_get(item, "dec");
                if ((hexValue && json_is_string(hexValue))) {
                    auto valueStr = (char*)json_string_value(hexValue);
                    hexLength = strlen(valueStr)/2;
                    searchKey = "hex";
                }
                else if ((decValue && json_is_string(decValue))) {
                    auto valueStr = (char*)json_string_value(decValue);
                    hexLength = strlen(valueStr)/2;
                    searchKey = "dec";
                }
                else {
                    json_decref(jsonData);
                    return "\u25B6";
                }
                std::string currentHex = readHexDataAtOffset("/atmosphere/kips/loader.kip", "43555354", offset, hexLength);
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
                                        cur_name = cur_name.substr(0, footer_pos);
                                    }
                                    json_decref(jsonData);
                                    return cur_name;
                                }
                            }
                        }
                    }
                }

            }
            json_decref(jsonData);
        }
        return "\u25B6";

    }

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
            helpPath += "/Help/" + getNameFromPath(subPath) + ".txt";
            if (isFileOrDirectory(helpPath)) {
                hasHelp = true; 
            } else {
                helpPath = "";
            }
        }

        std::string viewPackage = package;
        std::string viewsubPath = getNameFromPath(subPath);

        auto rootFrame = new tsl::elm::OverlayFrame(getNameFromPath(viewsubPath), viewPackage, "" , hasHelp);
        auto list = new tsl::elm::List();

        if (!kipVersion.empty()) {
            constexpr int lineHeight = 20;  // Adjust the line height as needed
            constexpr int fontSize = 19;    // Adjust the font size as needed
            std::string curKipVer = readHexDataAtOffset("/atmosphere/kips/loader.kip", "43555354", "4", 3);
            int i_curKipVer = reversedHexToInt(curKipVer);
            if (std::stoi(kipVersion) != i_curKipVer) {
                list->addItem(new tsl::elm::CustomDrawer([lineHeight, fontSize](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
                renderer->drawString("Kip version mismatch.\nUpdate the requirements to use this\npackage.", false, x, y + lineHeight, fontSize, a(tsl::style::color::ColorText));
                }), fontSize * 3 + lineHeight);
                rootFrame->setContent(list);
                return rootFrame;
            }
        }

        if (!enableConfigNav) {
            std::vector<std::string> subdirectories = getSubdirectories(subPath);
            std::sort(subdirectories.begin(), subdirectories.end());
            for (const auto& subDirectory : subdirectories) {
                if (isFileOrDirectory(subPath + subDirectory + '/' + configFileName)) {
                    auto item = new tsl::elm::ListItem(subDirectory);
                    item->setValue("\u25B6", tsl::PredefinedColors::White);
                    item->setClickListener([&, subDirectory](u64 keys)->bool {
                        if (keys & KEY_A) {
                            tsl::changeTo<SubMenu>(subPath + subDirectory + '/');
                            return true;
                        }
                        return false;
                        });
                    list->addItem(item);
                }
            }
        }

        // Add a section break with small text to indicate the "Commands" section
        // list->addItem(new tsl::elm::CategoryHeader("Commands"));

        // Load options from INI file in the subdirectory
        std::string subConfigIniPath = subPath + "/" + configFileName;
        std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> options = loadOptionsFromIni(subConfigIniPath);

        // Package Info
        PackageHeader packageHeader = getPackageHeaderFromIni(subConfigIniPath);
        
        // Populate the sub menu with options
        for (const auto& option : options) {
            std::string optionName = option.first;
            std::string footer; 
            bool usePattern = false;
            bool useSlider  = false;
            std::string headerName;
            if (optionName[0] == '@') { // a subdirectory. add a menu item and skip to the next command
                std::vector<std::string> tmpldir = option.second[0];
                auto item = new tsl::elm::ListItem(optionName.substr(1));
                item->setValue("\u25B6", tsl::PredefinedColors::White);
                item->setClickListener([&, tmpldir, item](u64 keys)->bool {
                    if (keys & KEY_A) {
                        if (!isFileOrDirectory(tmpldir[1])) {
                            item->setValue("FAIL", tsl::PredefinedColors::Red);
                            return true;
                        }
                        tsl::changeTo<KipInfoOverlay>(tmpldir, false);
                        return true;
                    }
                    return false;
                    });
                list->addItem(item);
                continue;
            }
            if (enableConfigNav && optionName[0] == '>') { // a subdirectory. add a menu item and skip to the next command
                auto subDirectory = optionName.substr(1);
                auto item = new tsl::elm::ListItem(subDirectory);
                item->setValue("\u25B6", tsl::PredefinedColors::White);
                item->setClickListener([&, subDirectory, item](u64 keys)->bool {
                    if (keys & KEY_A) {
                        if (!isDirectory(subPath + subDirectory + '/')) {
                            item->setValue("FAIL", tsl::PredefinedColors::Red);
                            return true;
                        }
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
            } else if (optionName[0] == '-') {
                useSlider = true;
                optionName = optionName.substr(1); // Strip the "-" character on the left
                footer = "\u25B6";
            } else {
                size_t pos = optionName.find(" - ");
                if (pos != std::string::npos) {
                    footer = optionName.substr(pos + 2); // Assign the part after "&&" as the footer
                    optionName = optionName.substr(0, pos); // Strip the "&&" and everything after it
                }
            }

            size_t pos = optionName.find(" ;; "); // Find the custom display header
            if (pos!= std::string::npos) {
                headerName = optionName.substr(pos + 4); // Strip the item name
                optionName = optionName.substr(0, pos); // Strip the displayName
            } else {
                headerName = optionName;
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
                } 
            }

            if (isSeparator) {
                auto item = new tsl::elm::CategoryHeader(optionName, true);
                list->addItem(item);
            } else if (usePattern || !useToggle || useSlider) {
                auto listItem = static_cast<tsl::elm::ListItem*>(nullptr);
                if ((footer == "\u25B6") || (footer.empty())) {
                    listItem = new tsl::elm::ListItem(optionName, footer);
                } else {
                    listItem = new tsl::elm::ListItem(optionName);
                    listItem->setValue(footer);
                }
                for (const auto& cmd : option.second) {
                    if (cmd[0] == "json_mark_cur_kip" && showCurInMenu) {
                        auto& offset = cmd[3];
                        std::string jsonPath = preprocessPath(cmd[1]);
                        listItem->setValue(findCurrent(jsonPath, offset));
                    }
                }
                
                //std::vector<std::vector<std::string>> modifiedCommands = getModifyCommands(option.second, pathReplace);
                listItem->setClickListener([command = option.second, keyName = headerName, subPath = this->subPath, usePattern, listItem, helpPath, useSlider](uint64_t keys) {
                    if (keys & KEY_A) {
                        if (listItem->getValue() == "APPLIED" && !prevValue.empty()) {
                            listItem->setValue(prevValue);
                            prevValue = "";
                            resetValue = false;
                        }
                        if (usePattern) {
                            tsl::changeTo<SelectionOverlay>(subPath, keyName, command);
                        } else if (useSlider) {
                            tsl::changeTo<FanSliderOverlay>(subPath, keyName, command);
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

    bool adjuct_top, adjuct_bot = false;

    virtual bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (this->adjuct_top) { // Adjust cursor to the top item after jump bot-top
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            this->adjuct_top = false;
        }
        if (this->adjuct_bot) {  // Adjust cursor to the top item after jump top-bot
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            this->adjuct_bot = false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DDOWN) || (keysDown & KEY_DOWN)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::UpMax);
                this->adjuct_top = true; // Go one item 
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            }
            return false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DUP) || (keysDown & KEY_UP)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::DownMax);
                this->adjuct_bot = true;
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            }
            return false;
        }
        if (keysDown & KEY_L) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpNum);
            return true;
        }
        if (keysDown & KEY_R) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownNum);
            return true;
        }if (keysDown & KEY_ZL) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpMax);
            return true;
        }
        if (keysDown & KEY_ZR) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownMax);
            return true;
        }
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
            if (this->getFocusedElement()->getClass()  == "ListItem" ){
                tsl::elm::ListItem* focusedItem = dynamic_cast<tsl::elm::ListItem*>(this->getFocusedElement());
                if (focusedItem->getClass() == "ListItem" && focusedItem->getValue() == "APPLIED") {
                    focusedItem->setValue(prevValue);
                    prevValue = "";
                    resetValue = false;
                }
            }
            return true;
        }
        return false;
    }
    virtual std::string getClass() {return "SubMenu";}
};

class MainMenu;

class Package : public SubMenu {
public:
    Package(const std::string& path) : SubMenu(path) {}

    tsl::elm::Element* createUI() override {
        package = getNameFromPath(subPath);
        std::string subConfigIniPath = subPath + "/" + configFileName;
        PackageHeader packageHeader = getPackageHeaderFromIni(subConfigIniPath);
        enableConfigNav = packageHeader.enableConfigNav;
        showCurInMenu = packageHeader.showCurInMenu;
        kipVersion = packageHeader.checkKipVersion;

        auto rootFrame = static_cast<tsl::elm::OverlayFrame*>(SubMenu::createUI());
        rootFrame->setTitle(package);
        rootFrame->setSubtitle("                             "); // FIXME: former subtitle is not fully erased if new string is shorter
        return rootFrame;
    }

    bool adjuct_top, adjuct_bot = false;

    bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (this->adjuct_top) { // Adjust cursor to the top item after jump bot-top
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            this->adjuct_top = false;
        }
        if (this->adjuct_bot) {  // Adjust cursor to the top item after jump top-bot
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            this->adjuct_bot = false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DDOWN) || (keysDown & KEY_DOWN)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::UpMax);
                this->adjuct_top = true; // Go one item 
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            }
            return false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DUP) || (keysDown & KEY_UP)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::DownMax);
                this->adjuct_bot = true;
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            }
            return false;
        }
        if (keysDown & KEY_L) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpNum);
            return true;
        }
        if (keysDown & KEY_R) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownNum);
            return true;
        }if (keysDown & KEY_ZL) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpMax);
            return true;
        }
        if (keysDown & KEY_ZR) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownMax);
            return true;
        }
        if ((keysDown & KEY_B)) {
            tsl::changeTo<MainMenu>();
            return true;
        }
        return false;
    }
    virtual std::string getClass() {return "Package";}

};

class Updater : public tsl::Gui {
protected:
    std::vector<std::map<std::string, std::string>> uitems;

public:
    Updater(const std::vector<std::map<std::string, std::string>>& items) : uitems(items) {}

    tsl::elm::Element* createUI() override {
        auto rootFrame = new tsl::elm::OverlayFrame("Updates available", "Updater");
        auto list = new tsl::elm::List();

        for (const auto& item : uitems) {
            auto* listItem = new tsl::elm::ListItem(item.at("name"), item.at("repoVer"));
            listItem->setClickListener([item, listItem](s64 key) {
                if (key & KEY_A) {
                    if (!DownloadProcessing) {
                        DownloadProcessing = true;
                        listItem->setText("Processing...");
                        return true;
                    }
                }
                if (DownloadProcessing) {
                    std::string type = item.at("type");
                    std::string link = item.at("link");
                    std::string name = item.at("name");

                    if (type == "ovl") {
                        if (downloadFile(link, "sdmc:/switch/.overlays/")) {
                            listItem->setText(name);
                            listItem->setValue("DONE", tsl::PredefinedColors::Green);
                            DownloadProcessing = false;
                            return true;
                        }
                    } else if (type == "pkgzip") {
                        std::string tempZipPath = "sdmc:/switch/.packages/temp.zip";
                        std::string destFolderPath = "sdmc:/switch/.packages/";

                        if (downloadFile(link, tempZipPath) && unzipFile(tempZipPath, destFolderPath)) {
                            deleteFileOrDirectory(tempZipPath);
                            listItem->setText(name);
                            listItem->setValue("DONE", tsl::PredefinedColors::Green);
                            DownloadProcessing = false;
                            return true;
                        }
                    } else if (type == "ovlzip") {
                        std::string tempZipPath = "sdmc:/switch/.packages/temp.zip";
                        std::string destFolderPath = "sdmc:/";

                        if (downloadFile(link, tempZipPath) && unzipFile(tempZipPath, destFolderPath)) {
                            deleteFileOrDirectory(tempZipPath);
                            listItem->setText(name);
                            listItem->setValue("DONE", tsl::PredefinedColors::Green);
                            DownloadProcessing = false;
                            return true;
                        }
                    }

                    listItem->setValue("FAIL", tsl::PredefinedColors::Red);
                    DownloadProcessing = false;
                }
                return false;
            });

            list->addItem(listItem);
        }

        rootFrame->setContent(list);
        return rootFrame;
    }

    bool adjuct_top, adjuct_bot = false;

    bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (this->adjuct_top) { // Adjust cursor to the top item after jump bot-top
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            this->adjuct_top = false;
        }
        if (this->adjuct_bot) {  // Adjust cursor to the top item after jump top-bot
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            this->adjuct_bot = false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DDOWN) || (keysDown & KEY_DOWN)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::UpMax);
                this->adjuct_top = true; // Go one item 
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            }
            return false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DUP) || (keysDown & KEY_UP)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::DownMax);
                this->adjuct_bot = true;
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            }
            return false;
        }
        if (keysDown & KEY_L) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpNum);
            return true;
        }
        if (keysDown & KEY_R) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownNum);
            return true;
        }if (keysDown & KEY_ZL) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpMax);
            return true;
        }
        if (keysDown & KEY_ZR) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownMax);
            return true;
        }
        if (keysDown & KEY_B) {
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
    std::string menuMode, defaultMenuMode, inOverlayString, fullPath, optionName, repoUrl;
    int priority;
    bool useDefaultMenu = false, showOverlayVersions = false, showPackageVersions = true;
    bool coolerMode = false;
    bool sorting = false;
    enum Screen {
        Default,
        Soverlays,
        Spackages
    };
    Screen ForMode;
public:
    MainMenu(const Screen& curScreen=Default) : ForMode(curScreen){}
    ~MainMenu() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("MainMenu");

        defaultMenuMode = "overlays";
        menuMode = "overlays";
        
        createDirectory(packageDirectory);
        createDirectory(settingsPath);
        
        
        bool settingsLoaded = false;
        if (isFileOrDirectory(settingsConfigIniPath)) {
            settingsData = getParsedDataFromIniFile(settingsConfigIniPath);
            if (settingsData.count("uberhand") > 0) {
                auto& uberhandSection = settingsData["uberhand"];
                if (uberhandSection.count("last_menu") > 0) {
                    menuMode = uberhandSection["last_menu"];
                    if (uberhandSection.count("default_menu") > 0) {
                        defaultMenuMode = uberhandSection["default_menu"];
                        if (uberhandSection.count("in_overlay") > 0) {
                            settingsLoaded = true;
                        }
                    }
                }
                if (uberhandSection["show_ovl_versions"] == "true") {
                    showOverlayVersions = true;
                } else {
                    setIniFileValue(settingsConfigIniPath, "uberhand", "show_ovl_versions", "false");
                }
                if (uberhandSection["show_pack_versions"] == "false") {
                    showPackageVersions = false;
                } else {
                    setIniFileValue(settingsConfigIniPath, "uberhand", "show_pack_versions", "true");
                }
                if (uberhandSection["coolerMode"] == "1"){
                    coolerMode = true;
                }
                if (!uberhandSection["ovlRepo"].empty()){
                    repoUrl = uberhandSection["ovlRepo"];
                } else {
                    repoUrl = "https://raw.githubusercontent.com/i3sey/uUpdater-ovl-repo/main/main.ini";
                }
            }
        }
        
        if (!settingsLoaded) { // write data if settings are not loaded
            setIniFileValue(settingsConfigIniPath, "uberhand", "default_menu", defaultMenuMode);
            setIniFileValue(settingsConfigIniPath, "uberhand", "last_menu", menuMode);
            setIniFileValue(settingsConfigIniPath, "uberhand", "in_overlay", "false");
        }
        copyTeslaKeyComboTouberhand();
        //setIniFileValue(settingsConfigIniPath, "uberhand", "in_overlay", "false");
        
        
        if ((defaultMenuMode == "overlays") || (defaultMenuMode == "packages")) {
            if (defaultMenuLoaded) {
                menuMode = defaultMenuMode.c_str();
                defaultMenuLoaded = false;
            }
        } else {
            defaultMenuMode = "last_menu";
            setIniFileValue(settingsConfigIniPath, "uberhand", "default_menu", defaultMenuMode);
        }
        
        std::string versionLabel = APP_VERSION+std::string("   (")+envGetLoaderInfo()+std::string(")");
        auto rootFrame = new tsl::elm::OverlayFrame("Uberhand", versionLabel, menuMode);
        auto list = new tsl::elm::List();

        //loadOverlayFiles(list);
        switch (ForMode) {
            case Soverlays:
                sorting = true;
                menuMode = "overlays";
                break;
            case Spackages:
                sorting = true;
                menuMode = "packages";
                break;
            case Default:
                break;
        }

        int count = 0;
        
        if (menuMode == "overlays") {
            // Load overlay files
            std::vector<std::string> overlayFiles = getFilesListByWildcard(overlayDirectory+"*.ovl");

            FILE* overlaysIniFile = fopen(overlaysIniFilePath.c_str(), "r");
            if (!overlaysIniFile) {
                fclose(fopen(overlaysIniFilePath.c_str(), "w")); // The INI file doesn't exist, so create an empty one.
            } else {
                fclose(overlaysIniFile); // The file exists, so close it.
            }
            //std::sort(overlayFiles.begin(), overlayFiles.end()); // Sort overlay files alphabetically

            std::string overlayFileName;

            if (!overlayFiles.empty()) {
                std::map<std::string, std::map<std::string, std::string>> overlaysIniData = getParsedDataFromIniFile(overlaysIniFilePath);
                std::multimap<int, std::string> order;
                std::map<std::string, std::string> alpSort;

                for (const std::string& sortElem : overlayFiles){
                    auto [result, overlayName, overlayVersion] = getOverlayInfo(sortElem);
                    if (result != ResultSuccess){
                        continue;
                    }
                    if (overlayName == "Uberhand"){
                        continue;
                    }
                    alpSort[overlayName] = sortElem;
                }
                overlayFiles.clear();
                for (const auto & overlay : alpSort) {
                    overlayFiles.push_back(overlay.second);
                }

                for (const auto& overlayFile : overlayFiles) {
                    overlayFileName = getNameFromPath(overlayFile);
                    int priority = 0;

                    if (getNameFromPath(overlayFile) == "ovlmenu.ovl"){
                        continue;
                    }

                    if (overlaysIniData.find(overlayFileName) == overlaysIniData.end()) {
                        setIniFileValue(overlaysIniFilePath, overlayFileName, "priority", "0");
                    } else {
                        // Check if the "priority" key exists in overlaysIniData for overlayFileName
                        if (overlaysIniData.find(overlayFileName) != overlaysIniData.end() &&
                            overlaysIniData[overlayFileName].find("priority") != overlaysIniData[overlayFileName].end()) {
                            priority = -(stoi(overlaysIniData[overlayFileName]["priority"]));
                        } else
                            setIniFileValue(overlaysIniFilePath, overlayFileName, "priority", "0");  
                    }
                    order.emplace(priority, overlayFileName);
                }

                
                
                for (const auto & overlay : order) {
                    overlayFileName = overlay.second;

                    auto [result, overlayName, overlayVersion] = getOverlayInfo(overlayDirectory + overlayFileName);
                    if (result != ResultSuccess)
                        continue;
                    
                    auto* listItem = new tsl::elm::ListItem(overlayName);
                    if (showOverlayVersions)
                        listItem->setValue(overlayVersion);

                    
                    if (sorting) {
                        std::map<std::string, std::map<std::string, std::string>> overlaysIniData = getParsedDataFromIniFile(overlaysIniFilePath);
                        priority = std::stoi(overlaysIniData[overlayFileName]["priority"]);
                        listItem->setValue("Priority: " + std::to_string(priority));
                    }
                    // Add a click listener to load the overlay when clicked upon
                    listItem->setClickListener([this, overlayFile=overlayDirectory + overlayFileName, listItem, overlayFileName](s64 key) {
                        int localPriority;
                        if (key & KEY_A) {
                            // Load the overlay here
                            setIniFileValue(settingsConfigIniPath, "uberhand", "in_overlay", "true"); // this is handled within tesla.hpp
                            tsl::setNextOverlay(overlayFile);
                            tsl::Overlay::get()->close();
                            return true;
                        } else if (key & KEY_PLUS) {
                            std::map<std::string, std::map<std::string, std::string>> overlaysIniData = getParsedDataFromIniFile(overlaysIniFilePath);
                            localPriority = std::stoi(overlaysIniData[overlayFileName]["priority"])+1;
                            setIniFileValue(overlaysIniFilePath, overlayFileName, "priority", std::to_string(localPriority));
                            tsl::changeTo<MainMenu>(Soverlays);
                            return true;
                        } else if (key & KEY_MINUS) {
                            std::map<std::string, std::map<std::string, std::string>> overlaysIniData = getParsedDataFromIniFile(overlaysIniFilePath);
                            localPriority = std::stoi(overlaysIniData[overlayFileName]["priority"])-1;
                            setIniFileValue(overlaysIniFilePath, overlayFileName, "priority", std::to_string(localPriority));
                            tsl::changeTo<MainMenu>(Soverlays);
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

            //ovl updater section
            list->addItem(new tsl::elm::CategoryHeader("Updater", true));
            auto updaterItem = new tsl::elm::ListItem("Check for Overlay Updates");
            updaterItem->setClickListener([this, updaterItem](uint64_t keys) {
                        if (keys & KEY_A) {
                            if (!DownloadProcessing) {
                                DownloadProcessing = true;
                                updaterItem->setText("Processing...");
                                return true;
                            }
                        }
                        if (DownloadProcessing) {
                            bool NeedUpdate = false;
                            std::vector<std::map<std::string, std::string>> items;
                            if (!coolerMode) {
                                std::vector<std::string> overlays = getFilesListByWildcard("sdmc:/switch/.overlays/*.ovl");
                                std::map<std::string, std::string> package;
                                downloadFile(repoUrl, "sdmc:/config/uberhand/Updater.ini");
                                std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> options = loadOptionsFromIni("sdmc:/config/uberhand/Updater.ini");
                                for (const auto& option : options) {
                                    for (const std::string& overlay : overlays) {
                                        std::string uoverlay = dropExtension(getNameFromPath(overlay));
                                        if (uoverlay == option.first) {
                                            auto [result, overlayName, overlayVersion] = getOverlayInfo(overlay);
                                            if (result != ResultSuccess)
                                                continue;
                                            package["name"] = overlayName;
                                            package["link"] = option.second.at(1).front().substr(5);
                                            package["localVer"] = overlayVersion;
                                            package["downloadEntry"] = option.second.front().front().substr(14);
                                            std::map<std::string, std::string> resultUpdate = ovlUpdateCheck(package);
                                            if (!resultUpdate.empty()) {
                                                NeedUpdate = true;
                                                items.insert(items.end(), resultUpdate);
                                            }
                                        }
                                    }
                                    
                                }
                            } else {
                                auto [result, overlayName, overlayVersion] = getOverlayInfo("sdmc:/switch/.overlays/ovlmenu.ovl");
                                if (result != ResultSuccess)
                                    return false;
                                std::map<std::string, std::string> ovlmenu;
                                ovlmenu["name"] = overlayName;
                                ovlmenu["localVer"] = overlayVersion;
                                ovlmenu["link"] = "https://api.github.com/repos/efosamark/Uberhand-Overlay/releases";
                                ovlmenu["downloadEntry"] = "1";
                                std::map<std::string, std::string> resultUpdate = ovlUpdateCheck(ovlmenu);
                                if (!resultUpdate.empty()) {
                                    NeedUpdate = true;
                                    items.insert(items.end(), resultUpdate);
                                }
                            }
                            
                            if (NeedUpdate){
                                DownloadProcessing = false;
                                tsl::changeTo<Updater>(items);
                            }
                            updaterItem->setText("No updates");
                            DownloadProcessing = false;
                            return true;
                        }
                        return false;
                    });
            list->addItem(updaterItem);
        }
        
        if (menuMode == "packages" ) {
            // Create the directory if it doesn't exist
            createDirectory(packageDirectory);

            // Load options from INI file
            std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> options = loadOptionsFromIni(packageConfigIniPath, true);
        

            // Load subdirectories
            std::vector<std::string> subdirectories = getSubdirectories(packageDirectory);
            
            FILE* packagesIniFile = fopen(packagesIniFilePath.c_str(), "r");
            if (!packagesIniFile) {
                fclose(fopen(packagesIniFilePath.c_str(), "w")); // The INI file doesn't exist, so create an empty one.
            } else {
                fclose(packagesIniFile); // The file exists, so close it.
            }
            std::multimap<int, std::string> order;
            std::map<std::string, std::map<std::string, std::string>> packagesIniData = getParsedDataFromIniFile(packagesIniFilePath);
            std::sort(subdirectories.begin(), subdirectories.end());
            for (const auto& taintedSubdirectory : subdirectories) {
                priority = 0;
                std::string subWithoutSpaces = taintedSubdirectory;
                std::remove(subWithoutSpaces.begin(), subWithoutSpaces.end(), ' ');
                if (packagesIniData.find(subWithoutSpaces) == packagesIniData.end()) {
                    setIniFileValue(packagesIniFilePath, subWithoutSpaces, "priority", "0");
                } else {
                    if (packagesIniData.find(subWithoutSpaces) != packagesIniData.end() &&
                        packagesIniData[subWithoutSpaces].find("priority") != packagesIniData[subWithoutSpaces].end()) {
                        priority = -(stoi(packagesIniData[subWithoutSpaces]["priority"]));
                    } else
                        setIniFileValue(packagesIniFilePath, subWithoutSpaces, "priority", "0");  
                }
                // logMessage("priority, taintedSubdirectory: "+std::to_string(priority)+subWithoutSpaces);
                order.emplace(priority, taintedSubdirectory);
            }

            count = 0;
            for (const auto& package : order) {

                //bool usingStar = false;
                std::string subdirectory = package.second;
                std::string subdirectoryIcon = "";
                

                std::string subPath = packageDirectory + subdirectory + "/";
                std::string configFilePath = subPath + "config.ini";
            
                if (isFileOrDirectory(configFilePath)) {
                    PackageHeader packageHeader = getPackageHeaderFromIni(subPath + configFileName);
                    if (count == 0) {
                        // Add a section break with small text to indicate the "Packages" section
                        list->addItem(new tsl::elm::CategoryHeader("Packages"));
                    }
                    
                    auto listItem = new tsl::elm::ListItem(subdirectoryIcon + subdirectory);
                    if (showPackageVersions)
                        listItem->setValue(packageHeader.version);

                    std::string subWithoutSpaces;
                    if (sorting) {
                        std::map<std::string, std::map<std::string, std::string>> packagesIniData = getParsedDataFromIniFile(packagesIniFilePath);
                        subWithoutSpaces = subdirectory;
                        std::remove(subWithoutSpaces.begin(), subWithoutSpaces.end(), ' ');
                        priority = std::stoi(packagesIniData[subWithoutSpaces]["priority"]);
                        listItem->setValue("Priority: " + std::to_string(priority));
                    }
                    listItem->setClickListener([this, listItem, subdirectory, subWithoutSpaces, subPath = packageDirectory + subdirectory + "/"](uint64_t keys) {
                        int localPriority;
                        std::string subWithoutSpaces;
                        if (keys & KEY_A) {
                            if (isFileOrDirectory(subPath + "/init.ini")) {
                                if (!DownloadProcessing) {
                                    DownloadProcessing = true;
                                    listItem->setText("Processing...");
                                    return true;
                                }
                            } else {
                                tsl::changeTo<Package>(subPath);
                            }
                            return true;
                        } else if (keys & KEY_PLUS) {
                            std::map<std::string, std::map<std::string, std::string>> packagesIniData = getParsedDataFromIniFile(packagesIniFilePath);
                            subWithoutSpaces = subdirectory;
                            std::remove(subWithoutSpaces.begin(), subWithoutSpaces.end(), ' ');
                            localPriority = std::stoi(packagesIniData[subWithoutSpaces]["priority"])+1;
                            setIniFileValue(packagesIniFilePath, subWithoutSpaces, "priority", std::to_string(localPriority));
                            tsl::changeTo<MainMenu>(Spackages);
                            return true;
                        } else if (keys & KEY_MINUS) {
                            std::map<std::string, std::map<std::string, std::string>> packagesIniData = getParsedDataFromIniFile(packagesIniFilePath);
                            subWithoutSpaces = subdirectory;
                            std::remove(subWithoutSpaces.begin(), subWithoutSpaces.end(), ' ');
                            localPriority = std::stoi(packagesIniData[subWithoutSpaces]["priority"])-1;
                            setIniFileValue(packagesIniFilePath, subWithoutSpaces, "priority", std::to_string(localPriority));
                            tsl::changeTo<MainMenu>(Spackages);
                            return true;
                        }
                        if (DownloadProcessing) {
                            std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>> options = loadOptionsFromIni(subPath + "/init.ini");
                                for (const auto& option : options) {
                                    if (interpretAndExecuteCommand(getModifyCommands(option.second, subPath + option.first)) == -1) {
                                        logMessage("Init failed!");
                                        DownloadProcessing = false;
                                        listItem->setValue("FAIL", tsl::PredefinedColors::Red);
                                    } else {
                                        deleteFileOrDirectory(subPath + "/init.ini");
                                        DownloadProcessing = false;
                                        tsl::changeTo<Package>(subPath);
                                    }
                                }
                        }
                        return false;
                    });

                    list->addItem(listItem);
                    count++;
                }

            }

            //package updater section
            list->addItem(new tsl::elm::CategoryHeader("Updater", true));
            auto updaterItem = new tsl::elm::ListItem("Check for Package Updates");
            updaterItem->setClickListener([this, subdirectories, updaterItem](uint64_t keys) {
                        if (keys & KEY_A) {
                            if (!DownloadProcessing) {
                                DownloadProcessing = true;
                                updaterItem->setText("Processing...");
                                return true;
                            }
                        }
                        if (DownloadProcessing) {
                            bool NeedUpdate = false;
                            std::vector<std::map<std::string, std::string>> items;
                            for (const auto& taintedSubdirectory : subdirectories) {
                                std::map<std::string, std::string> packageInfo = packageUpdateCheck(taintedSubdirectory + "/" + "config.ini");
                                if (packageInfo["localVer"] != packageInfo["repoVer"]){
                                    NeedUpdate = true;
                                    items.insert(items.end(), packageInfo);
                                    }
                            }
                            if (NeedUpdate){
                                DownloadProcessing = false;
                                tsl::changeTo<Updater>(items);
                            }
                            DownloadProcessing = false;
                            updaterItem->setText("No updates");
                            return true;
                        }
                        return false;
                    });
            list->addItem(updaterItem);
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
    
    bool adjuct_top, adjuct_bot = false;

    virtual bool handleInput(uint64_t keysDown, uint64_t keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (this->adjuct_top) { // Adjust cursor to the top item after jump bot-top
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            this->adjuct_top = false;
        }
        if (this->adjuct_bot) {  // Adjust cursor to the top item after jump top-bot
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            this->adjuct_bot = false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DDOWN) || (keysDown & KEY_DOWN)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::UpMax);
                this->adjuct_top = true; // Go one item 
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            }
            return false;
        }
        // Jump bot-top: scroll to the top after hitting the last list item
        if ((keysDown & KEY_DUP) || (keysDown & KEY_UP)) {
            auto prevItem = this->getFocusedElement();
            this->requestFocus(this->getTopElement(), tsl::FocusDirection::Up);
            if (prevItem == this->getFocusedElement()) {
                scrollListItems(this, ShiftFocusMode::DownMax);
                this->adjuct_bot = true;
            } else { // Adjust to account for tesla key processing
                this->requestFocus(this->getTopElement(), tsl::FocusDirection::Down);
            }
            return false;
        }
        if (keysDown & KEY_L) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpNum);
            return true;
        }
        if (keysDown & KEY_R) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownNum);
            return true;
        }if (keysDown & KEY_ZL) { // Scroll to up for 5 items
            scrollListItems(this, ShiftFocusMode::UpMax);
            return true;
        }
        if (keysDown & KEY_ZR) { // Scroll to down for 5 items
            scrollListItems(this, ShiftFocusMode::DownMax);
            return true;
        }
        if ((keysDown & KEY_DRIGHT) && !(keysHeld & ~KEY_DRIGHT)) {
            if (menuMode != "packages") {
                setIniFileValue(settingsConfigIniPath, "uberhand", "last_menu", "packages");
                tsl::changeTo<MainMenu>();
                return true;
            }
        }
        if ((keysDown & KEY_DLEFT) && !(keysHeld & ~KEY_DLEFT)) {
            if (menuMode != "overlays") {
                setIniFileValue(settingsConfigIniPath, "uberhand", "last_menu", "overlays");
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
    virtual std::string getClass() {return "MainMenu";}
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
