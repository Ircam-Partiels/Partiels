#pragma once

#include "AnlDocumentDirector.h"
#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class FileBased
    : public juce::FileBasedDocument
    , private juce::AsyncUpdater
    {
    public:
        FileBased(Accessor& accessor, Director& director, juce::String const& fileExtension, juce::String const& fileWildCard, juce::String const& openFileDialogTitle, juce::String const& saveFileDialogTitle);
        ~FileBased() override;

        static Document::AttrContainer const& getDefaultContainer();

        juce::Result consolidate();

        juce::Result loadTemplate(juce::File const& file);
        juce::Result loadBackup(juce::File const& file);
        juce::Result saveBackup(juce::File const& file);

    protected:
        // juce::FileBasedDocument
        juce::String getDocumentTitle() override;
        juce::Result loadDocument(juce::File const& file) override;
        juce::Result saveDocument(juce::File const& file) override;
        juce::File getLastDocumentOpened() override;
        void setLastDocumentOpened(juce::File const& file) override;
        void changed() override;

    private:
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;

        Accessor& mAccessor;
        Director& mDirector;
        juce::String const mFileExtension;
        Accessor::Listener mListener;
        Track::Accessor::Listener mTrackListener;
        std::vector<std::reference_wrapper<Track::Accessor>> mTrackAccessors;
        Group::Accessor::Listener mGroupListener;
        std::vector<std::reference_wrapper<Group::Accessor>> mGroupAccessors;

        juce::File mLastFile;
        Accessor mSavedStateAccessor;
    };
} // namespace Document

ANALYSE_FILE_END
