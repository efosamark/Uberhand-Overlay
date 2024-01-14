#include <tesla.hpp>
#include <utils.hpp>
class HelpOverlay : public tsl::Gui {
private:
    std::string helpPath;
    bool isInSection, inQuotes;

public:
    HelpOverlay(const std::string& helpPath) : helpPath(helpPath) {}
    ~HelpOverlay() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("HelpOverlay");

        std::pair<std::string, int> textDataPair;
        constexpr int lineHeight = 20;  // Adjust the line height as needed
        constexpr int fontSize = 19;    // Adjust the font size as needed

        auto rootFrame = new tsl::elm::OverlayFrame("Help", "Uberhand Package","",false,"\uE0E1  Back     ");
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