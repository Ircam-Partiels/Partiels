#include "AnlDocumentAnalyzerPanel.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Document::AnalyzerPanel::AnalyzerPanel(Accessor& accessor, PluginList::Accessor& pluginListAccessor)
: mAccessor(accessor)
, mPluginListTable(pluginListAccessor)
{
    mPluginListTable.onPluginSelected = [&](juce::String key)
    {
        if(dialogWindow != nullptr)
        {
            dialogWindow->exitModalState(0);
        }
        
        auto analyzerModel = std::make_unique<Analyzer::Model>();
        anlWeakAssert(analyzerModel != nullptr);
        if(analyzerModel != nullptr)
        {
            analyzerModel->key = key;
            auto copy = mAccessor.getModel();
            copy.analyzers.push_back(std::move(analyzerModel));
            mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
        }
    };
    
    mListener.onChanged = [&](Accessor& acsr, Attribute attribute)
    {
        switch (attribute)
        {
            case Attribute::analyzers:
            {
                auto const& ref = mAccessor.getModel();
                mSections.resize(ref.analyzers.size());
                std::transform(ref.analyzers.cbegin(), ref.analyzers.cend(), mSections.begin(), [&](auto const& analyzer)
                {
                    anlWeakAssert(analyzer != nullptr);
                    auto section = std::make_unique<Section>();
                    anlWeakAssert(section != nullptr);
                    if(section != nullptr && analyzer != nullptr)
                    {
                        section->name.setText(PluginList::Scanner::getPluginDescriptions()[analyzer->key].name, juce::NotificationType::dontSendNotification);
                        addAndMakeVisible(section->close);
                        addAndMakeVisible(section->name);
                        addAndMakeVisible(section->results);
                    }
                    //        mAnalyzerAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
                    //        if(mAudioFormatReader != nullptr)
                    //        {
                    //            mAnalyzerProcessor.perform(*(mAudioFormatReader.get()));
                    //        }m
                    return section;
                });
                resized();
            }
                break;
        }
    };
    mAccessor.addListener(mListener, juce::NotificationType::sendNotificationSync);
}

Document::AnalyzerPanel::~AnalyzerPanel()
{
    mAccessor.removeListener(mListener);
}

void Document::AnalyzerPanel::resized()
{
    auto constexpr size = 24;
    auto bounds = getLocalBounds();
    for(auto& section : mSections)
    {
        if(section != nullptr)
        {
            auto lbounds = bounds.removeFromTop(size);
            section->close.setBounds(lbounds.removeFromLeft(30));
            section->name.setBounds(lbounds.removeFromLeft(200));
            section->results.setBounds(lbounds);
        }
    }
}

void Document::AnalyzerPanel::mouseDoubleClick(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    juce::DialogWindow::LaunchOptions launchOption;
    launchOption.dialogTitle = juce::translate("New Analyzer...");
    launchOption.content.setNonOwned(&mPluginListTable);
    launchOption.componentToCentreAround = this;
    launchOption.dialogBackgroundColour = findColour(juce::ResizableWindow::backgroundColourId, true);
    launchOption.escapeKeyTriggersCloseButton = true;
    launchOption.useNativeTitleBar = false;
    launchOption.resizable = false;
    launchOption.useBottomRightCornerResizer = false;
    dialogWindow = launchOption.launchAsync();
    dialogWindow->runModalLoop();
}

ANALYSE_FILE_END
