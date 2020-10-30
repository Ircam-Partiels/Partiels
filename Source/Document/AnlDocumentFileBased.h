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
        using Attribute = Model::Attribute;
        
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
        
        void updateChangeFlag();
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        juce::File mLastFile;
        std::unique_ptr<juce::XmlElement> mSavedXml;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileBased)
    };
}

ANALYSE_FILE_END
