#pragma once

#include "../Document/AnlDocumentModel.h"

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
        using FloatingWindowContainer::show;
        void show(juce::Point<int> const& pt) override;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        // clang-format off
        enum class Format
        {
              JPEG
            , PNG
            , CSV
            , JSON
        };
        // clang-format on

        juce::String getSelectedIdentifier() const;
        void updateItems();
        void updateFormat();
        void sanitizeImageProperties();
        void exportToFile();

        static std::pair<int, int> getSizeFor(juce::String const& identifier);
        static juce::Result exportToImage(juce::File const file, Format format, juce::String const& identifier, bool autoSize, int width, int height, bool groupMode);
        static juce::Result exportToText(juce::File const file, Format format, juce::String const& identifier, bool ignoreGrids);

        Document::Accessor::Listener mDocumentListener;
        PropertyList mPropertyItem;
        PropertyList mPropertyFormat;
        PropertyToggle mPropertyGroupMode;
        PropertyToggle mPropertyAutoSizeMode;
        PropertyNumber mPropertyWidth;
        PropertyNumber mPropertyHeight;
        PropertyToggle mPropertyIgnoreGrids;
        PropertyTextButton mPropertyExport;
        LoadingCircle mLoadingCircle;
        std::future<std::tuple<AlertWindow::MessageType, juce::String, juce::String>> mProcess;

        auto static constexpr documentItemFactor = 1000000;
        auto static constexpr groupItemFactor = documentItemFactor / 1000;
    };
} // namespace Application

ANALYSE_FILE_END
