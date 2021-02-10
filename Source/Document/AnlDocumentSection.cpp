#include "AnlDocumentSection.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Document::Section::Section(Accessor& accessor)
: mAccessor(accessor)
{
    mZoomTimeRuler.setPrimaryTickInterval(0);
    mZoomTimeRuler.setTickReferenceValue(0.0);
    mZoomTimeRuler.setTickPowerInterval(10.0, 2.0);
    mZoomTimeRuler.setMaximumStringWidth(70.0);
    mZoomTimeRuler.setValueAsStringMethod([](double value)
    {
        return Format::secondsToString(value, {":", ":", ":", ""});
    });
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::isLooping:
            case AttrType::gain:
            case AttrType::isPlaybackStarted:
            case AttrType::playheadPosition:
                break;
            case AttrType::layoutHorizontal:
            {
                resized();
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::layout:
                break;
            case AcsrType::analyzers:
            {
                auto& anlAcsr = mAccessor.getAccessor<AcsrType::analyzers>(index);
                auto& timeZoomAcsr = mAccessor.getAccessor<AcsrType::timeZoom>(0);
                
                auto container = std::make_unique<Container>(anlAcsr, timeZoomAcsr, mResizerBar);
                anlStrongAssert(container != nullptr);
                if(container != nullptr)
                {
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
            }
                break;
                
            default:
                break;
        }
        
    };
    
    mListener.onAccessorErased = [&](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::layout:
                break;
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
    
    mResizerBar.onMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::layoutHorizontal>(size, NotificationType::synchronous);
    };
    
    mPlayhead.setInterceptsMouseClicks(false, false);
    
    setSize(480, 200);
    addAndMakeVisible(mZoomTimeRuler);
    addAndMakeVisible(mContainer);
    addAndMakeVisible(mZoomTimeScrollBar);
    addAndMakeVisible(mPlayhead);
    addAndMakeVisible(mResizerBar);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Section::~Section()
{
    mContents.clear();
    mAccessor.removeListener(mListener);
}

void Document::Section::resized()
{
    auto bounds = getLocalBounds();
    auto const left = mAccessor.getAttr<AttrType::layoutHorizontal>() + 2;
    auto const right = getWidth() - 32;
    mZoomTimeRuler.setBounds(bounds.removeFromTop(14).withLeft(left).withRight(right));
    mZoomTimeScrollBar.setBounds(bounds.removeFromBottom(8).withLeft(left).withRight(right));
    mResizerBar.setBounds(left - 2, bounds.getY(), 2, bounds.getHeight());
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
