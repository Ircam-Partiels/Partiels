#include "AnlDocumentSection.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Document::Section::Content::Content(Analyzer::Accessor& acsr, Zoom::Accessor& timeZoomAcsr, juce::StretchableLayoutManager& layoutManager)
: mAccessor(acsr)
, mTimeZoomAccessor(timeZoomAcsr)
, mLayoutManager(layoutManager)
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
        if(onThumbnailResized != nullptr)
        {
            onThumbnailResized(mLayoutManager.getItemCurrentPosition(3));
        }
        resized();
    };
    
    addAndMakeVisible(mThumbnail);
    addAndMakeVisible(mInstantRenderer);
    addAndMakeVisible(mResizerBar);
    addAndMakeVisible(mTimeRenderer);
    addAndMakeVisible(mRuler);
    addAndMakeVisible(mScrollbar);
    setSize(80, 100);
}

void Document::Section::Content::setTime(double time)
{
    mInstantRenderer.setTime(time);
}

void Document::Section::Content::resized()
{
    juce::Component* components[] =
    {
        &mThumbnail,
        &mInstantRenderer,
        &mRuler,
        &mResizerBar,
        &mTimeRenderer,
        &mScrollbar,
        &mDummy
    };
    mLayoutManager.layOutComponents(components, 7, 0, 0, getWidth(), getHeight(), false, true);
}

void Document::Section::Content::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::sectionColourId, true));
}

Document::Section::Section(Accessor& accessor, juce::AudioFormatManager const& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
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
                    auto container = std::make_unique<Container>(anlAcsrs[i], timeZoomAcsr, mLayoutManager);
                    anlWeakAssert(container != nullptr);
                    if(container != nullptr)
                    {
                        container->content.onThumbnailResized = [&](int size)
                        {
                            mAccessor.setAttr<AttrType::layoutHorizontal>(size, NotificationType::synchronous);
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
                        auto constexpr icon = juce::AlertWindow::AlertIconType::QuestionIcon;
                        auto const title = juce::translate("Remove Analysis?");
                        auto const message = juce::translate("Are you sure you want to rremove this analysis?");
                        if(juce::AlertWindow::showOkCancelBox(icon, title, message))
                        {
                            mAccessor.eraseAccessor<AttrType::analyzers>(i, NotificationType::synchronous);                            
                        }
                    };
                }
                
                layoutAcsr.setAttr<Layout::StrechableContainer::AttrType::sizes>(sizes, NotificationType::synchronous);
                
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContainer.setContent(i, &(mContents[i]->content), 100);
                }
                resized();
            }
                break;
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
                break;
            case AttrType::playheadPosition:
            {
                auto const time = acsr.getAttr<AttrType::playheadPosition>();
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContents[i]->content.setTime(time);
                }
            };
                break;
            case AttrType::timeZoom:
                break;
            case AttrType::layoutHorizontal:
            {
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContents[i]->content.resized();
                }
                resized();
            }
                break;
            case AttrType::layout:
                break;
        }
    };
    
    mLayoutManager.setItemLayout(0, 66.0, 66.0, 66.0);
    mLayoutManager.setItemLayout(1, 10.0, 200.0, 20.0);
    mLayoutManager.setItemLayout(2, 16.0, 16.0, 16.0);
    mLayoutManager.setItemLayout(3, 2.0, 2.0, 2.0);
    mLayoutManager.setItemLayout(4, 20.0, -1.0, -1.0);
    mLayoutManager.setItemLayout(5, 8.0, 8.0, 8.0);
    mLayoutManager.setItemLayout(6, 8.0, 8.0, 8.0);
    
    mZoomTimeRuler.onDoubleClick = [&]()
    {
        auto& acsr = mAccessor.getAccessor<AttrType::timeZoom>(0);
        acsr.setAttr<Zoom::AttrType::visibleRange>(acsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
    };
    
    mPlayhead.setInterceptsMouseClicks(false, false);
    
    setSize(480, 200);
    addAndMakeVisible(mZoomTimeRuler);
    addAndMakeVisible(mContainer);
    addAndMakeVisible(mZoomTimeScrollBar);
    addAndMakeVisible(mPlayhead);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

void Document::Section::resized()
{
    auto bounds = getLocalBounds();
    auto const left = mAccessor.getAttr<AttrType::layoutHorizontal>();
    auto const right = getWidth() - 16;
    mZoomTimeRuler.setBounds(bounds.removeFromTop(14).withLeft(left).withRight(right));
    mZoomTimeScrollBar.setBounds(bounds.removeFromBottom(8).withLeft(left).withRight(right));
    mContainer.setBounds(bounds);
    mPlayhead.setBounds(bounds.withLeft(left).withRight(right));
}

ANALYSE_FILE_END
