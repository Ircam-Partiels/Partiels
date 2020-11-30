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
    
    mResizerBar.onMoved = [&]()
    {
        resized();
        if(onThumbnailResized != nullptr)
        {
            onThumbnailResized(mLayoutManager.getItemCurrentPosition(1));
        }
    };
    
    mLayoutManager.setItemLayout(0, 100.0, 240.0, 240.0);
    mLayoutManager.setItemLayout(1, 2.0, 2.0, 2.0);
    mLayoutManager.setItemLayout(2, 20.0, -1.0, -1.0);
    mLayoutManager.setItemLayout(3, 16.0, 16.0,16.0);
    mLayoutManager.setItemLayout(4, 16.0, 16.0,16.0);
    
    addAndMakeVisible(mThumbnail);
    addAndMakeVisible(mResizerBar);
    addAndMakeVisible(mRenderer);
    addAndMakeVisible(mRuler);
    addAndMakeVisible(mScrollbar);
    setSize(80, 100);
}

void Document::Section::Content::setThumbnailSize(int size)
{
    mLayoutManager.setItemPosition(1, size);
    resized();
}

void Document::Section::Content::resized()
{
    juce::Component* components[] =
    {
        &mThumbnail,
        &mResizerBar,
        &mRenderer,
        &mRuler,
        &mScrollbar,
    };
    mLayoutManager.layOutComponents(components, 5, 0, 0, getWidth(), getHeight(), false, true);
}

void Document::Section::Content::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::sectionColourId, true));
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
                JUCE_COMPILER_WARNING("fix deletion and recursive approach")
                
                auto const& anlAcsrs = mAccessor.getAccessors<AttrType::analyzers>();
                auto& timeZoomAcsr = mAccessor.getAccessor<AttrType::timeZoom>(0);
                auto& layoutAcsr = mAccessor.getAccessor<AttrType::layout>(0);
                auto sizes = layoutAcsr.getAttr<Layout::StrechableContainer::AttrType::sizes>();
                
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContainer.setContent(i, nullptr, 100);
                }
                
                layoutAcsr.setAttr<Layout::StrechableContainer::AttrType::sizes>(sizes, NotificationType::synchronous);
                auto it = mContents.begin();
                while (it != mContents.end())
                {
                    if(std::none_of(anlAcsrs.cbegin(), anlAcsrs.cend(), [&](auto const anlAcsr)
                    {
                        return &((*it)->accessor) == &(anlAcsr.get());
                    }))
                    {
                        sizes.erase(sizes.begin() + std::distance(mContents.begin(), it));
                        it = mContents.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
//                mContents.erase(std::remove_if(mContents.begin(), mContents.end(), [&](auto const& content)
//                {
//                    return std::none_of(anlAcsrs.cbegin(), anlAcsrs.cend(), [&](auto const anlAcsr)
//                    {
//                        return &(content->accessor) == &(anlAcsr.get());
//                    });
//                }), mContents.end());
                
                
                for(size_t i = mContents.size(); i < anlAcsrs.size(); ++i)
                {
                    auto container = std::make_unique<Container>(anlAcsrs[i], timeZoomAcsr);
                    anlWeakAssert(container != nullptr);
                    if(container != nullptr)
                    {
                        container->content.onThumbnailResized = [&](int size)
                        {
                            mAccessor.setAttr<AttrType::layoutHorizontal>(size, NotificationType::synchronous);
                            for(size_t j = 0; j < mContents.size(); ++j)
                            {
                                mContents[j]->content.setThumbnailSize(size);
                            }
                        };
                        mContents.push_back(std::move(container));
                        if(i >= sizes.size())
                        {
                            sizes.push_back(100);
                        }
                    }
                }
                
                std::sort(mContents.begin(), mContents.end(), [&](auto const& lhs, auto const& rhs)
                {
                    auto lhsIt = std::find_if(anlAcsrs.cbegin(), anlAcsrs.cend(), [&](auto const anlAcsr)
                    {
                        return &(lhs->accessor) == &(anlAcsr.get());
                    });
                    auto rhsIt = std::find_if(anlAcsrs.cbegin(), anlAcsrs.cend(), [&](auto const anlAcsr)
                    {
                        return &(rhs->accessor) == &(anlAcsr.get());
                    });
                    return lhsIt < rhsIt;
                });
                
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContents[i]->content.onRemove = [this, i]()
                    {
                        mAccessor.eraseAccessor<AttrType::analyzers>(i, NotificationType::synchronous);
                    };
                }
                
                layoutAcsr.setAttr<Layout::StrechableContainer::AttrType::sizes>(sizes, NotificationType::synchronous);
                
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContainer.setContent(i, &(mContents[i]->content), 100);
                }
            }
                break;
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
            case AttrType::playheadPosition:
            case AttrType::timeZoom:
            case AttrType::layoutHorizontal:
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
