#include "AnlDocumentControlPanel.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Document::ControlPanel::ControlPanel(Accessor& accessor, PluginList::Accessor& pluginListAccessor, juce::AudioFormatManager const& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mPluginListTable(pluginListAccessor)
{
    addAndMakeVisible(mAddButton);
    mAddButton.setTooltip(juce::translate("Add a new analysis..."));
    mAddButton.onClick = [&]()
    {
        juce::DialogWindow::LaunchOptions launchOption;
        launchOption.dialogTitle = juce::translate("New Analyzer...");
        launchOption.content.setNonOwned(&mPluginListTable);
        launchOption.componentToCentreAround = this;
        launchOption.dialogBackgroundColour = findColour(juce::ResizableWindow::backgroundColourId, true);
        launchOption.escapeKeyTriggersCloseButton = true;
        launchOption.useNativeTitleBar = false;
        launchOption.resizable = false;
        launchOption.useBottomRightCornerResizer = false;
        launchOption.runModal();
    };
    
    mPluginListTable.onPluginSelected = [&](juce::String key)
    {
        juce::ModalComponentManager::getInstance()->cancelAllModalComponents();
        auto const name = PluginList::Scanner::getPluginDescriptions()[key].name;
        
        mAccessor.insertModel<AttrType::analyzers>(-1, Analyzer::Container{{key}, {name}, {0}, {{}}, {juce::Colours::black}, {}, {}});
        
        Analyzer::PropertyPanel panel(mAccessor.getAccessors<AttrType::analyzers>().back());
        panel.onAnalyse = [&]()
        {
            auto audioFormatReader = createAudioFormatReader(mAccessor, mAudioFormatManager, true);
            if(audioFormatReader == nullptr)
            {
                return;
            }
            //Analyzer::performAnalysis(mSections[i]->accessor, *audioFormatReader.get());
        };
        
        juce::DialogWindow::LaunchOptions launchOption;
        launchOption.dialogTitle = juce::translate("Analyzer Properties");
        launchOption.content.setNonOwned(&panel);
        launchOption.componentToCentreAround = this;
        launchOption.dialogBackgroundColour = findColour(juce::ResizableWindow::backgroundColourId, true);
        launchOption.escapeKeyTriggersCloseButton = true;
        launchOption.useNativeTitleBar = false;
        launchOption.resizable = false;
        launchOption.useBottomRightCornerResizer = false;
        launchOption.runModal();
    };
    
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::analyzers:
            {
                auto const& anlAcsrs = mAccessor.getAccessors<AttrType::analyzers>();
                mSections.erase(std::remove_if(mSections.begin(), mSections.end(), [&](auto const& section)
                {
                    return std::none_of(anlAcsrs.cbegin(), anlAcsrs.cend(), [&](auto const anlAcsr)
                    {
                        return &(section->accessor) == &(anlAcsr.get());
                    });
                }), mSections.end());
                
                for(size_t i = mSections.size(); i < anlAcsrs.size(); ++i)
                {
                    auto section = std::make_unique<Section>(anlAcsrs[i]);
                    anlWeakAssert(section != nullptr);
                    if(section != nullptr)
                    {
                        addAndMakeVisible(section->thumbnail);
                        addAndMakeVisible(section->separator);
                        mSections.push_back(std::move(section));
                    }
                }
                
                for(size_t i = 0; i < mSections.size(); ++i)
                {
                    if(mSections[i] != nullptr)
                    {
                        mSections[i]->thumbnail.onRemove = [this, i]()
                        {
                            mAccessor.eraseModel<AttrType::analyzers>(i, NotificationType::synchronous);
                        };
                        
                        mSections[i]->thumbnail.onRelaunch = [this, i]()
                        {
                            auto audioFormatReader = createAudioFormatReader(mAccessor, mAudioFormatManager, true);
                            if(audioFormatReader == nullptr)
                            {
                                return;
                            }
                            Analyzer::performAnalysis(mSections[i]->accessor, *audioFormatReader.get());
                        };
                    }
                }
                resized();
            }
                break;
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::ControlPanel::~ControlPanel()
{
    mAccessor.removeListener(mListener);
}

void Document::ControlPanel::resized()
{
    auto constexpr size = 100;
    auto bounds = getLocalBounds();
    for(auto& section : mSections)
    {
        if(section != nullptr)
        {
            auto lbounds = bounds.removeFromTop(size);
            section->separator.setBounds(lbounds.removeFromBottom(2));
            section->thumbnail.setBounds(lbounds);
        }
    }
    mAddButton.setBounds(bounds.removeFromTop(24).withWidth(bounds.getWidth() / 3).reduced(2));
}

ANALYSE_FILE_END
