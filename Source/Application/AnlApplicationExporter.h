#pragma once

#include "../Document/AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class ImageExporter
    : public FloatingWindowContainer
    {
    public:
        ImageExporter();
        ~ImageExporter() override;

        // juce::Component
        void resized() override;

        // FloatingWindowContainer
        using FloatingWindowContainer::show;
        void show(juce::Point<int> const& pt) override;

    private:
        static std::pair<int, int> getSizeFor(juce::String const& identifier);
        juce::String getSelectedIdentifier() const;
        void updateItems();
        void sanitizeProperties();
        void exportToFile();

        Document::Accessor::Listener mDocumentListener;
        PropertyList mPropertyItem;
        PropertyToggle mPropertyGroupMode;
        PropertyToggle mPropertyAutoSizeMode;
        PropertyNumber mPropertyWidth;
        PropertyNumber mPropertyHeight;
        PropertyList mPropertyFormat;
        PropertyTextButton mPropertyExport;

        auto static constexpr documentItemFactor = 1000000;
        auto static constexpr groupItemFactor = documentItemFactor / 1000;
    };
} // namespace Application

ANALYSE_FILE_END
