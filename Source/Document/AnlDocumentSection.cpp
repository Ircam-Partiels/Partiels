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
    
    mRuler.onDoubleClick = [&]()
    {
        auto& valueZoomAcsr = mAccessor.getAccessor<Analyzer::AttrType::zoom>(0);
        auto const range = valueZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        valueZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
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

Document::Section::Section(Accessor& accessor, juce::AudioFormatManager const& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
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
    
    addAndMakeVisible(mContainer);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

void Document::Section::resized()
{
    mContainer.setBounds(getLocalBounds());
}

void Document::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

ANALYSE_FILE_END
