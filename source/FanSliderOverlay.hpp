#define NDEBUG
#define STBTT_STATIC
#define TESLA_INIT_IMPL
#include <tesla.hpp>
#include <HelpOverlay.hpp>
#include <utils.hpp>

class FanSliderOverlay : public tsl::Gui {
private:
    std::string filePath, specificKey ;
    std::vector<std::vector<std::string>> commands;
    std::string helpPath = "";


public:
    FanSliderOverlay(const std::string& file, const std::string& key = "", const std::vector<std::vector<std::string>>& cmds = {}) 
        : filePath(file), specificKey(key), commands(cmds) {}
    ~FanSliderOverlay() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("FanSliderOverlay");

        size_t fifthSlashPos = filePath.find('/', filePath.find('/', filePath.find('/', filePath.find('/') + 1) + 1) + 1);
        bool hasHelp = false;
        std::string menuName = "";
        if (fifthSlashPos != std::string::npos) {
            // Extract the substring up to the fourth slash
            helpPath = filePath.substr(0, fifthSlashPos);
            if (!specificKey.empty()) {
                menuName = specificKey;
                removeLastNumericWord(menuName);
                helpPath += "/Help/" + getNameWithoutPrefix(getNameFromPath(filePath)) + "/" + menuName + ".txt";
            } else {
                helpPath += "/Help/" + getNameWithoutPrefix(getNameFromPath(filePath)) + ".txt";
            }
            if (isFileOrDirectory(helpPath)) {
               hasHelp = true; 
            } else {
                helpPath = "";
            }

        }
        auto rootFrame = new tsl::elm::OverlayFrame(specificKey.empty() ? getNameWithoutPrefix(getNameFromPath(filePath)) : specificKey,
                                                    "Uberhand Package", "", hasHelp, "\uE0E1  Back     \uE0E0  Apply     ");
        auto list = new tsl::elm::List();

        std::string sourceIni = "";
        std::string sectionIni = "";
        std::string keyIni = "";
        enum fanModes {Both, Handheld, Console};
        fanModes fanMode = fanModes::Both;
        
        for (const auto& cmd : commands) {
            if (cmd.size() > 1) {
                if (cmd[0] == "slider_ini") {
                    sourceIni  = preprocessPath(cmd[1]);
                    if (cmd[2] == "handheld") {
                        fanMode = fanModes::Handheld;
                    } else if (cmd[2] == "console") {
                        fanMode = fanModes::Console;
                    } else {
                        fanMode = fanModes::Both;
                    }
                }
            } 
        }

