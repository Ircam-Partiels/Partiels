#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class MainMenuModel
    : public juce::MenuBarModel
    {
    public:
        MainMenuModel(juce::DocumentWindow& window);
        ~MainMenuModel() override;

        // juce::MenuBarModel
        juce::StringArray getMenuBarNames() override;
        juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, juce::String const& menuName) override;
        void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

#ifdef JUCE_MAC
        void updateAppleMenuItems();
#endif

    private:
        void addGlobalSettingsMenu(juce::PopupMenu& menu);
#ifndef JUCE_MAC
        juce::DocumentWindow& mWindow;
#endif
    };
} // namespace Application

ANALYSE_FILE_END
