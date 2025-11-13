#pragma once

#include "../Document/AnlDocumentModel.h"
#include "../Misc/AnlMisc.h"
#include <future>

ANALYSE_FILE_BEGIN

namespace Application
{
    class CoAnalyzerContent
    : public juce::Component
    , private juce::AsyncUpdater
    {
    public:
        CoAnalyzerContent();
        ~CoAnalyzerContent() override;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void resized() override;
        void parentHierarchyChanged() override;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        void sendRequest();
        void showApiKeyDialog();
        std::tuple<juce::Result, std::unique_ptr<juce::XmlElement>> sendRequestToMistral(juce::String const& apiKey, juce::String const& xmlData, juce::String const& userPrompt);

        juce::TextEditor mQueryEditor;
        Icon mSendButton;
        juce::TextButton mApiKeyButton;
        juce::Label mStatusLabel;

        juce::String mApiKey;
        std::future<std::tuple<juce::Result, std::unique_ptr<juce::XmlElement>>> mRequestFuture;
        bool mRequestPending{false};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CoAnalyzerContent)
    };

    class CoAnalyzerPanel
    : public juce::Component
    {
    public:
        CoAnalyzerPanel();
        ~CoAnalyzerPanel() override;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void resized() override;
        void colourChanged() override;
        void parentHierarchyChanged() override;

    private:
        juce::Label mTitleLabel;
        ColouredPanel mTopSeparator;
        CoAnalyzerContent mContent;
    };
} // namespace Application

ANALYSE_FILE_END
