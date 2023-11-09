#include <tesla.hpp>
#include <utils.hpp>

class KipInfoOverlay : public tsl::Gui {
private:
    std::vector<std::string> kipInfoCommand;
    bool showBackup, hasPages, isFirstPage = false;

public:
    KipInfoOverlay(std::vector<std::string> kipInfoCommand) : kipInfoCommand(kipInfoCommand), showBackup(true) {}
    KipInfoOverlay(std::vector<std::string> kipInfoCommand, bool showBackup, bool isFirstPage = true) : kipInfoCommand(kipInfoCommand), showBackup(showBackup), isFirstPage(isFirstPage) {}
    ~KipInfoOverlay() {}

    virtual tsl::elm::Element* createUI() override {
        // logMessage ("KipInfoOverlay");

        std::pair<std::string, int> textDataPair;
        constexpr int lineHeight = 20;  // Adjust the line height as needed
        constexpr int fontSize = 19;    // Adjust the font size as needed
        std::string footer;

        if (showBackup) {
            if (kipInfoCommand.size() > 3){
                hasPages = true;
                if (isFirstPage)
                    footer = "\uE0E0  Apply     \uE0E2  Delete     \uE0EE  Page 2";
                else
                    footer = "\uE0E0  Apply     \uE0E2  Delete     \uE0ED  Page 1";
            } else {
                footer = "\uE0E0  Apply     \uE0E2  Delete";
            }
        } else {
            if (kipInfoCommand.size() > 2) {
                hasPages = true;
                if (isFirstPage)
                    footer = "\uE0E1  Back     \uE0EE  Page 2";
                else 
                    footer = "\uE0E1  Back     \uE0ED  Page 1";
            } else {
                footer = "\uE0E1  Back";
            }
        }
        
        auto rootFrame = new tsl::elm::OverlayFrame("Kip Management", "Uberhand Package", "", false, footer);
        auto list = new tsl::elm::List();

        if (!showBackup) {
            textDataPair = dispCustData(kipInfoCommand[isFirstPage ? 1 : 2]);
            showBackup = false;
        }
        else {
            showBackup = true;
            textDataPair = dispCustData(kipInfoCommand[isFirstPage ? 2 : 3], kipInfoCommand[1]);
        }

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
        if (!isFirstPage && (keysDown & KEY_B)) {
            tsl::goBack();
            tsl::goBack();
            return true;
        }
        if (keysDown & KEY_B) {
            tsl::goBack();
            return true;
        }
        if (showBackup && (keysDown & KEY_A)) {
           copyFileOrDirectory(this->kipInfoCommand[1], "/atmosphere/kips/loader.kip");
           applied = true;
           tsl::goBack();
           return true;
        }
        if (showBackup && (keysDown & KEY_X)) {
           deleteFileOrDirectory(this->kipInfoCommand[1]);
           tsl::goBack();
           applied = false;
           deleted = true;
           return true;
        }
        if (hasPages && isFirstPage && (keysDown & KEY_DRIGHT)) {
           tsl::changeTo<KipInfoOverlay>(kipInfoCommand, false, false);
        }
        if (hasPages && !isFirstPage && (keysDown & KEY_DLEFT)) {
           tsl::goBack();
        }
        return false;
    }
};