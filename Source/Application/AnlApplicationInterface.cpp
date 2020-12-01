#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentFileInfoPanel(Instance::get().getDocumentAccessor(), Instance::get().getDocumentFileBased(), Instance::get().getAudioFormatManager())
, mDocumentTransport(Instance::get().getDocumentAccessor())
, mDocumentSection(Instance::get().getDocumentAccessor())
{    
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mDocumentTransport);
    
    addAndMakeVisible(mDocumentSection);
    
    addAndMakeVisible(mToolTipDisplay);
    
    mDocumentListener.onChanged = [&](Document::Accessor const& acsr, Document::AttrType attribute)
    {
        switch(attribute)
        {
            case Document::AttrType::file:
            {
                auto const file = acsr.getAttr<Document::AttrType::file>();
                auto const& audioFormatManager = Instance::get().getAudioFormatManager();
                auto const isDocumentEnable = file.existsAsFile() && audioFormatManager.getWildcardForAllFormats().contains(file.getFileExtension());
                
                mDocumentTransport.setEnabled(isDocumentEnable);
                mDocumentFileInfoPanel.setEnabled(isDocumentEnable);
                mDocumentSection.setEnabled(isDocumentEnable);
            }
                break;
            case Document::isLooping:
            case Document::gain:
            case Document::isPlaybackStarted:
            case Document::playheadPosition:
            case Document::timeZoom:
            case Document::layout:
            case Document::layoutHorizontal:
            case Document::analyzers:
                break;
        }
    };
    
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.addListener(mDocumentListener, NotificationType::synchronous);
}

Application::Interface::~Interface()
{
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.removeListener(mDocumentListener);
}

void Application::Interface::resized()
{
    auto bounds = getLocalBounds();
    
    auto header = bounds.removeFromTop(102);
    mDocumentFileInfoPanel.setBounds(header.removeFromRight(320));
    mDocumentTransport.setBounds(header.removeFromLeft(240));
    
    mToolTipDisplay.setBounds(bounds.removeFromBottom(24));
    mDocumentSection.setBounds(bounds);
}

ANALYSE_FILE_END
