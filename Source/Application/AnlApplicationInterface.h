#pragma once

#include "../Document/AnlDocumentFileInfoPanel.h"
#include "../Document/AnlDocumentModel.h"
#include "../Document/AnlDocumentSection.h"
#include "../Transport/AnlTransportDisplay.h"
#include "AnlApplicationCommandTarget.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Interface
    : public juce::Component
    , public CommandTarget
    , private juce::ComponentListener
    {
    public:
        Interface();
        ~Interface() override;

        void moveKeyboardFocusTo(juce::String const& identifier);

        // juce::Component
        void resized() override;

    private:
        // juce::ComponentListener
        void componentVisibilityChanged(juce::Component& component) override;

        class Loader
        : public juce::Component
        , public juce::FileDragAndDropTarget
        , private juce::ApplicationCommandManagerListener
        {
        public:
            Loader();
            ~Loader() override;

            void updateState();

            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;

            // juce::FileDragAndDropTarget
            bool isInterestedInFileDrag(juce::StringArray const& files) override;
            void fileDragEnter(juce::StringArray const& files, int x, int y) override;
            void fileDragExit(juce::StringArray const& files) override;
            void filesDropped(juce::StringArray const& files, int x, int y) override;

        private:
            // juce::ApplicationCommandManagerListener
            void applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info) override;
            void applicationCommandListChanged() override;

            Document::Accessor::Listener mDocumentListener;
            juce::TextButton mLoadFileButton{juce::translate("Load")};
            juce::Label mLoadFileLabel{"", juce::translate("Or\nDrag & Drop\n(Audio/Document)")};
            juce::TextButton mAddTrackButton{juce::translate("Add Track")};
            juce::TextButton mLoadTemplateButton{juce::translate("Load Template")};
            bool mIsDragging{false};
        };

        Transport::Display mTransportDisplay;
        Document::Section mDocumentSection;
        Loader mLoader;
        Decorator mLoaderDecorator{mLoader, 1, 2.0f};
        Tooltip::Display mToolTipDisplay;
    };
} // namespace Application

ANALYSE_FILE_END
