#pragma once

#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class KeyMappingsContent
    : public juce::Component
    {
    public:
        KeyMappingsContent();
        ~KeyMappingsContent() override = default;

        // juce::Component
        void resized() override;

    private:
        class Container
        : public juce::Component
        , private juce::ChangeListener
        {
        public:
            Container(juce::KeyPressMappingSet& keyPressMappingSet);
            ~Container() override;

            // juce::Component
            void resized() override;

        private:
            struct Section
            {
            public:
                Section(juce::ApplicationCommandManager const& commandManager, juce::String const& category);
                ~Section() = default;

                class Content
                : public juce::Component
                {
                public:
                    Content(juce::ApplicationCommandManager const& commandManager, juce::String const& category);
                    ~Content() override = default;

                    void resized() override;

                private:
                    std::vector<std::unique_ptr<PropertyLabel>> mProperties;
                };

                Content content;
                ConcertinaTable table;
            };

            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;

            juce::KeyPressMappingSet& mKeyPressMappingSet;
            ComponentListener mComponentListener;
            std::vector<std::unique_ptr<Section>> mSections;
        };

        Container mContainer;
        juce::Viewport mViewport;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyMappingsContent)
    };

    class KeyMappingsPanel
    : public HideablePanelTyped<KeyMappingsContent>
    {
    public:
        KeyMappingsPanel();
        ~KeyMappingsPanel() override = default;

#ifdef JUCE_DEBUG
        void mouseDown(juce::MouseEvent const& event) override;
#endif
    };
} // namespace Application

ANALYSE_FILE_END
