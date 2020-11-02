#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentTransport(Instance::get().getDocumentAccessor())
, mDocumentFileInfoPanel(Instance::get().getDocumentAccessor(), Instance::get().getDocumentFileBased(), Instance::get().getAudioFormatManager())
, mZoomStateTimeRuler(Instance::get().getDocumentAccessor().getZoomStateTimeAccessor())
, mDocumentAnalyzerPanel(Instance::get().getDocumentAccessor(), Instance::get().getPluginListAccessor())
, mTimeSlider(Instance::get().getDocumentAccessor().getZoomStateTimeAccessor(), Zoom::State::Slider::Orientation::horizontal)
{
    addAndMakeVisible(mDocumentTransport);
    addAndMakeVisible(mDocumentTransportSeparator);
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mHeaderSeparator);
    
    addAndMakeVisible(mZoomStateTimeRuler);
    addAndMakeVisible(mMainSeparator);
    addAndMakeVisible(mDocumentAnalyzerPanel);
    
    addAndMakeVisible(mBottomSeparator);
    addAndMakeVisible(mToolTipDisplay);
    addAndMakeVisible(mTooTipSeparator);
    addAndMakeVisible(mTimeSlider);
    
    mDocumentListener.onChanged = [&](Document::Accessor const& acsr, Document::Model::Attribute attribute)
    {
        if(attribute == Document::Model::Attribute::file)
        {
            auto const file = acsr.getModel().file;
            auto const& audioFormatManager = Instance::get().getAudioFormatManager();
            auto const isDocumentEnable = file.existsAsFile() && audioFormatManager.getWildcardForAllFormats().contains(file.getFileExtension());
                
            mDocumentTransport.setEnabled(isDocumentEnable);
            mDocumentFileInfoPanel.setEnabled(isDocumentEnable);
            mDocumentAnalyzerPanel.setEnabled(isDocumentEnable);
        }
    };
    
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.addListener(mDocumentListener, juce::NotificationType::sendNotificationSync);
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
    }
    
    mHeaderSeparator.setBounds(bounds.removeFromTop(separatorSize));
    mZoomStateTimeRuler.setBounds(bounds.removeFromTop(24));
    mMainSeparator.setBounds(bounds.removeFromTop(separatorSize));
    
    {
        auto footer = bounds.removeFromBottom(24);
        footer.removeFromRight(24);
        mToolTipDisplay.setBounds(footer.removeFromLeft(240));
        mTooTipSeparator.setBounds(footer.removeFromLeft(separatorSize));
        mTimeSlider.setBounds(footer);
    }
    
    mBottomSeparator.setBounds(bounds.removeFromBottom(separatorSize));
    mDocumentAnalyzerPanel.setBounds(bounds);
}

ANALYSE_FILE_END
