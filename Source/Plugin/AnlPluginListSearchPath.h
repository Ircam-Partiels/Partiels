#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class SearchPath
    : public juce::Component
    , public juce::DragAndDropContainer
    {
    public:
        SearchPath(Accessor& accessor);
        ~SearchPath() override;

        // juce::Component
        void resized() override;

        //! @brief Asks the user to apply or discard the pending changes before closing.
        //! @details If there are no pending changes, the callback is called immediately with true.
        //! Otherwise, the user can apply or discard the changes (true) or cancel the closing (false).
        //! @param callback The function called with true if the content can be closed or false otherwise.
        void askToClose(std::function<void(bool)> callback);

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        FileSearchPathTable mFileSearchPathTable;
        ColouredPanel mSeparator;
        juce::TextButton mApplyButton{juce::translate("Apply"), juce::translate("Apply the new plugin settings")};
        juce::TextButton mResetButton{juce::translate("Reset"), juce::translate("Reset to the current plugin settings")};
        juce::TextButton mDefaultButton{juce::translate("Default"), juce::translate("Reset to the default plugin settings")};
        juce::TextButton mRescanButton{juce::translate("Rescan"), juce::translate("Rescan the list of installed plugins")};
        juce::TextButton mOptionButton{juce::translate("Options"), juce::translate("Change the plugin options")};
        bool mUseEnvVariable = true;
        bool mIncludePreReleases = false;
#if JUCE_MAC
        QuarantineMode mQuarantineMode = QuarantineMode::force;
#endif
        JUCE_LEAK_DETECTOR(SearchPath)
    };
} // namespace PluginList

ANALYSE_FILE_END
