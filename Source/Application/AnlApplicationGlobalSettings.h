#pragma once

#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    //! @brief The content of the global settings panel
    //! @details Displays and edits the global settings of the application (theme, file management,
    //! quick export, edition, updates, default template and translation).
    class GlobalSettingsContent
    : public juce::Component
    {
    public:
        GlobalSettingsContent();
        ~GlobalSettingsContent() override;

        // juce::Component
        void resized() override;
        void visibilityChanged() override;

    private:
        void updateDefaultTemplate();
        void updateQuickExportDirectory();
        void updateTranslation();

        Accessor::Listener mListener{typeid(*this).name()};
        PropertyList mPropertyTheme;
        PropertyToggle mPropertySilentFileManagement;
        PropertyToggle mPropertyIgnoreTimeSelection;
        PropertyToggle mPropertyPreserveFullDuration;
        PropertyToggle mPropertyAutoUpdate;
        PropertyToggle mPropertyIncludePreReleases;
        PropertyList mPropertyDefaultTemplate;
        PropertyList mPropertyQuickExportDirectory;
        PropertyList mPropertyTranslation;
        std::vector<juce::File> mTranslationFiles;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalSettingsContent)
    };

    //! @brief The panel that contains the global settings
    class GlobalSettingsPanel
    : public HideablePanelTyped<GlobalSettingsContent>
    {
    public:
        GlobalSettingsPanel();
        ~GlobalSettingsPanel() override = default;
    };
} // namespace Application

ANALYSE_FILE_END
