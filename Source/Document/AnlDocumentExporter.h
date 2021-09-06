#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Exporter
    {
        using GetSizeFn = std::function<std::pair<int, int>(juce::String const& identifier)>;

        struct Options
        {
            // clang-format off
            enum class Format
            {
                  jpeg
                , png
                , csv
                , json
            };
            
            enum class ColumnSeparator
            {
                  comma
                , space
                , tab
                , pipe
                , slash
                , colon
            };
            // clang-format on

            Format format{Format::jpeg};
            bool useGroupOverview{false};
            bool useAutoSize{false};
            int imageWidth{1920};
            int imageHeight{1200};
            bool includeHeaderRaw{true};
            bool ignoreGridResults{true};
            ColumnSeparator columnSeparator{ColumnSeparator::comma};
            bool includeDescription{true};

            bool operator==(Options const& rhd) const noexcept;
            bool operator!=(Options const& rhd) const noexcept;

            bool useImageFormat() const noexcept;
            bool useTextFormat() const noexcept;

            juce::String getFormatName() const;
            juce::String getFormatExtension() const;
            juce::String getFormatWilcard() const;

            char getSeparatorChar() const;
        };

        class Panel
        : public juce::Component
        , private juce::AsyncUpdater
        {
        public:
            Panel(Accessor& accessor, GetSizeFn getSizeFor);
            ~Panel() override;

            // juce::Component
            void resized() override;

            void setOptions(Options const& options, juce::NotificationType notification);
            Options const& getOptions() const;
            juce::String getSelectedIdentifier() const;

            std::function<void(void)> onOptionsChanged = nullptr;

        private:
            void updateItems();
            void sanitizeProperties(bool updateModel);

            // juce::AsyncUpdater
            void handleAsyncUpdate() override;

            Accessor& mAccessor;
            GetSizeFn mGetSizeForFn = nullptr;
            Options mOptions;

            PropertyList mPropertyItem;
            PropertyList mPropertyFormat;
            PropertyToggle mPropertyGroupMode;
            PropertyToggle mPropertyAutoSizeMode;
            PropertyNumber mPropertyWidth;
            PropertyNumber mPropertyHeight;
            PropertyToggle mPropertyRawHeader;
            PropertyList mPropertyRawSeparator;
            PropertyToggle mPropertyIncludeDescription;
            PropertyToggle mPropertyIgnoreGrids;

            Accessor::Listener mListener{typeid(*this).name()};
            std::vector<std::unique_ptr<Track::Accessor::SmartListener>> mTrackListeners;
            std::vector<std::unique_ptr<Group::Accessor::SmartListener>> mGroupListeners;
            LayoutNotifier mDocumentLayoutNotifier;

            auto static constexpr documentItemFactor = 1000000;
            auto static constexpr groupItemFactor = documentItemFactor / 1000;
        };

        juce::Result toFile(Accessor& accessor, juce::File const file, juce::String const filePrefix, juce::String const& identifier, Options const& options, std::atomic<bool> const& shouldAbort, GetSizeFn getSizeFor);
    } // namespace Exporter
} // namespace Document

namespace XmlParser
{
    template <>
    void toXml<Document::Exporter::Options>(juce::XmlElement& xml, juce::Identifier const& attributeName, Document::Exporter::Options const& value);

    template <>
    auto fromXml<Document::Exporter::Options>(juce::XmlElement const& xml, juce::Identifier const& attributeName, Document::Exporter::Options const& defaultValue)
        -> Document::Exporter::Options;
} // namespace XmlParser

ANALYSE_FILE_END
