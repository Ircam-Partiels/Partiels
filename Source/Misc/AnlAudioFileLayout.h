#pragma once

#include "AnlBoundsListener.h"
#include "AnlDecorator.h"
#include "AnlDraggableTable.h"
#include "AnlFileWatcher.h"
#include "AnlXmlParser.h"

ANALYSE_FILE_BEGIN

struct AudioFileLayout
{
    // clang-format off
    enum ChannelLayout : int
    {
          all = -2
        , mono = -1
        , split = 0
    };
    // clang-format on

    AudioFileLayout(juce::File const& f = juce::File{}, int c = 0)
    : file(f)
    , channel(c)
    {
    }

    juce::File file;
    int channel = 0;

    inline bool operator==(AudioFileLayout const& rhd) const noexcept
    {
        return file == rhd.file && channel == rhd.channel;
    }

    inline bool operator!=(AudioFileLayout const& rhd) const noexcept
    {
        return !(*this == rhd);
    }
};

namespace XmlParser
{
    template <>
    void toXml<AudioFileLayout>(juce::XmlElement& xml, juce::Identifier const& attributeName, AudioFileLayout const& value);

    template <>
    auto fromXml<AudioFileLayout>(juce::XmlElement const& xml, juce::Identifier const& attributeName, AudioFileLayout const& defaultValue)
        -> AudioFileLayout;
} // namespace XmlParser

class AudioFileLayoutTable
: public juce::Component
, public juce::FileDragAndDropTarget
, private FileWatcher
{
public:
    // clang-format off
    enum SupportMode
    {
          supportLayoutAll = 1
        , supportLayoutMono = 2
        , supportMultipleSampleRates = 4
        , all = supportLayoutAll | supportLayoutMono | supportMultipleSampleRates
    };
    // clang-format on

    AudioFileLayoutTable(juce::AudioFormatManager& audioFormatManager, SupportMode mode, AudioFileLayout::ChannelLayout preferredChannelLayout);
    ~AudioFileLayoutTable() override;

    void setLayout(std::vector<AudioFileLayout> const& layout, juce::NotificationType notification);
    std::vector<AudioFileLayout> getLayout() const;

    void setSelection(std::set<size_t> indices, juce::NotificationType notification);
    std::set<size_t> getSelection() const;

    std::function<void(void)> onLayoutChanged = nullptr;
    std::function<void(void)> onSelectionChanged = nullptr;

    // juce::Component
    void resized() override;
    void paintOverChildren(juce::Graphics& g) override;
    void lookAndFeelChanged() override;
    void parentHierarchyChanged() override;
    bool keyPressed(juce::KeyPress const& key) override;
    void handleCommandMessage(int commandId) override;

    // juce::FileDragAndDropTarget
    bool isInterestedInFileDrag(juce::StringArray const& files) override;
    void fileDragEnter(juce::StringArray const& files, int x, int y) override;
    void fileDragExit(juce::StringArray const& files) override;
    void filesDropped(juce::StringArray const& files, int x, int y) override;

private:
    // FileWatcher
    bool fileHasBeenRemoved(juce::File const& file) override;
    bool fileHasBeenRestored(juce::File const& file) override;
    bool fileHasBeenModified(juce::File const& file) override;

    void setLayout(std::vector<AudioFileLayout> const& layout);
    void updateLayout();

    bool selectionAll();
    bool moveSelectionUp(bool preserve);
    bool moveSelectionDown(bool preserve);
    bool deleteSelection();
    bool copySelection();
    bool cutSelection();
    bool pasteSelection();

    class Channel
    : public juce::Component
    {
    public:
        Channel(AudioFileLayoutTable& owner, size_t index, AudioFileLayout audioFileLayout, SupportMode mode);
        ~Channel() override = default;

        double sampleRate = 0.0;
        void setSelected(bool state);

        // juce::Component
        void resized() override;
        void mouseMove(juce::MouseEvent const& event) override;
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;

    private:
        struct Entry
        : public juce::Component
        {
            Entry(size_t i, juce::String const& fileName);
            ~Entry() override = default;

            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;

            size_t const index;
            juce::Label thumbLabel;
            juce::Label fileNameLabel;
            juce::ComboBox channelMenu;
            juce::ImageButton warningButton;
        };

        AudioFileLayoutTable& mOwner;
        AudioFileLayout mAudioFileLayout;
        Entry mEntry;
        Decorator mDecorator{mEntry};
    };

    juce::AudioFormatManager& mAudioFormatManager;
    SupportMode const mSupportMode;
    AudioFileLayout::ChannelLayout const mPreferredChannelLayout;
    std::vector<std::unique_ptr<Channel>> mChannels;
    DraggableTable mDraggableTable{"Channel"};
    BoundsListener mBoundsListener;
    juce::Viewport mViewport;
    juce::ImageButton mAddButton;
    juce::Label mAddLabel;
    juce::ImageButton mAlertButton;
    juce::Label mAlertLabel;
    std::vector<AudioFileLayout> mLayout;
    std::set<size_t> mSelection;
    std::vector<AudioFileLayout> mClipBoard;
    bool mIsDragging{false};
    juce::UndoManager mUndoManager;
};

std::vector<AudioFileLayout> getAudioFileLayouts(juce::AudioFormatManager& audioFormatManager, juce::Array<juce::File> const& files, AudioFileLayout::ChannelLayout preferredChannelLayout);
std::vector<AudioFileLayout> getAudioFileLayouts(juce::AudioFormatManager& audioFormatManager, juce::StringArray const& files, AudioFileLayout::ChannelLayout preferredChannelLayout);

ANALYSE_FILE_END
