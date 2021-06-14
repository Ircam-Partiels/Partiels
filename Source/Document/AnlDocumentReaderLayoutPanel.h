#pragma once

#include "AnlDocumentDirector.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class ReaderLayoutPanel
    : public FloatingWindowContainer
    , public juce::DragAndDropContainer
    , public juce::FileDragAndDropTarget
    {
    public:
        ReaderLayoutPanel(Director& director);
        ~ReaderLayoutPanel() override;

        // juce::Component
        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;
        void lookAndFeelChanged() override;
        void parentHierarchyChanged() override;

        // juce::FileDragAndDropTarget
        bool isInterestedInFileDrag(juce::StringArray const& files) override;
        void fileDragEnter(juce::StringArray const& files, int x, int y) override;
        void fileDragExit(juce::StringArray const& files) override;
        void filesDropped(juce::StringArray const& files, int x, int y) override;

    private:
        class FileInfoPanel
        : public juce::Component
        {
        public:
            FileInfoPanel();
            ~FileInfoPanel() override = default;

            void setAudioFormatReader(juce::File const& file, juce::AudioFormatReader const* reader);

            // juce::Component
            void resized() override;

        private:
            PropertyTextButton mFilePath;
            PropertyLabel mFileFormat{juce::translate("Format"), juce::translate("The format of the audio file")};
            PropertyLabel mSampleRate{juce::translate("Sample Rate"), juce::translate("The sample rate of the audio file")};

            PropertyLabel mBitPerSample{juce::translate("Bits"), juce::translate("The number of bits per samples of the audio file")};
            PropertyLabel mLengthInSamples{juce::translate("Length"), juce::translate("The length of the audio file in samples")};
            PropertyLabel mDurationInSeconds{juce::translate("Duration"), juce::translate("The duration of the audio file in seconds")};
            PropertyLabel mNumChannels{juce::translate("Channels"), juce::translate("The number of channels of the audio file")};

            std::vector<std::unique_ptr<PropertyLabel>> mMetaDataPanels;

            ConcertinaTable mConcertinaTable{"", false};
            juce::Viewport mViewport;
        };

        class Channel
        : public juce::Component
        {
        public:
            Channel(FileInfoPanel& fileInfoPanel, int index, juce::File const& file, int channel, std::unique_ptr<juce::AudioFormatReader> reader);
            ~Channel() override = default;

            std::function<void(void)> onDelete = nullptr;
            std::function<void(int)> onChannelChange = nullptr;

            // juce::Component
            void resized() override;
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseDown(juce::MouseEvent const& event) override;
            void mouseDrag(juce::MouseEvent const& event) override;
            void mouseUp(juce::MouseEvent const& event) override;
            bool keyPressed(juce::KeyPress const& key) override;
            void focusGained(juce::Component::FocusChangeType cause) override;
            void focusLost(juce::Component::FocusChangeType cause) override;
            void focusOfChildComponentChanged(juce::Component::FocusChangeType cause) override;

        private:
            struct Entry
            : public juce::Component
            {
                Entry();
                ~Entry() override = default;

                // juce::Component
                void resized() override;
                void paint(juce::Graphics& g) override;

                juce::Label thumbLabel;
                juce::Label fileNameLabel;
                juce::ComboBox channelMenu;
                juce::ImageButton warningButton;
            };

            FileInfoPanel& mFileInfoPanel;
            Entry mEntry;
            int const mIndex;
            juce::File const mFile;
            std::unique_ptr<juce::AudioFormatReader> const mReader;
            Decorator mDecorator{mEntry};
        };

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        juce::AudioFormatManager& mAudioFormatManager{mDirector.getAudioFormatManager()};
        Accessor::Listener mListener;

        std::vector<std::unique_ptr<Channel>> mChannels;
        DraggableTable mDraggableTable{"Channel"};
        juce::Viewport mViewport;
        juce::ImageButton mAddButton;
        juce::Label mAddLabel;
        juce::ImageButton mAlertButton;
        juce::Label mAlertLabel;
        ColouredPanel mSeparator;
        FileInfoPanel mFileInfoPanel;
        BoundsListener mBoundsListener;

        juce::TooltipWindow mTooltipWindow{this};
        bool mIsDragging{false};
        JUCE_LEAK_DETECTOR(ReaderLayoutPanel)
    };
} // namespace Document

ANALYSE_FILE_END
