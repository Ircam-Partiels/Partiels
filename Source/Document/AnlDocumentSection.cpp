#include "AnlDocumentSection.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Document::Section::Content::Content(Analyzer::Accessor& acsr, Zoom::Accessor& timeZoomAcsr)
: mAccessor(acsr)
, mTimeZoomAccessor(timeZoomAcsr)
{
    mThumbnail.onRemove = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    mThumbnail.onRelaunch = [&]()
    {
        if(onRelaunch != nullptr)
        {
            onRelaunch();
        }
    };
    addAndMakeVisible(mThumbnail);
    addAndMakeVisible(mRenderer);
    addAndMakeVisible(mRuler);
    addAndMakeVisible(mScrollbar);
    setSize(80, 100);
}

Analyzer::Accessor const& Document::Section::Content::getAccessor() const
{
    return mAccessor;
}

void Document::Section::Content::resized()
{
    auto bounds = getLocalBounds();
    mThumbnail.setBounds(bounds.removeFromLeft(240));
    mScrollbar.setBounds(bounds.removeFromRight(8));
    mRuler.setBounds(bounds.removeFromRight(16));
    mRenderer.setBounds(bounds);
}

Document::Section::Section(Accessor& accessor, PluginList::Accessor& pluginListAccessor, juce::AudioFormatManager const& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mPluginListTable(pluginListAccessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::analyzers:
            {
                auto const& anlAcsrs = mAccessor.getAccessors<AttrType::analyzers>();
                auto& timeZoomAcsr = mAccessor.getAccessor<AttrType::timeZoom>(0);
                
                mContents.erase(std::remove_if(mContents.begin(), mContents.end(), [&](auto const& content)
                {
                    return std::none_of(anlAcsrs.cbegin(), anlAcsrs.cend(), [&](auto const anlAcsr)
                    {
                        return &(content->getAccessor()) == &(anlAcsr.get());
                    });
                }), mContents.end());
                
                
                for(size_t i = mContents.size(); i < anlAcsrs.size(); ++i)
                {
                    auto content = std::make_unique<Content>(anlAcsrs[i], timeZoomAcsr);
                    anlWeakAssert(content != nullptr);
                    if(content != nullptr)
                    {
                        mContents.push_back(std::move(content));
                    }
                }
                
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    if(mContents[i] != nullptr)
                    {
                        mContents[i]->onRemove = [this, i]()
                        {
                            mAccessor.eraseAccessor<AttrType::analyzers>(i, NotificationType::synchronous);
                        };
                        
                        mContents[i]->onRelaunch = [this, i]()
                        {
                            auto afr = createAudioFormatReader(mAccessor, mAudioFormatManager, true);
                            if(afr == nullptr)
                            {
                                return;
                            }
                            auto& anlAcsr = mAccessor.getAccessor<AttrType::analyzers>(i);
                            Analyzer::performAnalysis(anlAcsr, *afr.get());
                        };
                    }
                }
                
                std::vector<int> sizes;
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    sizes.push_back(mContents[i]->getHeight());
                }
                auto& layoutAcsr = mAccessor.getAccessor<AttrType::layout>(0);
                layoutAcsr.setAttr<Layout::StrechableContainer::AttrType::sizes>(sizes, NotificationType::synchronous);
                
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContainer.setContent(i, *mContents[i].get(), 100);
                }
            }
                break;
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
            case AttrType::playheadPosition:
            case AttrType::timeZoom:
            case AttrType::layout:
                break;
        }
    };
    
    mPluginListTable.onPluginSelected = [&](juce::String key)
    {
        juce::ModalComponentManager::getInstance()->cancelAllModalComponents();
        auto const name = PluginList::Scanner::getPluginDescriptions()[key].name;
        
        if(!mAccessor.insertAccessor<AttrType::analyzers>(-1, Analyzer::Container{{key}, {name}, {0}, {{}}, {}, {juce::Colours::black}, {Analyzer::ColorMap::Heat},  {}}))
        {
            return;
        }
        auto& anlAcsr = mAccessor.getAccessors<AttrType::analyzers>().back();
        Analyzer::PropertyPanel panel(anlAcsr);
        panel.onAnalyse = [&]()
        {
            auto audioFormatReader = createAudioFormatReader(mAccessor, mAudioFormatManager, true);
            if(audioFormatReader == nullptr)
            {
                return;
            }
            Analyzer::performAnalysis(mAccessor.getAccessors<AttrType::analyzers>().back(), *audioFormatReader.get());
        };
        
        juce::DialogWindow::showModalDialog(juce::translate("Analyzer Properties"), &panel, this, findColour(juce::ResizableWindow::backgroundColourId, true), true, false, false);
    };
    
    addAndMakeVisible(mContainer);
    addAndMakeVisible(mAddButton);
    mAddButton.setTooltip(juce::translate("Add a new analysis..."));
    mAddButton.onClick = [&]()
    {
        juce::DialogWindow::showModalDialog(juce::translate("New Analyzer..."), &mPluginListTable, this, findColour(juce::ResizableWindow::backgroundColourId, true), true, false, false);
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

void Document::Section::resized()
{
    auto bounds = getLocalBounds();
    mAddButton.setBounds(bounds.removeFromBottom(20).withWidth(180).reduced(2));
    mContainer.setBounds(bounds);
}

void Document::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

ANALYSE_FILE_END