        if (!isFileOrDirectory(sourceIni)) {
            setIniFileValue(sourceIni, "tc", "use_configurations_on_fwdbg", "u8!0x1");
            setIniFileValue(sourceIni, "tc", "tskin_rate_table_console_on_fwdbg", "\"str!\"[[-1000000, 16000, -255, -255], [16000, 36000, -255, 0], [36000, 41000, 0, 51], [41000, 47000, 51, 64], [47000, 58000, 64, 153], [58000, 1000000, 255, 255]]\"\"");
            setIniFileValue(sourceIni, "tc", "tskin_rate_table_handheld_on_fwdbg", "\"str!\"[[-1000000, 16000, -255, -255], [16000, 36000, -255, 0], [36000, 41000, 0, 51], [41000, 47000, 51, 64], [47000, 58000, 64, 153], [58000, 1000000, 255, 255]]\"\"");
        } else {
            if (readIniValue(sourceIni, "tc", "use_configurations_on_fwdbg") != "u8!0x1") {
                setIniFileValue(sourceIni, "tc", "use_configurations_on_fwdbg", "u8!0x1");
            }
            if (readIniValue(sourceIni, "tc", "tskin_rate_table_console_on_fwdbg").empty()) {
                setIniFileValue(sourceIni, "tc", "tskin_rate_table_console_on_fwdbg", "\"str!\"[[-1000000, 16000, -255, -255], [16000, 36000, -255, 0], [36000, 41000, 0, 51], [41000, 47000, 51, 64], [47000, 58000, 64, 153], [58000, 1000000, 255, 255]]\"\"");
            }
            if (readIniValue(sourceIni, "tc", "tskin_rate_table_handheld_on_fwdbg").empty()) {
                setIniFileValue(sourceIni, "tc", "tskin_rate_table_handheld_on_fwdbg", "\"str!\"[[-1000000, 16000, -255, -255], [16000, 36000, -255, 0], [36000, 41000, 0, 51], [41000, 47000, 51, 64], [47000, 58000, 64, 153], [58000, 1000000, 255, 255]]\"\"");
            }
        }
        std::string iniString = readIniValue(sourceIni, "tc", (fanMode == fanModes::Console or fanMode == fanModes::Both) ? "tskin_rate_table_console_on_fwdbg" : "tskin_rate_table_handheld_on_fwdbg");
        std::vector<std::vector<int>> iniValues = parseIntIniData(iniString);
        for (const auto& arr : iniValues) {
            std::string low = arr[0] < 0 ? "0" : std::to_string(arr[0]/1000) + "°C";
            std::string high = arr[1] > 100000 ? "100°C" : std::to_string((arr[1]/1000) - 1) + "°C";
            std::string header = "Max fan speed at " + low + "-" + high + ": ";
            double stepSize = 0.05 * 255;
            int percentage = 0;
            if (arr[3] > 0) {
                percentage = static_cast<int>(ceil(arr[3] / stepSize));
            }

            auto slider = new tsl::elm::NamedStepTrackBar(" ",{header + "0%", header + "5%", header + "10%", header + "15%", header + "20%", header + "25%", header + "30%", header + "35%", header + "40%", header + "45%", header + "50%", header + "55%", header + "60%", header + "65%", header + "70%", header + "75%", header + "80%", header + "85%", header + "90%", header + "95%", header+"100%"});
            
            slider->setProgress(percentage);
            slider->setValueChangedListener([this, list, slider](u8 val) {
                size_t listSize = list->getSize();
                size_t sliderIndex = list->getIndexInList(slider);
                    if (sliderIndex != 0) {
                        for (size_t i = 0; i < sliderIndex; i++) {
                            if (list->getItemAtIndex(i)->getClass()  == "TrackBar") {
                                tsl::elm::StepTrackBar* prevSlider = dynamic_cast<tsl::elm::StepTrackBar*>(list->getItemAtIndex(i));
                                if (prevSlider->getProgress() > val)
                                {
                                    prevSlider->setProgress(val);
                                }
                            }
                        }
                    }
                    for (size_t i = sliderIndex; i < listSize; i++) {
                        if (list->getItemAtIndex(i)->getClass()  == "TrackBar") {
                            tsl::elm::StepTrackBar* curSlider = dynamic_cast<tsl::elm::StepTrackBar*>(list->getItemAtIndex(i));
                            if (curSlider->getProgress() < val)
                                curSlider->setProgress(val);
                        }
                    }
            });
            slider->setClickListener([this, list, iniString, sourceIni, iniValues, fanMode](uint64_t keys) { // Add 'command' to the capture list
                if (keys & KEY_A) {
                    std::vector<int> values;
                    size_t listSize = list->getSize();
                    for (size_t i = 0; i < listSize; i++) {
                        if (list->getItemAtIndex(i)->getClass()  == "TrackBar") {
                            values.push_back(int(double(dynamic_cast<tsl::elm::StepTrackBar*>(list->getItemAtIndex(i))->getProgress())*12.75));
                        }
                    }
                    if (fanMode == fanModes::Both) {
                        setIniFileValue(sourceIni, "tc", "tskin_rate_table_console_on_fwdbg", formString(values, parseIntIniData(iniString, false)));
                        setIniFileValue(sourceIni, "tc", "tskin_rate_table_handheld_on_fwdbg", formString(values, parseIntIniData(iniString, false)));
                    } else {
                        setIniFileValue(sourceIni, "tc", (fanMode == fanModes::Handheld) ? "tskin_rate_table_handheld_on_fwdbg" : "tskin_rate_table_console_on_fwdbg", formString(values, parseIntIniData(iniString, false)));
                    }
                    
                    applied = true;
                    tsl::goBack();
                    return true;
                }
                return false;
            });
            list->addItem(slider);
        }
        rootFrame->setContent(list);
    return rootFrame;
    }

    std::string formString(std::vector<int> newValues, std::vector<std::vector<int>> initialData) {

        std::stringstream result;
        result << "\"str!\"[[";

        newValues.insert(newValues.begin(), 0);

        size_t newValueIndex = 0;
        int newThird = -1;

        for (size_t i = 0; i < initialData.size(); i++) {
            std::vector<int> arr = initialData[i];
            if (newThird >= 0)
                arr[2] = newThird;
            if (newValues[newValueIndex] < arr[2])
                arr[3] = arr[2];
            else
                arr[3] = newValues[newValueIndex];
            newThird = arr[3];
            newValueIndex++;
            for (int j = 0; j < 3; j++) {
                result << arr[j] << ", ";
            }
            if (i != initialData.size() -1)
                result << arr[3]  <<  "], [";
            else
                result << arr[3] << "]]\"\"";

        }
        return result.str();
    }

    virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        } else if (keysDown & KEY_Y && !helpPath.empty()) {
            tsl::changeTo<HelpOverlay>(helpPath);
        }
        return false;
    }
};
