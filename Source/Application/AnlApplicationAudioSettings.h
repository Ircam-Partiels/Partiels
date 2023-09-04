#pragma once

#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class AudioSettingsContent
    : public juce::Component
    , private juce::ChangeListener
    {
    public:
        AudioSettingsContent();
        ~AudioSettingsContent() override;

        // juce::Component
        void resized() override;

    private:
        // juce::ChangeListener
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;

        class PropertyChannelRouting
        : public PropertyComponent<BooleanMatrixSelector>
        {
        public:
            PropertyChannelRouting(std::function<void(size_t row, size_t column)> fn);
            ~PropertyChannelRouting() override = default;

            // juce::Component
            void resized() override;
        };

        Accessor::Listener mListener{typeid(*this).name()};
        Document::Accessor::Listener mDocumentListener{typeid(*this).name()};
        PropertyList mPropertyDriver;
        PropertyList mPropertyOutputDevice;
        PropertyList mPropertySampleRate;
        PropertyList mPropertyBufferSize;
        PropertyNumber mPropertyBufferSizeNumber;
        PropertyChannelRouting mPropertyChannelRouting;
        PropertyTextButton mPropertyDriverPanel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsContent)
    };

    class AudioSettingsPanel
    : public HideablePanelTyped<AudioSettingsContent>
    {
    public:
        AudioSettingsPanel();
        ~AudioSettingsPanel() override = default;
    };
} // namespace Application

ANALYSE_FILE_END
