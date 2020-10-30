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
    
    mListener.onChanged = [&](Accessor const& acsr, Attribute attribute)
    {
        switch (attribute)
        {
            case Attribute::analyzers:
            {
                auto const& ref = mAccessor.getModel();
                for(size_t i = mSections.size(); i < ref.analyzers.size(); ++i)
                {
                    auto& anlAcsr = mAccessor.getAnalyzerAccessor(i);
                    auto section = std::make_unique<Section>(anlAcsr);
                    anlWeakAssert(section != nullptr);
                    if(section != nullptr)
                    {
                        addAndMakeVisible(section->thumbnail);
                        addAndMakeVisible(section->results);
                        mSections.push_back(std::move(section));
                    }
                }
                mSections.resize(ref.analyzers.size());
                for(size_t i = 0; i < mSections.size(); ++i)
                {
                    if(mSections[i] != nullptr)
                    {
                        mSections[i]->thumbnail.onRemove = [this, i]()
                        {
                            auto copy = mAccessor.getModel();
                            copy.analyzers.erase(copy.analyzers.begin() + static_cast<long>(i));
                            mAccessor.fromModel(copy, juce::NotificationType::sendNotificationSync);
                        };
                    }
                }
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
    auto constexpr size = 48;
    auto bounds = getLocalBounds();
    for(auto& section : mSections)
    {
        if(section != nullptr)
        {
            auto lbounds = bounds.removeFromTop(size);
            section->thumbnail.setBounds(lbounds.removeFromLeft(200));
            section->results.setBounds(lbounds);
        }
    }
}

void Document::AnalyzerPanel::mouseDoubleClick(juce::MouseEvent const& event)
{
    if(!isEnabled())
    {
        return;
    }
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
