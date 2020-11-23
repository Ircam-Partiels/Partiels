#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentTransport(Instance::get().getDocumentAccessor())
, mDocumentFileInfoPanel(Instance::get().getDocumentAccessor(), Instance::get().getDocumentFileBased(), Instance::get().getAudioFormatManager())
, mZoomTimeRuler(Instance::get().getDocumentAccessor().getAccessor<Document::AttrType::timeZoom>(0), Zoom::Ruler::Orientation::horizontal)
, mDocumentControlPanel(Instance::get().getDocumentAccessor(), Instance::get().getPluginListAccessor(), Instance::get().getAudioFormatManager())
, mDocumentMainPanel(Instance::get().getDocumentAccessor())
, mTimeScrollBar(Instance::get().getDocumentAccessor().getAccessor<Document::AttrType::timeZoom>(0), Zoom::ScrollBar::Orientation::horizontal)
{
    mZoomTimeRuler.setPrimaryTickInterval(0);
    mZoomTimeRuler.setTickReferenceValue(0.0);
    mZoomTimeRuler.setTickPowerInterval(10.0, 2.0);
    mZoomTimeRuler.setMaximumStringWidth(70.0);
    mZoomTimeRuler.setValueAsStringMethod([](double value)
    {
        auto time = value;
        auto const hours = static_cast<int>(std::floor(time / 3600.0));
        time -= static_cast<double>(hours) * 3600.0;
        auto const minutes = static_cast<int>(std::floor(time / 60.0));
        time -= static_cast<double>(minutes) * 60.0;
        auto const seconds = static_cast<int>(std::floor(time));
        time -= static_cast<double>(seconds);
        auto const ms = static_cast<int>(std::floor(time * 1000.0));
        return juce::String::formatted("%02d:%02d:%02d:%03d", hours, minutes, seconds, ms);
    });
    
    mZoomTimeRuler.onDoubleClick = [&]()
    {
        auto& acsr = Instance::get().getDocumentAccessor().getAccessor<Document::AttrType::timeZoom>(0);
        acsr.setValue<Zoom::AttrType::visibleRange>(acsr.getValue<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
    };
    addAndMakeVisible(mZoomTimeRuler);
    
    addAndMakeVisible(mDocumentTransport);
    addAndMakeVisible(mDocumentTransportSeparator);
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mHeaderSeparator);
    
    addAndMakeVisible(mZoomTimeRuler);
    addAndMakeVisible(mZoomTimeRulerSeparator);
    addAndMakeVisible(mDocumentControlPanel);
    addAndMakeVisible(mDocumentControlPanelSeparator);
    addAndMakeVisible(mDocumentMainPanel);
    
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
        auto top = bounds.removeFromTop(14 + separatorSize);
        mZoomTimeRulerSeparator.setBounds(top.removeFromBottom(separatorSize));
        top.removeFromLeft(240);
        mZoomTimeRuler.setBounds(top);
        
        mDocumentControlPanel.setBounds(bounds.removeFromLeft(240));
        mDocumentControlPanelSeparator.setBounds(bounds.removeFromLeft(2));
        mDocumentMainPanel.setBounds(bounds);
    }
}

ANALYSE_FILE_END
