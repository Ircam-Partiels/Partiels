#include "AnlTrackNavigator.h"
#include "../AnlTrackTools.h"
#include <AnlCursorsData.h>

ANALYSE_FILE_BEGIN

Track::Navigator::Navigator(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeRuler(timeZoomAccessor, Zoom::Ruler::Orientation::horizontal)
{
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
            case AttrType::extraThresholds:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::graphics:
            case AttrType::colours:
            case AttrType::font:
            case AttrType::lineWidth:
            case AttrType::unit:
            case AttrType::labelLayout:
            case AttrType::showInGroup:
            case AttrType::sendViaOsc:
            case AttrType::grid:
            case AttrType::hasPluginColourMap:
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
                break;
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::description:
            case AttrType::results:
            case AttrType::channelsLayout:
            {
                auto zoomAcsr = Tools::getVerticalZoomAccessor(mAccessor, false);
                if(!mVerticalZoomAcsr.has_value() || !zoomAcsr.has_value() || std::addressof(mVerticalZoomAcsr.value().get()) != std::addressof(zoomAcsr.value().get()))
                {
                    mVerticalRulers.clear();
                    mVerticalZoomAcsr = zoomAcsr;
                }
                if(!zoomAcsr.has_value())
                {
                    return;
                }
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                while(channelsLayout.size() < mVerticalRulers.size())
                {
                    mVerticalRulers.pop_back();
                }
                while(channelsLayout.size() > mVerticalRulers.size())
                {
                    auto ruler = std::make_unique<Zoom::Ruler>(zoomAcsr.value().get(), Zoom::Ruler::Orientation::vertical);
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
                        ruler->onDoubleClick = [this]([[maybe_unused]] juce::MouseEvent const& event)
                        {
                            auto const& valueGlobalRange = mVerticalZoomAcsr.value().get().getAttr<Zoom::AttrType::globalRange>();
                            mVerticalZoomAcsr.value().get().setAttr<Zoom::AttrType::visibleRange>(valueGlobalRange, NotificationType::synchronous);
                        };
                        addChildComponent(ruler.get());
                    }
                    mVerticalRulers.push_back(std::move(ruler));
                }
                resized();
                break;
            }
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Navigator::~Navigator()
{
    mAccessor.removeListener(mListener);
}

bool Track::Navigator::isPerformingAction() const
{
    return isEnabled() && mNavigationMode != NavigationMode::navigate;
}

void Track::Navigator::resized()
{
    auto const bounds = getLocalBounds();
    mTimeRuler.setBounds(bounds);
    for(auto& ruler : mVerticalRulers)
    {
        anlWeakAssert(ruler != nullptr);
        if(ruler != nullptr)
        {
            ruler->setVisible(false);
        }
    }
    if(!mVerticalZoomAcsr.has_value())
    {
        return;
    }
    auto const verticalRanges = Tools::getChannelVerticalRanges(mAccessor, bounds);
    for(auto const& verticalRange : verticalRanges)
    {
        anlWeakAssert(verticalRange.first < mVerticalRulers.size());
        if(verticalRange.first < mVerticalRulers.size())
        {
            auto& ruler = mVerticalRulers[verticalRange.first];
            if(ruler != nullptr)
            {
                ruler->setVisible(true);
                ruler->setBounds(bounds.withTop(verticalRange.second.getStart()).withBottom(verticalRange.second.getEnd()));
            }
        }
    }
}

void Track::Navigator::mouseMove(juce::MouseEvent const& event)
{
    updateInteraction(event.mods);
    updateMouseCursor(event.mods);
}

void Track::Navigator::mouseDown(juce::MouseEvent const& event)
{
    updateInteraction(event.mods);
}

void Track::Navigator::mouseUp(juce::MouseEvent const& event)
{
    updateMouseCursor(event.mods);
}

void Track::Navigator::modifierKeysChanged(juce::ModifierKeys const& modifiers)
{
    if(!isMouseButtonDownAnywhere())
    {
        updateInteraction(modifiers);
        updateMouseCursor(modifiers);
    }
}

void Track::Navigator::updateInteraction(juce::ModifierKeys const& modifiers)
{
    auto currentNavigation = Zoom::Ruler::getNavigationMode(modifiers);
    if(std::exchange(mNavigationMode, currentNavigation) != currentNavigation)
    {
        // The navigate mode is ignored
        if(mNavigationMode == NavigationMode::navigate)
        {
            removeMouseListener(std::addressof(mTimeRuler));
            for(auto& ruler : mVerticalRulers)
            {
                anlWeakAssert(ruler != nullptr);
                if(ruler != nullptr && ruler->isVisible())
                {
                    removeMouseListener(ruler.get());
                }
            }
        }
        else
        {
            addMouseListener(std::addressof(mTimeRuler), true);
            for(auto& ruler : mVerticalRulers)
            {
                anlWeakAssert(ruler != nullptr);
                if(ruler != nullptr && ruler->isVisible())
                {
                    addMouseListener(ruler.get(), true);
                }
            }
        }
        if(onModeUpdated != nullptr)
        {
            onModeUpdated();
        }
    }
}

void Track::Navigator::updateMouseCursor(juce::ModifierKeys const& modifiers)
{
    switch(Zoom::Ruler::getNavigationMode(modifiers))
    {
        case NavigationMode::zoom:
        {
            static auto const zoomImage = juce::ImageCache::getFromMemory(AnlCursorsData::selectionzoom_png, AnlCursorsData::selectionzoom_pngSize);
            static auto const zoomCursor = juce::MouseCursor(zoomImage, 8, 8, 2.0f);
            setMouseCursor(zoomCursor);
            break;
        }
        case NavigationMode::translate:
        {
            setMouseCursor({juce::MouseCursor::StandardCursorType::UpDownLeftRightResizeCursor});
            break;
        }
        case NavigationMode::select:
        {
            static auto const selectImage = juce::ImageCache::getFromMemory(AnlCursorsData::selectionrectangle_png, AnlCursorsData::selectionrectangle_pngSize);
            static auto const selectCursor = juce::MouseCursor(selectImage, 8, 8, 2.0f);
            setMouseCursor(selectCursor);
            break;
        }
        case NavigationMode::navigate:
            break;
    }
}

ANALYSE_FILE_END
