#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
: mDocumentTransport(Instance::get().getDocumentAccessor())
, mDocumentFileInfoPanel(Instance::get().getDocumentAccessor(), Instance::get().getAudioFormatManager())
, mDocumentAnalyzerPanel(Instance::get().getDocumentAccessor(), Instance::get().getPluginListAccessor())
{
    addAndMakeVisible(mDocumentTransport);
    addAndMakeVisible(mDocumentTransportSeparator);
    addAndMakeVisible(mDocumentFileInfoPanel);
    addAndMakeVisible(mHeaderSeparator);
    addAndMakeVisible(mDocumentAnalyzerPanel);
    
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
    
    Instance::get().getDocumentAccessor().addListener(mDocumentListener, juce::NotificationType::sendNotificationSync);
}

Application::Interface::~Interface()
{
    Instance::get().getDocumentAccessor().removeListener(mDocumentListener);
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
    mDocumentAnalyzerPanel.setBounds(bounds);
}

ANALYSE_FILE_END
