#include "AnlTrackRuler.h"
#include "AnlTrackRenderer.h"
#include <AnlCursorsData.h>

ANALYSE_FILE_BEGIN

Track::Ruler::Ruler(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::file:
            case AttrType::edit:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::grid:
            case AttrType::showInGroup:
                break;
            case AttrType::description:
            case AttrType::results:
            case AttrType::channelsLayout:
            {
                auto const FrameType = Tools::getFrameType(acsr);
                if(mFrameType != FrameType)
                {
                    mRulers.clear();
                    mFrameType = FrameType;
                }
                if(mFrameType == Track::FrameType::label)
                {
                    return;
                }
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                while(channelsLayout.size() < mRulers.size())
                {
                    mRulers.pop_back();
                }
                auto& zoomAcsr = mFrameType == Track::FrameType::value ? mAccessor.getAcsr<AcsrType::valueZoom>() : mAccessor.getAcsr<AcsrType::binZoom>();
                while(channelsLayout.size() > mRulers.size())
                {
                    auto ruler = std::make_unique<Zoom::Ruler>(zoomAcsr, Zoom::Ruler::Orientation::vertical);
                    anlWeakAssert(ruler != nullptr);
                    if(ruler != nullptr)
                    {
                        ruler->onMouseDown = [&, rulerPtr = ruler.get()](juce::MouseEvent const& event)
                        {
                            if(event.mods.isCtrlDown())
                            {
                                auto rangeEditor = mFrameType == Track::FrameType::value ? Tools::createValueRangeEditor(mAccessor) : Tools::createBinRangeEditor(mAccessor);
                                if(rangeEditor == nullptr)
                                {
                                    return false;
                                }
                                auto const point = juce::Desktop::getMousePosition();
                                auto const rulerBounds = rulerPtr->getScreenBounds();
                                auto const width = rulerBounds.getWidth();
                                auto const bounds = rulerBounds.withY(point.getY() - width / 2).withHeight(width);
                                juce::CallOutBox::launchAsynchronously(std::move(rangeEditor), bounds, nullptr).setArrowSize(0.0f);
                                return false;
                            }
                            return true;
                        };
                        ruler->onDoubleClick = [&]([[maybe_unused]] juce::MouseEvent const& event)
                        {
                            auto const& range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
                            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                        };
                        addChildComponent(ruler.get());
                    }
                    mRulers.push_back(std::move(ruler));
                }
                colourChanged();
                resized();
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Ruler::~Ruler()
{
    mAccessor.removeListener(mListener);
}

void Track::Ruler::resized()
{
    if(mFrameType == Track::FrameType::label)
    {
        return;
    }

    for(auto& ruler : mRulers)
    {
        anlWeakAssert(ruler != nullptr);
        if(ruler != nullptr)
        {
            ruler->setVisible(false);
        }
    }
    auto const bounds = getLocalBounds();
    auto const verticalRanges = Tools::getChannelVerticalRanges(mAccessor, getLocalBounds());
    for(auto const& verticalRange : verticalRanges)
    {
        anlWeakAssert(verticalRange.first < mRulers.size());
        if(verticalRange.first < mRulers.size())
        {
            auto& ruler = mRulers[verticalRange.first];
            if(ruler != nullptr)
            {
                ruler->setVisible(true);
                ruler->setBounds(bounds.withTop(verticalRange.second.getStart()).withBottom(verticalRange.second.getEnd()));
            }
        }
    }
}

void Track::Ruler::paint(juce::Graphics& g)
{
    Renderer::paintChannels(mAccessor, g, getLocalBounds(), findColour(Decorator::ColourIds::normalBorderColourId), [&](juce::Rectangle<int>, size_t)
                            {
                            });
}

void Track::Ruler::colourChanged()
{
    for(auto& ruler : mRulers)
    {
        anlWeakAssert(ruler != nullptr);
        if(ruler != nullptr)
        {
            ruler->setColour(Zoom::Ruler::ColourIds::gridColourId, findColour(Decorator::ColourIds::normalBorderColourId));
        }
    }
}

Track::ScrollBar::ScrollBar(Accessor& accessor)
: mAccessor(accessor)
{
    addChildComponent(mValueScrollBar);
    addChildComponent(mBinScrollBar);

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::file:
            case AttrType::edit:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::grid:
            case AttrType::focused:
            case AttrType::showInGroup:
                break;
            case AttrType::description:
            case AttrType::results:
            {
                auto const FrameType = Tools::getFrameType(acsr);
                mValueScrollBar.setVisible(FrameType == Track::FrameType::value);
                mBinScrollBar.setVisible(FrameType == Track::FrameType::vector);
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::ScrollBar::~ScrollBar()
{
    mAccessor.removeListener(mListener);
}

void Track::ScrollBar::resized()
{
    auto const bounds = getLocalBounds();
    mValueScrollBar.setBounds(bounds);
    mBinScrollBar.setBounds(bounds);
}

Track::SelectionBar::SelectionBar(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::file:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::grid:
            case AttrType::description:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::showInGroup:
                break;
            case AttrType::focused:
            {
                colourChanged();
            }
            break;
            case AttrType::channelsLayout:
            {
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                if(channelsLayout.size() < mSelectionBars.size())
                {
                    mSelectionBars.resize(channelsLayout.size());
                }
                while(channelsLayout.size() > mSelectionBars.size())
                {
                    auto bar = std::make_unique<Transport::SelectionBar>(mTransportAccessor, mTimeZoomAccessor);
                    addAndMakeVisible(bar.get());
                    mSelectionBars.push_back(std::move(bar));
                }
                for(auto& bar : mSelectionBars)
                {
                    anlWeakAssert(bar != nullptr);
                    if(bar != nullptr)
                    {
                        bar->setVisible(false);
                    }
                }
                colourChanged();
                resized();
            }
            break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::SelectionBar::~SelectionBar()
{
    mAccessor.removeListener(mListener);
}

void Track::SelectionBar::resized()
{
    auto const bounds = getLocalBounds();
    auto const verticalRanges = Tools::getChannelVerticalRanges(mAccessor, getLocalBounds());
    for(auto const& verticalRange : verticalRanges)
    {
        anlWeakAssert(verticalRange.first < mSelectionBars.size());
        if(verticalRange.first < mSelectionBars.size())
        {
            auto& bar = mSelectionBars[verticalRange.first];
            MiscWeakAssert(bar != nullptr);
            if(bar != nullptr)
            {
                bar->setVisible(true);
                bar->setBounds(bounds.withTop(verticalRange.second.getStart()).withBottom(verticalRange.second.getEnd()));
            }
        }
    }
}

void Track::SelectionBar::colourChanged()
{
    auto const onColour = getLookAndFeel().findColour(Transport::SelectionBar::thumbCoulourId);
    auto const offColour = onColour.withAlpha(onColour.getFloatAlpha() * 0.5f);
    auto const& focused = mAccessor.getAttr<AttrType::focused>();
    for(auto channel = 0_z; channel < mSelectionBars.size() && channel < focused.size(); ++channel)
    {
        auto& bar = mSelectionBars[channel];
        MiscWeakAssert(bar != nullptr);
        if(bar != nullptr)
        {
            auto const colour = focused.test(channel) ? onColour : offColour;
            bar->setColour(Transport::SelectionBar::thumbCoulourId, colour);
        }
    }
}

Track::NavigationBar::NavigationBar(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mTimeRuler(timeZoomAccessor, Zoom::Ruler::Orientation::horizontal)
, mSelectionBar(accessor, timeZoomAccessor, transportAccessor)
{
    addAndMakeVisible(mSelectionBar);
    mSelectionBar.addMouseListener(this, true);
    addAndMakeVisible(mTimeRuler);
    mTimeRuler.setInterceptsMouseClicks(false, false);
    mTimeRuler.setColour(Zoom::Ruler::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    mTimeRuler.setColour(Zoom::Ruler::ColourIds::gridColourId, juce::Colours::transparentBlack);
    mTimeRuler.onMouseDown = [&](juce::MouseEvent const& event)
    {
        return Zoom::Ruler::getNavigationMode(event.mods) != Zoom::Ruler::NavigationMode::navigate;
    };
    mTimeRuler.onDoubleClick = [&]([[maybe_unused]] juce::MouseEvent const& event)
    {
        auto const timeGlobalRange = timeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();
        timeZoomAccessor.setAttr<Zoom::AttrType::visibleRange>(timeGlobalRange, NotificationType::synchronous);
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::file:
            case AttrType::edit:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::unit:
            case AttrType::grid:
            case AttrType::showInGroup:
                break;
            case AttrType::description:
            case AttrType::results:
            case AttrType::channelsLayout:
            {
                auto const frameType = Tools::getFrameType(acsr);
                if(mFrameType != frameType)
                {
                    mRulers.clear();
                    mFrameType = frameType;
                }
                if(mFrameType == Track::FrameType::label)
                {
                    return;
                }
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                while(channelsLayout.size() < mRulers.size())
                {
                    mRulers.pop_back();
                }
                auto& zoomAcsr = mFrameType == Track::FrameType::value ? mAccessor.getAcsr<AcsrType::valueZoom>() : mAccessor.getAcsr<AcsrType::binZoom>();
                while(channelsLayout.size() > mRulers.size())
                {
                    auto ruler = std::make_unique<Zoom::Ruler>(zoomAcsr, Zoom::Ruler::Orientation::vertical);
                    anlWeakAssert(ruler != nullptr);
                    if(ruler != nullptr)
                    {
                        ruler->setInterceptsMouseClicks(false, false);
                        ruler->setColour(Zoom::Ruler::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
                        ruler->setColour(Zoom::Ruler::ColourIds::gridColourId, juce::Colours::transparentBlack);
                        ruler->onMouseDown = [&](juce::MouseEvent const& event)
                        {
                            return Zoom::Ruler::getNavigationMode(event.mods) != Zoom::Ruler::NavigationMode::navigate;
                        };
                        ruler->onDoubleClick = [&]([[maybe_unused]] juce::MouseEvent const& event)
                        {
                            auto const& valueGlobalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
                            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(valueGlobalRange, NotificationType::synchronous);
                        };
                        addChildComponent(ruler.get());
                    }
                    mRulers.push_back(std::move(ruler));
                }
                colourChanged();
                resized();
            }
            break;
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::NavigationBar::~NavigationBar()
{
    mAccessor.removeListener(mListener);
}

void Track::NavigationBar::resized()
{
    auto const bounds = getLocalBounds();
    mSelectionBar.setBounds(bounds);
    mTimeRuler.setBounds(bounds);
    for(auto& ruler : mRulers)
    {
        anlWeakAssert(ruler != nullptr);
        if(ruler != nullptr)
        {
            ruler->setVisible(false);
        }
    }
    auto const verticalRanges = Tools::getChannelVerticalRanges(mAccessor, bounds);
    for(auto const& verticalRange : verticalRanges)
    {
        anlWeakAssert(verticalRange.first < mRulers.size());
        if(verticalRange.first < mRulers.size())
        {
            auto& ruler = mRulers[verticalRange.first];
            if(ruler != nullptr)
            {
                ruler->setVisible(true);
                ruler->setBounds(bounds.withTop(verticalRange.second.getStart()).withBottom(verticalRange.second.getEnd()));
            }
        }
    }
}

void Track::NavigationBar::mouseMove(juce::MouseEvent const& event)
{
    updateInteraction(event.mods);
    updateMouseCursor(event.mods);
}

void Track::NavigationBar::mouseDown(juce::MouseEvent const& event)
{
    updateInteraction(event.mods);
}

void Track::NavigationBar::mouseUp(juce::MouseEvent const& event)
{
    updateMouseCursor(event.mods);
}

void Track::NavigationBar::modifierKeysChanged(juce::ModifierKeys const& modifiers)
{
    if(!isMouseButtonDownAnywhere())
    {
        updateInteraction(modifiers);
        updateMouseCursor(modifiers);
    }
}

void Track::NavigationBar::updateInteraction(juce::ModifierKeys const& modifiers)
{
    using NavigationMode = Zoom::Ruler::NavigationMode;
    if(Zoom::Ruler::getNavigationMode(modifiers) == NavigationMode::navigate)
    {
        removeMouseListener(std::addressof(mTimeRuler));
        for(auto& ruler : mRulers)
        {
            anlWeakAssert(ruler != nullptr);
            if(ruler != nullptr && ruler->isVisible())
            {
                removeMouseListener(ruler.get());
            }
        }
        mSelectionBar.setInterceptsMouseClicks(true, true);
    }
    else
    {
        addMouseListener(std::addressof(mTimeRuler), true);
        for(auto& ruler : mRulers)
        {
            anlWeakAssert(ruler != nullptr);
            if(ruler != nullptr && ruler->isVisible())
            {
                addMouseListener(ruler.get(), true);
            }
        }
        mSelectionBar.setInterceptsMouseClicks(false, false);
    }
}

void Track::NavigationBar::updateMouseCursor(juce::ModifierKeys const& modifiers)
{
    using NavigationMode = Zoom::Ruler::NavigationMode;
    switch(Zoom::Ruler::getNavigationMode(modifiers))
    {
        case NavigationMode::zoom:
        {
            static auto const image = juce::ImageCache::getFromMemory(AnlCursorsData::selectionzoom_png, AnlCursorsData::selectionzoom_pngSize);
            static auto const cursor = juce::MouseCursor(image, 8, 8, 2.0f);
            setMouseCursor(cursor);
            break;
        }
        case NavigationMode::translate:
        {
            setMouseCursor({juce::MouseCursor::StandardCursorType::UpDownLeftRightResizeCursor});
            break;
        }
        case NavigationMode::select:
        {
            static auto const image = juce::ImageCache::getFromMemory(AnlCursorsData::selectionrectangle_png, AnlCursorsData::selectionrectangle_pngSize);
            static auto const cursor = juce::MouseCursor(image, 8, 8, 2.0f);
            setMouseCursor(cursor);
            break;
        }
        case NavigationMode::navigate:
            break;
    }
}

ANALYSE_FILE_END
