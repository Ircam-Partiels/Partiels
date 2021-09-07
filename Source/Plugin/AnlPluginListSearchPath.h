#pragma once

#include "AnlPluginListModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    class SearchPath
    : public FloatingWindowContainer
    , public juce::DragAndDropContainer
    {
    public:
        SearchPath(Accessor& accessor);
        ~SearchPath() override;

        // juce::Component
        void resized() override;

    private:
        Accessor& mAccessor;
        Accessor::Listener mListener{typeid(*this).name()};
        FileSearchPathTable mFileSearchPathTable;
        ColouredPanel mSeparator;
        juce::TextButton mApplyButton{juce::translate("Apply"), juce::translate("Apply the new plugin settings")};
        juce::TextButton mResetButton{juce::translate("Reset"), juce::translate("Reset to the current plugin settings")};
        juce::TextButton mDefaultButton{juce::translate("Default"), juce::translate("Reset to the default plugin settings")};
        juce::Label mEnvVariableInfo;
        juce::ToggleButton mEnvVariableButton;
        juce::TooltipWindow mTooltipWindow{this};

        JUCE_LEAK_DETECTOR(SearchPath)
    };
} // namespace PluginList

ANALYSE_FILE_END
