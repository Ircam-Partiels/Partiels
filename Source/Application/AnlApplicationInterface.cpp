#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentTransport(Instance::get().getDocumentAccessor())
, mDocumentFileInfoPanel(Instance::get().getDocumentAccessor(), Instance::get().getDocumentFileBased(), Instance::get().getAudioFormatManager())
, mZoomStateTimeRuler(Instance::get().getDocumentAccessor().getZoomStateTimeAccessor())
, mDocumentAnalyzerPanel(Instance::get().getDocumentAccessor(), Instance::get().getPluginListAccessor())
{
    addAndMakeVisible(mDocumentTransport);
    addAndMakeVisible(mDocumentTransportSeparator);
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mHeaderSeparator);
    
    addAndMakeVisible(mZoomStateTimeRuler);
    addAndMakeVisible(mMainSeparator);
    addAndMakeVisible(mDocumentAnalyzerPanel);
    
    addAndMakeVisible(mBottomSeparator);
    addAndMakeVisible(mTimeSlider);
    mTimeSlider.setRange(0.0, 1.0);
    
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
    
    mTimeSlider.onValueChange = [&]()
    {
        auto& zoomStateTimeAccessor = Instance::get().getDocumentAccessor().getZoomStateTimeAccessor();
        auto const globalRange = std::get<0>(zoomStateTimeAccessor.getContraints());
        auto const newRangeStart = mTimeSlider.getMinValue() * globalRange.getLength() + globalRange.getStart();
        auto const newRangeEnd = mTimeSlider.getMaxValue() * globalRange.getLength() + globalRange.getStart();
        zoomStateTimeAccessor.fromModel({{newRangeStart, newRangeEnd}}, juce::NotificationType::sendNotificationSync);
    };
    
    mZoomStateTimeListener.onChanged = [&](Zoom::State::Accessor const& acsr, Zoom::State::Model::Attribute attribute)
    {
        if(attribute == Zoom::State::Model::Attribute::visibleRange)
        {
            auto const globalRange = std::get<0>(acsr.getContraints());
            auto const visibleRange = acsr.getModel().visibleRange;
            auto const scaledRangeStart = (visibleRange.getStart() - globalRange.getStart()) / globalRange.getLength();
            auto const scaledRangeEnd = (visibleRange.getEnd() - globalRange.getStart()) / globalRange.getLength();
            mTimeSlider.setMinAndMaxValues(scaledRangeStart, scaledRangeEnd, juce::NotificationType::dontSendNotification);
        }
    };
    
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.getZoomStateTimeAccessor().addListener(mZoomStateTimeListener, juce::NotificationType::sendNotificationSync);
    documentAccessor.addListener(mDocumentListener, juce::NotificationType::sendNotificationSync);
}

Application::Interface::~Interface()
{
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.removeListener(mDocumentListener);
    documentAccessor.getZoomStateTimeAccessor().removeListener(mZoomStateTimeListener);
}

void Application::Interface::resized()
{
    auto constexpr separatorSize = 2;
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(102);
    mDocumentTransport.setBounds(header.removeFromLeft(240));
    mDocumentTransportSeparator.setBounds(header.removeFromLeft(separatorSize));
    mDocumentFileInfoPanel.setBounds(header);
    mHeaderSeparator.setBounds(bounds.removeFromTop(separatorSize));
    
    mZoomStateTimeRuler.setBounds(bounds.removeFromTop(24));
    mMainSeparator.setBounds(bounds.removeFromTop(separatorSize));
    
    mTimeSlider.setBounds(bounds.removeFromBottom(16));
    mBottomSeparator.setBounds(bounds.removeFromBottom(separatorSize));
    mDocumentAnalyzerPanel.setBounds(bounds);
}

ANALYSE_FILE_END
