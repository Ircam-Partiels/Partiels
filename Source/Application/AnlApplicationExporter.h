#pragma once

#include "../Document/AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Exporter
    : public FloatingWindowContainer
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
        // clang-format off
        enum class Format
        {
              JPEG
            , PNG
            , CSV
            , XML
            , JSON
        };
        // clang-format on

        juce::String getSelectedIdentifier() const;
        void updateItems();
        void updateFormat();
        void sanitizeImageProperties();
        void exportToFile();

        static std::pair<int, int> getSizeFor(juce::String const& identifier);
        static void exportToImage(Format format, juce::String const& identifier, bool autoSize, int width, int height, bool groupMode);
        static void exportToText(Format format, juce::String const& identifier, bool ignoreGrids);

        Document::Accessor::Listener mDocumentListener;
        PropertyList mPropertyItem;
        PropertyList mPropertyFormat;
        PropertyToggle mPropertyGroupMode;
        PropertyToggle mPropertyAutoSizeMode;
        PropertyNumber mPropertyWidth;
        PropertyNumber mPropertyHeight;
        PropertyToggle mPropertyIgnoreGrids;
        PropertyTextButton mPropertyExport;

        auto static constexpr documentItemFactor = 1000000;
        auto static constexpr groupItemFactor = documentItemFactor / 1000;
    };
} // namespace Application

ANALYSE_FILE_END
