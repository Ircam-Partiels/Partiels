#include "AnlDocumentSection.h"

ANALYSE_FILE_BEGIN

Document::Section::GroupContainer::GroupContainer(Accessor& accessor)
: mAccessor(accessor)
{
    auto updateComponents = [&]()
    {
        std::vector<ConcertinaTable::ComponentRef> components;
        components.reserve(mSections.size());
        auto const& layout = mAccessor.getAttr<AttrType::layout>();
        for(auto const& identifier : layout)
        {
            auto it = std::find_if(mSections.cbegin(), mSections.cend(), [&](auto const& section)
            {
                return section->getIdentifier() == identifier;
            });
            if(it != mSections.cend())
            {
                components.push_back(*it->get());
            }
        }
        mDraggableTable.setComponents(components);
        resized();
    };
    
    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
                break;
            case AttrType::layoutHorizontal:
            {
                resized();
            }
            case AttrType::layoutVertical:
            {
                resized();
            }
                break;
            case AttrType::layout:
            {
                updateComponents();
            }
                break;
            case AttrType::expanded:
            {
                mConcertinaTable.setOpen(mAccessor.getAttr<AttrType::expanded>(), true);
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [=, this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
            case AcsrType::tracks:
            {
                auto& trackAcsr = mAccessor.getAcsr<AcsrType::tracks>(index);
                auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
                auto& transport = mAccessor.getAcsr<AcsrType::transport>();
                
                auto newSection = std::make_unique<Track::Section>(trackAcsr, timeZoomAcsr, transport, mResizerBar);
                anlStrongAssert(newSection != nullptr);
                if(newSection != nullptr)
                {
                    newSection->onRemove = [this, ptr = newSection.get()]()
                    {
                        if(onRemoveTrack != nullptr)
                        {
                            onRemoveTrack(ptr->getIdentifier());
                        }
                    };
                }
                mSections.insert(mSections.begin() + static_cast<long>(index), std::move(newSection));
                updateComponents();
            }
                break;
        }
    };
    
    mListener.onAccessorErased = [=, this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
            case AcsrType::tracks:
            {
                mSections.erase(mSections.begin() + static_cast<long>(index));
                updateComponents();
            }
                break;
        }
    };
    
    mResizerBar.onMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::layoutHorizontal>(size, NotificationType::synchronous);
    };
    
    mDraggableTable.onComponentDragged = [&](size_t previousIndex, size_t nextIndex)
    {
        auto layout = mAccessor.getAttr<AttrType::layout>();
        auto const identifier = layout[previousIndex];
        std::erase(layout, identifier);
        layout.insert(layout.begin() + static_cast<long>(nextIndex), identifier);
        
        auto const trackAcsrs = mAccessor.getAcsrs<AcsrType::tracks>();
        std::erase_if(layout, [&](auto const& trackIdentifier)
        {
            return std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
            {
                return trackAcsr.get().template getAttr<Track::AttrType::identifier>() == trackIdentifier;
            });
        });
        anlWeakAssert(layout.size() == trackAcsrs.size());
        mAccessor.setAttr<AttrType::layout>(layout, NotificationType::synchronous);
    };
    
    mBoundsListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        auto const height = mGroupSection.getHeight() + mConcertinaTable.getHeight();
        setSize(getWidth(), height);
    };
    mBoundsListener.attachTo(mGroupSection);
    mBoundsListener.attachTo(mConcertinaTable);
    
    mConcertinaTable.setComponents({mDraggableTable});
    mConcertinaTable.setOpen(mAccessor.getAttr<AttrType::expanded>(), false);
    
    addAndMakeVisible(mGroupSection);
    addAndMakeVisible(mConcertinaTable);
    addAndMakeVisible(mResizerBar);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Section::GroupContainer::~GroupContainer()
{
    mBoundsListener.detachFrom(mConcertinaTable);
    mAccessor.removeListener(mListener);
}

