#pragma once

#include "AnlDocumentModel.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    class FileBased
    : public juce::FileBasedDocument
    , private juce::AsyncUpdater
    {
    public:
        FileBased(Accessor& accessor, juce::String const& fileExtension, juce::String const& fileWildCard, juce::String const& openFileDialogTitle, juce::String const& saveFileDialogTitle);
        ~FileBased() override;

    protected:
        
        // juce::FileBasedDocument
        juce::String getDocumentTitle() override;
        juce::Result loadDocument(juce::File const& file) override;
        juce::Result saveDocument(juce::File const& file) override;
        juce::File getLastDocumentOpened() override;
        void setLastDocumentOpened(juce::File const& file) override;
        
    private:
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override;
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        juce::File mLastFile;
        Container mSavedStateContainer;
        Accessor mSavedStateAccessor {mSavedStateContainer};
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileBased)
    };
}

ANALYSE_FILE_END
