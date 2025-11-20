#pragma once

#include "AnlApplicationLlamaChat.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace CoAnalyzer
    {
        // clang-format off
        enum class AttrType : size_t
        {
              model
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::model, juce::File, Model::Flag::basic>
        >;
        // clang-format on

        class Accessor
        : public Model::Accessor<Accessor, AttrContainer>
        {
        public:
            using Model::Accessor<Accessor, AttrContainer>::Accessor;
            // clang-format off
            Accessor()
            : Accessor(AttrContainer({
                                          {}
                                    }))
            {
            }
            // clang-format on
        };

        class SettingsContent
        : public juce::Component
        {
        public:
            SettingsContent();
            ~SettingsContent() override;

            // juce::Component
            void resized() override;

        private:
            Accessor::Listener mListener{typeid(*this).name()};
            PropertyList mModel;
            std::vector<juce::File> mInstalledModels;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsContent)
        };

        class SettingsPanel
        : public HideablePanel
        {
        public:
            SettingsPanel();
            ~SettingsPanel() override;

        private:
            SettingsContent mContent;
        };

        class Chat
        : public juce::Component
        , private juce::AsyncUpdater
        {
        public:
            Chat(Accessor& accessor);
            ~Chat() override;

            // juce::Component
            void paint(juce::Graphics& g) override;
            void resized() override;
            void colourChanged() override;
            void parentHierarchyChanged() override;

        private:
            using Role = Llama::Chat::Role;

            // juce::AsyncUpdater
            void handleAsyncUpdate() override;

            bool canSendQuery() const;
            void initializeSystem();
            void sendUserQuery();
            void updateHistory();

            Accessor& mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            std::atomic<bool> mIsInitialized{false};
            std::atomic<bool> mShouldQuit{false};
            Llama::Chat mChat{mShouldQuit};
            juce::TextEditor mHistoryEditor;
            ColouredPanel mSeparator1;
            juce::TextEditor mQueryEditor;
            Icon mSendButton;
            ColouredPanel mSeparator2;
            juce::Label mStatusLabel;
            std::vector<std::tuple<Role, juce::String>> mHistory;
            std::future<std::tuple<juce::Result, juce::String, juce::String>> mRequestFuture;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Chat)
        };
    } // namespace CoAnalyzer
} // namespace Application

ANALYSE_FILE_END