void Document::Section::GroupContainer::resized()
{
    auto const left = mAccessor.getAttr<AttrType::layoutHorizontal>();
    mResizerBar.setBounds(left, 0, 2, getHeight());
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mGroupSection.setBounds(bounds.removeFromTop(mGroupSection.getHeight()));
    mConcertinaTable.setBounds(bounds.removeFromTop(mConcertinaTable.getHeight()));
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
        return Format::secondsToString(value, {":", ":", ":", ""});
    });
    
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
                break;
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            case AttrType::expanded:
            {
                resized();
            }
                break;
            case AttrType::layout:
                break;
        }
    };
    
    auto accessorChanged = [this](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
            case AcsrType::tracks:
            {
                auto const numTracks = acsr.getNumAcsr<AcsrType::tracks>();
                mZoomTimeRuler.setVisible(numTracks > 0);
                mViewport.setVisible(numTracks > 0);
                mZoomTimeScrollBar.setVisible(numTracks > 0);
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = accessorChanged;
    mListener.onAccessorErased = accessorChanged;
    
    mZoomTimeRuler.onDoubleClick = [&]()
    {
        auto& acsr = mAccessor.getAcsr<AcsrType::timeZoom>();
        acsr.setAttr<Zoom::AttrType::visibleRange>(acsr.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
    };
    
    mGroupContainer.onRemoveTrack = [&](juce::String const& identifier)
    {
        if(onRemoveTrack != nullptr)
        {
            onRemoveTrack(identifier);
        }
    };
    
    mViewport.setViewedComponent(&mGroupContainer, false);
    mViewport.setScrollBarsShown(true, false, false, false);
    setSize(480, 200);
    addAndMakeVisible(mZoomTimeRuler);
    addAndMakeVisible(mViewport);
    addAndMakeVisible(mZoomTimeScrollBar);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

void Document::Section::resized()
{
    auto const scrollbarWidth = mViewport.getScrollBarThickness();
    auto bounds = getLocalBounds().withTrimmedRight(scrollbarWidth);
    auto const width = bounds.getWidth();
    auto const left = mAccessor.getAttr<AttrType::layoutHorizontal>() + 2;
    auto const right = width - 32;
    
    mZoomTimeRuler.setBounds(bounds.removeFromTop(14).withLeft(left + 1).withRight(right - 1));
    mZoomTimeScrollBar.setBounds(bounds.removeFromBottom(8).withLeft(left + 1).withRight(right - 1));
    bounds.removeFromTop(14);
    mGroupContainer.setBounds(0, 0, width, mGroupContainer.getHeight());
    mViewport.setBounds(bounds.withTrimmedRight(-scrollbarWidth));
}

void Document::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    auto bounds = getLocalBounds();
    g.setColour(mZoomTimeRuler.findColour(Zoom::Ruler::backgroundColourId));
    g.fillRect(bounds.removeFromTop(mZoomTimeRuler.getHeight()));
}

void Document::Section::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    juce::ignoreUnused(event);
    auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto const visibleRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const offset = static_cast<double>(wheel.deltaX) * visibleRange.getLength();
    timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
    
    auto viewportPosition = mViewport.getViewPosition();
    viewportPosition.y += static_cast<int>(std::round(wheel.deltaY * 14.0f * 16.0f));
    mViewport.setViewPosition(viewportPosition);
}

void Document::Section::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    juce::ignoreUnused(event);
    auto& timeZoomAcsr = mAccessor.getAcsr<AcsrType::timeZoom>();
    auto const globalRange = timeZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    auto const amount = static_cast<double>(1.0f - magnifyAmount) / 5.0 * globalRange.getLength();
    auto const visibleRange = timeZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
    auto const anchor = std::get<1>(timeZoomAcsr.getAttr<Zoom::AttrType::anchor>());

    auto const amountLeft = (anchor - visibleRange.getStart()) / visibleRange.getEnd() * amount;
    auto const amountRight = (visibleRange.getEnd() - anchor) / visibleRange.getEnd() * amount;
    
    auto const minDistance = timeZoomAcsr.getAttr<Zoom::AttrType::minimumLength>() / 2.0;
    auto const start = std::min(anchor - minDistance, visibleRange.getStart() - amountLeft);
    auto const end = std::max(anchor + minDistance, visibleRange.getEnd() + amountRight);
    timeZoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{start, end}, NotificationType::synchronous);
}

ANALYSE_FILE_END
