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
        FileBased(Director& director, juce::String const& fileExtension, juce::String const& fileWildCard, juce::String const& openFileDialogTitle, juce::String const& saveFileDialogTitle);
        ~FileBased() override;

        Accessor const& getDefaultAccessor();

        juce::Result consolidate();

        juce::Result loadTemplate(juce::File const& file, bool adaptOnSampleRate);
        juce::Result loadBackup(juce::File const& file);
        juce::Result saveBackup(juce::File const& file);

        static std::variant<std::unique_ptr<juce::XmlElement>, juce::Result> parse(juce::File const& file);
        static juce::Result saveTo(Accessor& accessor, juce::File const& file);
        static void loadTemplate(Accessor& accessor, juce::XmlElement const& xml, bool adaptOnSampleRate);
        static juce::File getConsolidateDirectory(juce::File const& file);

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

        static void replacePath(juce::XmlElement& element, juce::String const& oldPath, juce::String const& newPath);

        Director& mDirector;
        Accessor& mAccessor;
        juce::String const mFileExtension;
        Accessor::Listener mListener{typeid(*this).name()};
        Track::Accessor::Listener mTrackListener{typeid(*this).name()};
        std::vector<std::reference_wrapper<Track::Accessor>> mTrackAccessors;
        Group::Accessor::Listener mGroupListener{typeid(*this).name()};
        std::vector<std::reference_wrapper<Group::Accessor>> mGroupAccessors;

        juce::File mLastFile;
        Accessor mSavedStateAccessor;
        Accessor const mDefaultDdocument;
    };
} // namespace Document

ANALYSE_FILE_END
