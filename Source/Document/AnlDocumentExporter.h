#pragma once

#include "AnlDocumentModel.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    namespace Exporter
    {
        juce::Component const* getPlotComponent(juce::String const& identifier);
        juce::Rectangle<int> getPlotBounds(juce::String const& identifier);
        std::tuple<int, int, int> getPlotDimension(juce::String const& identifier);

        struct Options
        {
            using ColumnSeparator = Track::Result::FileDescription::ColumnSeparator;
            using ReaperType = Track::Result::FileDescription::ReaperType;

            // clang-format off
            enum class Format
            {
                  jpeg
                , png
                , csv
                , lab
                , json
                , cue
                , reaper
                , puredata
                , max
                , sdif
            };
            
            enum class TimePreset
            {
                  global
                , visible
                , selection
                , manual
            };
            // clang-format on

            Format format{Format::jpeg};
            bool useAutoSize{false};
            int imageWidth{1920};
            int imageHeight{1200};
            int imagePpi{144};
            bool includeHeaderRow{true};
            bool applyExtraThresholds{false};
            ReaperType reaperType{ReaperType::marker};
            ColumnSeparator columnSeparator{ColumnSeparator::comma};
            ColumnSeparator labSeparator{ColumnSeparator::space};
            bool disableLabelEscaping{false};
            bool includeDescription{true};
            juce::String sdifFrameSignature{"????"};
            juce::String sdifMatrixSignature{"????"};
            juce::String sdifColumnName;
            TimePreset timePreset{TimePreset::global};
            Zoom::Grid::Justification outsideGridJustification{0};

            bool operator==(Options const& rhs) const noexcept;
            bool operator!=(Options const& rhs) const noexcept;

            bool useImageFormat() const noexcept;
            bool useTextFormat() const noexcept;

            juce::String getFormatName() const;
            juce::String getFormatExtension() const;
            juce::String getFormatWilcard() const;

            char getSeparatorChar() const;
            char getLabSeparatorChar() const;

            bool isValid() const;
            bool isCompatible(Track::FrameType frameType) const;

            void setPlotDimension(juce::String const& identifier);
        };

        size_t getNumFilesToExport(Accessor const& accessor, std::set<juce::String> const& identifiers);

        class Panel
        : public juce::Component
        {
        public:
            Panel(Accessor& accessor, bool showTimeRange, bool showAutoSize);
            ~Panel() override;

            // juce::Component
            void resized() override;

            juce::Range<double> getTimeRange() const;

            void setOptions(Options const& options, juce::NotificationType notification);
            Options const& getOptions() const;

            void setSelectedIdentifiers(std::set<juce::String> const& identifiers, juce::NotificationType notification);
            std::set<juce::String> getSelectedIdentifiers() const;

            bool isAllDocumentSelected() const;
            bool isAllGroupSelected(juce::String const& groupId) const;
            std::set<juce::String> getDefaultSelectedIdentifiers() const;

            std::function<void(void)> onOptionsChanged = nullptr;
            std::function<void(void)> onSelectionChanged = nullptr;

        private:
            void showItemPopup(int groupToShow = 0);
            void sanitizeProperties(bool updateModel);
            void updateTimePreset(bool updateModel, juce::NotificationType notification);
            void setTimeRange(juce::Range<double> const& range, bool updateModel, juce::NotificationType notification);

            static auto constexpr OptionsChangedId = 1;
            static auto constexpr SelectionChangedId = 2;
            // juce::Component
            void handleCommandMessage(int commandId) override;

            Accessor& mAccessor;
            bool mShowAutoSize;
            Options mOptions;
            std::set<juce::String> mSelectedIdentifiers;

            PropertyList mPropertyItem;
            PropertyList mPropertyTimePreset;
            Misc::PropertyHMSmsField mPropertyTimeStart;
            Misc::PropertyHMSmsField mPropertyTimeEnd;
            Misc::PropertyHMSmsField mPropertyTimeLength;
            PropertyList mPropertyFormat;
            PropertyList mPropertySizePreset;
            PropertyNumber mPropertyWidth;
            PropertyNumber mPropertyHeight;
            PropertyNumber mPropertyPpi;
            PropertyToggle mPropertyRowHeader;
            PropertyList mPropertyColumnSeparator;
            PropertyList mPropertyLabSeparator;
            PropertyToggle mPropertyDisableLabelEscaping;
            PropertyList mPropertyReaperType;
            PropertyToggle mPropertyIncludeDescription;
            PropertyText mPropertySdifFrame;
            PropertyText mPropertySdifMatrix;
            PropertyText mPropertySdifColName;
            PropertyToggle mPropertyApplyExtraThresholds;
            PropertyList mPropertyOutsideGridJustification;

            Accessor::Listener mListener{typeid(*this).name()};
            Zoom::Accessor::Listener mTimeZoomListener{typeid(*this).name()};
            Transport::Accessor::Listener mTransportZoomListener{typeid(*this).name()};
            std::vector<std::unique_ptr<Track::Accessor::SmartListener>> mTrackListeners;
            std::vector<std::unique_ptr<Group::Accessor::SmartListener>> mGroupListeners;
            LayoutNotifier mDocumentLayoutNotifier;
        };

        juce::Result exportTo(Accessor& accessor, juce::File const directory, juce::Range<double> const& timeRange, std::set<size_t> const& channels, juce::String const filePrefix, juce::String const& identifier, Options const& options, std::atomic<bool> const& shouldAbort);

        juce::Result exportTo(Accessor& accessor, juce::File const file, juce::Range<double> const& timeRange, std::set<size_t> const& channels, juce::String const filePrefix, std::set<juce::String> const& identifiers, Options const& options, std::atomic<bool> const& shouldAbort);

        juce::Result clearUnusedAudioFiles(Accessor const& accessor, juce::File directory);
        juce::Result clearUnusedTrackFiles(Accessor const& accessor, juce::File directory);
        juce::Result consolidateAudioFiles(Accessor& accessor, juce::File directory);
        juce::Result consolidateTrackFiles(Accessor& accessor, juce::File directory);
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
