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
            case AttrType::graphicsSettings:
            case AttrType::showInGroup:
            case AttrType::oscIdentifier:
            case AttrType::sendViaOsc:
            case AttrType::grid:
            case AttrType::hasPluginColourMap:
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
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
                if(mFrameType.value_or(FrameType::label) == FrameType::label)
                {
                    return;
                }
                auto const channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                while(channelsLayout.size() < mRulers.size())
                {
                    mRulers.pop_back();
                }
                auto& zoomAcsr = mFrameType.value() == Track::FrameType::value ? mAccessor.getAcsr<AcsrType::valueZoom>() : mAccessor.getAcsr<AcsrType::binZoom>();
                while(channelsLayout.size() > mRulers.size())
                {
                    auto ruler = std::make_unique<Zoom::Ruler>(zoomAcsr, Zoom::Ruler::Orientation::vertical);
                    anlWeakAssert(ruler != nullptr);
                    if(ruler != nullptr)
                    {
                        ruler->onMouseDown = [&, rulerPtr = ruler.get()](juce::MouseEvent const& event)
                        {
                            if(event.mods.isPopupMenu())
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
                                auto& box = juce::CallOutBox::launchAsynchronously(std::move(rangeEditor), bounds, nullptr);
                                box.setLookAndFeel(std::addressof(getLookAndFeel()));
                                box.setArrowSize(0.0f);
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
    Renderer::paintChannels(g, getLocalBounds(), mAccessor.getAttr<AttrType::channelsLayout>(), findColour(Decorator::ColourIds::normalBorderColourId), [&](juce::Rectangle<int>, size_t)
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
            case AttrType::graphicsSettings:
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
            case AttrType::oscIdentifier:
            case AttrType::sendViaOsc:
            case AttrType::hasPluginColourMap:
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
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

ANALYSE_FILE_END
