#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentTransport(Instance::get().getDocumentAccessor())
, mDocumentFileInfoPanel(Instance::get().getDocumentAccessor(), Instance::get().getDocumentFileBased(), Instance::get().getAudioFormatManager())
, mZoomStateTimeRuler(Instance::get().getDocumentAccessor().getAccessor<Document::AttrType::timeZoom>())
, mDocumentControlPanel(Instance::get().getDocumentAccessor(), Instance::get().getPluginListAccessor(), Instance::get().getAudioFormatManager())
, mTimeScrollBar(Instance::get().getDocumentAccessor().getAccessor<Document::AttrType::timeZoom>(), Zoom::State::ScrollBar::Orientation::horizontal)
{
    addAndMakeVisible(mDocumentTransport);
    addAndMakeVisible(mDocumentTransportSeparator);
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mHeaderSeparator);
    
    addAndMakeVisible(mZoomStateTimeRuler);
    addAndMakeVisible(mZoomStateTimeRulerSeparator);
    addAndMakeVisible(mDocumentControlPanel);
    addAndMakeVisible(mDocumentControlPanelSeparator);
    
    addAndMakeVisible(mBottomSeparator);
    addAndMakeVisible(mToolTipDisplay);
    addAndMakeVisible(mTooTipSeparator);
    addAndMakeVisible(mTimeScrollBar);
    
    mDocumentListener.onChanged = [&](Document::Accessor const& acsr, Document::AttrType attribute)
    {
        switch(attribute)
        {
            case Document::AttrType::file:
            {
                auto const file = acsr.getValue<Document::AttrType::file>();
                auto const& audioFormatManager = Instance::get().getAudioFormatManager();
                auto const isDocumentEnable = file.existsAsFile() && audioFormatManager.getWildcardForAllFormats().contains(file.getFileExtension());
                
                mDocumentTransport.setEnabled(isDocumentEnable);
                mDocumentFileInfoPanel.setEnabled(isDocumentEnable);
                mDocumentControlPanel.setEnabled(isDocumentEnable);
            }
                break;
                
            default:
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
    auto constexpr separatorSize = 2;
    auto bounds = getLocalBounds();
    
    {
        auto header = bounds.removeFromTop(102);
        mDocumentTransport.setBounds(header.removeFromLeft(240));
        mDocumentTransportSeparator.setBounds(header.removeFromLeft(separatorSize));
        mDocumentFileInfoPanel.setBounds(header);
        mHeaderSeparator.setBounds(bounds.removeFromTop(separatorSize));
    }
    
    
    {
        auto footer = bounds.removeFromBottom(36);
        mToolTipDisplay.setBounds(footer.removeFromBottom(24));
        mTooTipSeparator.setBounds(footer.removeFromBottom(separatorSize));
        footer.removeFromRight(8);
        footer.removeFromLeft(240);
        mTimeScrollBar.setBounds(footer);
        mBottomSeparator.setBounds(bounds.removeFromBottom(separatorSize));
    }
    
    {
        auto top = bounds.removeFromTop(24 + separatorSize);
        mZoomStateTimeRulerSeparator.setBounds(top.removeFromBottom(separatorSize));
        top.removeFromLeft(240);
        mZoomStateTimeRuler.setBounds(top);
        
        mDocumentControlPanel.setBounds(bounds.removeFromLeft(240));
        mDocumentControlPanelSeparator.setBounds(bounds.removeFromLeft(2));
    }
}

ANALYSE_FILE_END
