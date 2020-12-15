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
        auto& valueZoomAcsr = mAccessor.getAccessor<Analyzer::AcsrType::zoom>(0);
        auto const range = valueZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        valueZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };
    
    mResizerBar.onMoved = [&](int position)
    {
        if(onThumbnailResized != nullptr)
        {
            onThumbnailResized(position + 4);
        }
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

void Document::Section::Content::setThumbnailSize(int size)
{
    mResizerBar.setTopLeftPosition(size - 4, 0);
    resized();
}

void Document::Section::Content::resized()
{
    auto bounds = getLocalBounds();
    auto const resizerPos = mResizerBar.getX();
    mScrollbar.setBounds(bounds.removeFromRight(8));
    mRuler.setBounds(bounds.removeFromRight(16));
    
    mThumbnail.setBounds(bounds.removeFromLeft(24));
    mInstantRenderer.setBounds(bounds.removeFromLeft(resizerPos - bounds.getX()));
    mResizerBar.setBounds(bounds.removeFromLeft(4));
    mTimeRenderer.setBounds(bounds);
}

void Document::Section::Content::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::sectionColourId, true));
}

Document::Section::Section(Accessor& accessor)
: mAccessor(accessor)
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
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
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
            case AttrType::layoutHorizontal:
            {
                resized();
                auto const pos = acsr.getAttr<AttrType::layoutHorizontal>();
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContents[i]->content.setThumbnailSize(pos);
                }
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [&](Accessor const& acsr, AcsrType attribute, size_t index)
    {
        switch(attribute)
        {
            case AcsrType::analyzers:
            {
                auto& anlAcsr = mAccessor.getAccessor<AcsrType::analyzers>(index);
                auto& timeZoomAcsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
                
                auto container = std::make_unique<Container>(anlAcsr, timeZoomAcsr);
                anlStrongAssert(container != nullptr);
                if(container != nullptr)
                {
                    container->content.onThumbnailResized = [&](int size)
                    {
                        mAccessor.setAttr<AttrType::layoutHorizontal>(size, NotificationType::synchronous);
                    };
                    
                    container->content.onRemove = [this, ptr = container.get()]()
                    {
                        auto const anlAcsrs = mAccessor.getAccessors<AcsrType::analyzers>();
                        for(size_t i = 0; i < anlAcsrs.size(); ++i)
                        {
                            if(&(anlAcsrs[i].get()) == &ptr->accessor)
                            {
                                auto constexpr icon = juce::AlertWindow::AlertIconType::QuestionIcon;
                                auto const title = juce::translate("Remove Analysis");
                                auto const message = juce::translate("Are you sure you want to remove the \"ANLNAME\" analysis from the project? If you edited the results of the analysis, the changes will be lost!").replace("ANLNAME", anlAcsrs[i].get().getAttr<Analyzer::AttrType::name>());
                                if(juce::AlertWindow::showOkCancelBox(icon, title, message))
                                {
                                    mAccessor.eraseAccessor<AcsrType::analyzers>(i, NotificationType::synchronous);
                                }
                                
                                return;
                            }
                        }
                    };
                }
                mContents.insert(mContents.begin() + static_cast<long>(index), std::move(container));
                

                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContainer.setContent(i, &(mContents[i]->content), 100);
                }
                resized();
                auto const pos = acsr.getAttr<AttrType::layoutHorizontal>();
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContents[i]->content.setThumbnailSize(pos);
                }
            }
                break;
                
            default:
                break;
        }
        
    };
    
    mListener.onAccessorErased = [&](Accessor const& acsr, AcsrType attribute, size_t index)
    {
        switch(attribute)
        {
            case AcsrType::analyzers:
            {
                mContents.erase(mContents.begin() + static_cast<long>(index));
                for(size_t i = 0; i < mContents.size(); ++i)
                {
                    mContainer.setContent(i, &(mContents[i]->content), 100);
                }
                resized();
            }
                break;
                
            default:
                break;
        }
        
    };
    
    mZoomTimeRuler.onDoubleClick = [&]()
    {
        auto& acsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
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
    auto const left = mAccessor.getAttr<AttrType::layoutHorizontal>() + 2;
    auto const right = getWidth() - 24;
    mZoomTimeRuler.setBounds(bounds.removeFromTop(14).withLeft(left).withRight(right));
    mZoomTimeScrollBar.setBounds(bounds.removeFromBottom(8).withLeft(left).withRight(right));
    mContainer.setBounds(bounds);
    mPlayhead.setBounds(bounds.withLeft(left).withRight(right));
}

void Document::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    auto bounds = getLocalBounds();
    g.setColour(mZoomTimeRuler.findColour(Zoom::Ruler::backgroundColourId));
    g.fillRect(bounds.removeFromTop(mZoomTimeRuler.getHeight()));
}

ANALYSE_FILE_END
