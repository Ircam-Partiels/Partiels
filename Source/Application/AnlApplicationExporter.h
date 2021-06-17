#pragma once

#include "../Document/AnlDocumentTools.h"
#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Exporter
    : public FloatingWindowContainer
    , private juce::AsyncUpdater
    {
    public:
        Exporter();
        ~Exporter() override;

        // juce::Component
        void resized() override;

        // FloatingWindowContainer
        void showAt(juce::Point<int> const& pt) override;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        juce::String getSelectedIdentifier() const;
        void updateItems();
        void sanitizeProperties(bool updateModel);
        void exportToFile();

        static std::pair<int, int> getSizeFor(juce::String const& identifier);

        Accessor::Listener mListener;
        PropertyList mPropertyItem;
        PropertyList mPropertyFormat;
        PropertyToggle mPropertyGroupMode;
        PropertyToggle mPropertyAutoSizeMode;
        PropertyNumber mPropertyWidth;
        PropertyNumber mPropertyHeight;
        PropertyToggle mPropertyRawHeader;
        PropertyList mPropertyRawSeparator;
        PropertyToggle mPropertyIgnoreGrids;
        PropertyTextButton mPropertyExport;
        LoadingCircle mLoadingCircle;
        std::atomic<bool> mShoulAbort{false};
        std::future<std::tuple<AlertWindow::MessageType, juce::String, juce::String>> mProcess;
        Document::LayoutNotifier mDocumentLayoutNotifier;

        auto static constexpr documentItemFactor = 1000000;
        auto static constexpr groupItemFactor = documentItemFactor / 1000;
    };
} // namespace Application

ANALYSE_FILE_END
