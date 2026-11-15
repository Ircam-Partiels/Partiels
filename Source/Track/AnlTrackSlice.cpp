#include "AnlTrackSlice.h"
#include "AnlTrackRenderer.h"
#include "AnlTrackTools.h"
#include <AnlCursorsData.h>

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace
    {
        //! @brief Converts the bin indices of the vectors to the values of the bin zoom and vice versa
        //! @details If the bin zoom is expressed in hertz, the bins are spread from 0 to nyquist inclusive and the
        //! zoom values are expressed in hertz (or in scaled midi values if the log scale is enabled). Otherwise, the
        //! zoom values are expressed in bin indices and each bin is represented at its center.
        class BinScale
        {
        public:
            BinScale(Accessor const& accessor, size_t numBins)
            : mNumBins(numBins)
            , mNyquist(accessor.getAttr<AttrType::sampleRate>() / 2.0)
            , mMidiMax(std::max(Tools::getMidiFromHertz(mNyquist), 1.0))
            , mIsHertz(Tools::hasVerticalZoomInHertz(accessor) && mNyquist > 0.0 && numBins > 1_z)
            , mIsLog(mIsHertz && accessor.getAttr<AttrType::zoomLogScale>())
            {
            }

            bool isHertz() const noexcept
            {
                return mIsHertz;
            }

            double getHertz(size_t index) const noexcept
            {
                return mIsHertz ? static_cast<double>(index) / static_cast<double>(mNumBins - 1_z) * mNyquist : 0.0;
            }

            double getZoomValue(size_t index) const noexcept
            {
                if(!mIsHertz)
                {
                    return static_cast<double>(index) + 0.5;
                }
                auto const hertz = getHertz(index);
                return mIsLog ? Tools::getMidiFromHertz(hertz) * mNyquist / mMidiMax : hertz;
            }

            std::optional<size_t> getIndex(double zoomValue) const noexcept
            {
                if(mNumBins == 0_z)
                {
                    return {};
                }
                if(!mIsHertz)
                {
                    if(zoomValue < 0.0 || zoomValue >= static_cast<double>(mNumBins))
                    {
                        return {};
                    }
                    return static_cast<size_t>(std::floor(zoomValue));
                }
                auto const hertz = mIsLog ? Tools::getHertzFromMidi(zoomValue * mMidiMax / mNyquist) : zoomValue;
                auto const index = std::round(hertz / mNyquist * static_cast<double>(mNumBins - 1_z));
                return static_cast<size_t>(std::clamp(index, 0.0, static_cast<double>(mNumBins - 1_z)));
            }

        private:
            size_t const mNumBins;
            double const mNyquist;
            double const mMidiMax;
            bool const mIsHertz;
            bool const mIsLog;
        };

        //! @brief Gets the scale ratio of the values of the points if the log scale is enabled
        std::optional<double> getPointLogScaleRatio(Accessor const& accessor)
        {
            if(!accessor.getAttr<AttrType::zoomLogScale>() || !Tools::hasVerticalZoomInHertz(accessor))
            {
                return {};
            }
            auto const nyquist = accessor.getAttr<AttrType::sampleRate>() / 2.0;
            return nyquist / std::max(Tools::getMidiFromHertz(nyquist), 1.0);
        }

        float valueToHorizontalPixel(double value, juce::Range<double> const& valueRange, juce::Rectangle<float> const& bounds)
        {
            return static_cast<float>((value - valueRange.getStart()) / valueRange.getLength()) * bounds.getWidth() + bounds.getX();
        }

        double horizontalPixelToValue(int position, juce::Range<double> const& valueRange, juce::Rectangle<int> const& bounds)
        {
            return static_cast<double>(position - bounds.getX()) / static_cast<double>(bounds.getWidth()) * valueRange.getLength() + valueRange.getStart();
        }

        Results::Column const* getColumn(Results::Columns const& channelResults, double time)
        {
            auto const it = std::upper_bound(channelResults.cbegin(), channelResults.cend(), time, Result::upper_cmp<Results::Column>);
            return it == channelResults.cbegin() ? nullptr : std::addressof(*std::prev(it));
        }

        //! @brief Creates a vertical gradient equivalent to the colour map used to render the images of the vectors
        //! @details The bottom of the bounds corresponds to the start of the colour map and the top to its end.
        juce::ColourGradient createColourMapGradient(ColourMap const colourMap, juce::Rectangle<float> const& bounds)
        {
            juce::ColourGradient gradient;
            gradient.isRadial = false;
            gradient.point1 = bounds.getBottomLeft();
            gradient.point2 = bounds.getTopLeft();
            auto constexpr numSteps = 64_z;
            for(auto step = 0_z; step <= numSteps; ++step)
            {
                auto const ratio = static_cast<double>(step) / static_cast<double>(numSteps);
                auto const colour = tinycolormap::GetColor(ratio, colourMap);
                gradient.addColour(ratio, juce::Colour::fromFloatRGBA(static_cast<float>(colour.r()), static_cast<float>(colour.g()), static_cast<float>(colour.b()), 1.0f));
            }
            return gradient;
        }

        void fillPath(juce::Graphics& g, juce::Path const& path, ColourSet const& colours, juce::Point<float> const& shadowOffset)
        {
            if(!colours.shadow.isTransparent())
            {
                g.setColour(colours.shadow.withMultipliedAlpha(0.5f));
                g.fillPath(path, juce::AffineTransform::translation(shadowOffset * 2.0f));
                g.setColour(colours.shadow.withMultipliedAlpha(0.75f));
                g.fillPath(path, juce::AffineTransform::translation(shadowOffset));
            }
            g.setColour(colours.foreground);
            g.fillPath(path);
        }
    } // namespace
} // namespace Track

Track::Slice::WindowContainer::WindowContainer(Slice& slice)
: FloatingWindowContainer(juce::translate("Instant Results"), slice)
, mSlice(slice)
{
    mFloatingWindow.setResizable(true, false);
    mBoundsConstrainer.setMinimumWidth(160);
    mBoundsConstrainer.setMinimumHeight(100);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
            {
                mFloatingWindow.setName(juce::translate("ANLNAME INSTANT RESULTS").replace("ANLNAME", acsr.getAttr<AttrType::name>().toUpperCase()));
            }
            break;
            case AttrType::identifier:
            case AttrType::file:
            case AttrType::fileDescription:
            case AttrType::voiceLink:
            case AttrType::voiceCounts:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::key:
            case AttrType::inputs:
            case AttrType::description:
            case AttrType::state:
            case AttrType::height:
            case AttrType::graphicsSettings:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
            case AttrType::oscIdentifier:
            case AttrType::sendViaOsc:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::useInputResultsExtraThresholds:
            case AttrType::extraThresholds:
            case AttrType::graphics:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::focused:
            case AttrType::grid:
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
                break;
        }
    };
    mSlice.mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Slice::WindowContainer::~WindowContainer()
{
    mSlice.mAccessor.removeListener(mListener);
}

Track::Slice::Slice(Accessor& accessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mPlot(accessor, transportAccessor)
{
    addAndMakeVisible(mFrame);
    setSize(560, 220);

    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
            case AttrType::inputs:
            case AttrType::useInputResultsExtraThresholds:
            case AttrType::file:
            case AttrType::fileDescription:
            case AttrType::voiceLink:
            case AttrType::voiceCounts:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::focused:
            case AttrType::showInGroup:
            case AttrType::oscIdentifier:
            case AttrType::sendViaOsc:
            case AttrType::graphics:
            case AttrType::grid:
            case AttrType::edit:
            case AttrType::graphicsSettings:
            case AttrType::channelsLayout:
            case AttrType::zoomLogScale:
            case AttrType::sampleRate:
                break;
            case AttrType::description:
            case AttrType::results:
            {
                mFrame.setTracks(std::addressof(mAccessor), std::addressof(mAccessor));
            }
            break;
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Slice::~Slice()
{
    mAccessor.removeListener(mListener);
}

void Track::Slice::resized()
{
    mFrame.setBounds(getLocalBounds());
}

Track::Slice::Plot::Plot(Accessor& accessor, Transport::Accessor& transportAccessor)
: mAccessor(accessor)
, mTransportAccessor(transportAccessor)
{
    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::key:
            case AttrType::inputs:
            case AttrType::useInputResultsExtraThresholds:
            case AttrType::file:
            case AttrType::fileDescription:
            case AttrType::voiceLink:
            case AttrType::voiceCounts:
            case AttrType::name:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::focused:
            case AttrType::showInGroup:
            case AttrType::oscIdentifier:
            case AttrType::sendViaOsc:
            case AttrType::graphics:
                break;
            case AttrType::description:
            case AttrType::grid:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::graphicsSettings:
            case AttrType::channelsLayout:
            case AttrType::zoomLogScale:
            case AttrType::sampleRate:
            {
                repaint();
                if(isShowing())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
        }
    };

    mZoomListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
            case Zoom::AttrType::visibleRange:
            {
                repaint();
                if(isShowing())
                {
                    updateTooltip(getMouseXYRelative());
                }
            }
            break;
        }
    };

    mGridListener.onAttrChanged = [this]([[maybe_unused]] Zoom::Grid::Accessor const& acsr, [[maybe_unused]] Zoom::Grid::AttrType attribute)
    {
        repaint();
    };

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& acsr, Transport::AttrType attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::startPlayhead:
            case Transport::AttrType::runningPlayhead:
            {
                auto const isPlaying = acsr.getAttr<Transport::AttrType::playback>();
                if(isPlaying == (attribute == Transport::AttrType::runningPlayhead))
                {
                    repaint();
                    if(isShowing())
                    {
                        updateTooltip(getMouseXYRelative());
                    }
                }
            }
            break;
            case Transport::AttrType::playback:
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::selection:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().addListener(mGridListener, NotificationType::synchronous);
    mTransportAccessor.addListener(mTransportListener, NotificationType::synchronous);
}

Track::Slice::Plot::~Plot()
{
    mTransportAccessor.removeListener(mTransportListener);
    mAccessor.getAcsr<AcsrType::binZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().getAcsr<Zoom::AcsrType::grid>().removeListener(mGridListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Slice::Plot::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::graphicsSettings>().colours.background);
    auto const colour = findColour(Decorator::ColourIds::normalBorderColourId);
    Slice::paint(mAccessor, getTime(mTransportAccessor), g, getLocalBounds(), colour, colour);
}

void Track::Slice::Plot::mouseMove(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
}

void Track::Slice::Plot::mouseEnter(juce::MouseEvent const& event)
{
    updateTooltip(getLocalPoint(event.eventComponent, juce::Point<int>{event.x, event.y}));
}

void Track::Slice::Plot::mouseExit([[maybe_unused]] juce::MouseEvent const& event)
{
    setTooltip("");
}

void Track::Slice::Plot::updateTooltip(juce::Point<int> const& pt)
{
    if(!isSupported(mAccessor) || !getLocalBounds().contains(pt))
    {
        setTooltip("");
        return;
    }
    auto const time = getTime(mTransportAccessor);
    juce::StringArray lines;
    lines.add(juce::translate("Time: TIMESTR").replace("TIMESTR", Format::secondsToString(time)));
    lines.addArray(Slice::getTooltip(mAccessor, getLocalBounds(), pt, time));
    setTooltip(lines.joinIntoString("\n"));
}

Track::Slice::Navigator::Navigator(juce::Component& content)
: mContent(content)
{
    setInterceptsMouseClicks(false, false);
    addChildComponent(mScroller);
    mContent.addMouseListener(this, true);
}

Track::Slice::Navigator::~Navigator()
{
    setRulersListening(false);
    mContent.removeMouseListener(this);
}

void Track::Slice::Navigator::setZoomAccessors(Zoom::Accessor* horizontalZoomAccessor, Zoom::Accessor* verticalZoomAccessor)
{
    if(horizontalZoomAccessor == mHorizontalZoomAccessor && verticalZoomAccessor == mVerticalZoomAccessor)
    {
        return;
    }
    // The rulers must not listen to the content anymore before being deleted
    setRulersListening(false);
    mHorizontalZoomAccessor = horizontalZoomAccessor;
    mVerticalZoomAccessor = verticalZoomAccessor;
    mHorizontalRuler = mHorizontalZoomAccessor != nullptr ? createRuler(*mHorizontalZoomAccessor, Zoom::Ruler::Orientation::horizontal) : nullptr;
    mVerticalRuler = mVerticalZoomAccessor != nullptr ? createRuler(*mVerticalZoomAccessor, Zoom::Ruler::Orientation::vertical) : nullptr;
    for(auto* ruler : {mHorizontalRuler.get(), mVerticalRuler.get()})
    {
        if(ruler != nullptr)
        {
            addAndMakeVisible(ruler);
        }
    }
    auto const toOptionalRef = [](Zoom::Accessor* accessor)
    {
        return accessor != nullptr ? optional_ref<Zoom::Accessor>(std::ref(*accessor)) : optional_ref<Zoom::Accessor>{};
    };
    mScroller.setAccessors(toOptionalRef(mHorizontalZoomAccessor), toOptionalRef(mVerticalZoomAccessor), {});
    setRulersListening(mNavigationMode != NavigationMode::navigate);
    resized();
}

std::unique_ptr<Zoom::Ruler> Track::Slice::Navigator::createRuler(Zoom::Accessor& zoomAccessor, Zoom::Ruler::Orientation orientation)
{
    auto ruler = std::make_unique<Zoom::Ruler>(zoomAccessor, orientation);
    ruler->setInterceptsMouseClicks(false, false);
    ruler->setColour(Zoom::Ruler::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    ruler->setColour(Zoom::Ruler::ColourIds::gridColourId, juce::Colours::transparentBlack);
    ruler->onMouseDown = [](juce::MouseEvent const& event)
    {
        return Zoom::Ruler::getNavigationMode(event.mods) != NavigationMode::navigate;
    };
    ruler->onDoubleClick = [&zoomAccessor]([[maybe_unused]] juce::MouseEvent const& event)
    {
        zoomAccessor.setAttr<Zoom::AttrType::visibleRange>(zoomAccessor.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
    };
    return ruler;
}

void Track::Slice::Navigator::setRulersListening(bool state)
{
    for(auto* ruler : {mHorizontalRuler.get(), mVerticalRuler.get()})
    {
        if(ruler != nullptr)
        {
            // Removes the ruler first to ensure that it is never registered twice
            mContent.removeMouseListener(ruler);
            if(state)
            {
                mContent.addMouseListener(ruler, true);
            }
        }
    }
}

void Track::Slice::Navigator::resized()
{
    auto const bounds = getLocalBounds();
    mScroller.setBounds(bounds);
    for(auto* ruler : {mHorizontalRuler.get(), mVerticalRuler.get()})
    {
        if(ruler != nullptr)
        {
            ruler->setBounds(bounds);
        }
    }
}

void Track::Slice::Navigator::mouseMove(juce::MouseEvent const& event)
{
    updateInteraction(event.mods);
    updateMouseCursor(event.mods);
}

void Track::Slice::Navigator::mouseDown(juce::MouseEvent const& event)
{
    updateInteraction(event.mods);
}

void Track::Slice::Navigator::mouseUp(juce::MouseEvent const& event)
{
    updateMouseCursor(event.mods);
}

void Track::Slice::Navigator::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    mScroller.mouseWheelMove(event.getEventRelativeTo(&mScroller), wheel);
}

void Track::Slice::Navigator::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    mScroller.mouseMagnify(event.getEventRelativeTo(&mScroller), magnifyAmount);
}

void Track::Slice::Navigator::updateInteraction(juce::ModifierKeys const& modifiers)
{
    auto const navigationMode = Zoom::Ruler::getNavigationMode(modifiers);
    if(std::exchange(mNavigationMode, navigationMode) != navigationMode)
    {
        // The rulers only listen to the content in the modes that zoom or move the ranges
        setRulersListening(mNavigationMode != NavigationMode::navigate);
    }
}

void Track::Slice::Navigator::updateMouseCursor(juce::ModifierKeys const& modifiers)
{
    switch(Zoom::Ruler::getNavigationMode(modifiers))
    {
        case NavigationMode::zoom:
        {
            static auto const zoomImage = juce::ImageCache::getFromMemory(AnlCursorsData::selectionzoom_png, AnlCursorsData::selectionzoom_pngSize);
            static auto const zoomCursor = juce::MouseCursor(zoomImage, 8, 8, 2.0f);
            mContent.setMouseCursor(zoomCursor);
            break;
        }
        case NavigationMode::translate:
        {
            mContent.setMouseCursor({juce::MouseCursor::StandardCursorType::UpDownLeftRightResizeCursor});
            break;
        }
        case NavigationMode::select:
        {
            static auto const selectImage = juce::ImageCache::getFromMemory(AnlCursorsData::selectionrectangle_png, AnlCursorsData::selectionrectangle_pngSize);
            static auto const selectCursor = juce::MouseCursor(selectImage, 8, 8, 2.0f);
            mContent.setMouseCursor(selectCursor);
            break;
        }
        case NavigationMode::navigate:
        {
            mContent.setMouseCursor(juce::MouseCursor::NormalCursor);
            break;
        }
    }
}

Track::Slice::Frame::Frame(juce::Component& content)
: mContent(content)
, mContentDecoration(content)
, mNavigator(content)
{
    addAndMakeVisible(mContentDecoration);
    addAndMakeVisible(mNavigator);
}

Track::Slice::Frame::~Frame()
{
    setTracks(nullptr, nullptr);
}

void Track::Slice::Frame::setTracks(Accessor* horizontalTrack, Accessor* verticalTrack)
{
    auto const horizontalFrameType = horizontalTrack != nullptr ? Tools::getFrameType(*horizontalTrack) : std::optional<FrameType>{};
    auto* horizontalZoomAcsr = horizontalFrameType == FrameType::value ? std::addressof(horizontalTrack->getAcsr<AcsrType::valueZoom>()) : (horizontalFrameType == FrameType::vector ? std::addressof(horizontalTrack->getAcsr<AcsrType::binZoom>()) : nullptr);
    auto const hasVerticalZoom = verticalTrack != nullptr && Tools::getFrameType(*verticalTrack) == FrameType::vector;
    auto* verticalZoomAcsr = hasVerticalZoom ? std::addressof(verticalTrack->getAcsr<AcsrType::valueZoom>()) : nullptr;

    if(horizontalZoomAcsr != mHorizontalZoomAccessor)
    {
        // The decoration refers to the ruler so it must be deleted first
        mHorizontalRulerDecoration.reset();
        mHorizontalRuler.reset();
        mHorizontalScrollBar.reset();
        mHorizontalZoomAccessor = horizontalZoomAcsr;
        if(horizontalZoomAcsr != nullptr)
        {
            auto& trackAcsr = *horizontalTrack;
            auto const isVector = horizontalFrameType == FrameType::vector;
            auto const stringify = [acsr = std::addressof(trackAcsr)](double value)
            {
                auto const fn = Renderer::getGridStringify(*acsr);
                return fn != nullptr ? fn(value) : juce::String();
            };
            mHorizontalRuler = createRuler(*horizontalZoomAcsr, Zoom::Ruler::Orientation::horizontal, stringify, getHorizontalLabelWidth(trackAcsr), [&trackAcsr, isVector]()
                                           {
                                               return isVector ? Tools::createBinRangeEditor(trackAcsr) : Tools::createValueRangeEditor(trackAcsr);
                                           });
            mHorizontalRulerDecoration = std::make_unique<Decorator>(*mHorizontalRuler.get());
            mHorizontalScrollBar = std::make_unique<Zoom::ScrollBar>(*horizontalZoomAcsr, Zoom::ScrollBar::Orientation::horizontal);
            addAndMakeVisible(mHorizontalRulerDecoration.get());
            addAndMakeVisible(mHorizontalScrollBar.get());
        }
    }

    if(verticalZoomAcsr != mVerticalZoomAccessor)
    {
        // The decoration refers to the ruler so it must be deleted first
        mVerticalRulerDecoration.reset();
        mVerticalRuler.reset();
        mVerticalScrollBar.reset();
        mVerticalZoomAccessor = verticalZoomAcsr;
        if(verticalZoomAcsr != nullptr)
        {
            auto& trackAcsr = *verticalTrack;
            mVerticalRuler = createRuler(*verticalZoomAcsr, Zoom::Ruler::Orientation::vertical, nullptr, 0, [&trackAcsr]()
                                         {
                                             return Tools::createValueRangeEditor(trackAcsr);
                                         });
            mVerticalRulerDecoration = std::make_unique<Decorator>(*mVerticalRuler.get());
            mVerticalScrollBar = std::make_unique<Zoom::ScrollBar>(*verticalZoomAcsr, Zoom::ScrollBar::Orientation::vertical, true);
            addAndMakeVisible(mVerticalRulerDecoration.get());
            addAndMakeVisible(mVerticalScrollBar.get());
        }
    }
    mNavigator.setZoomAccessors(mHorizontalZoomAccessor, mVerticalZoomAccessor);
    lookAndFeelChanged();
    resized();
}

std::unique_ptr<Zoom::Ruler> Track::Slice::Frame::createRuler(Zoom::Accessor& zoomAccessor, Zoom::Ruler::Orientation orientation, std::function<juce::String(double)> stringify, int maxStringWidth, std::function<std::unique_ptr<juce::Component>(void)> createRangeEditor)
{
    auto ruler = stringify != nullptr ? std::make_unique<Zoom::Ruler>(zoomAccessor, orientation, stringify, maxStringWidth) : std::make_unique<Zoom::Ruler>(zoomAccessor, orientation);
    ruler->onMouseDown = [rulerPtr = ruler.get(), orientation, createRangeEditor](juce::MouseEvent const& event)
    {
        if(!event.mods.isPopupMenu())
        {
            return true;
        }
        auto rangeEditor = createRangeEditor != nullptr ? createRangeEditor() : nullptr;
        if(rangeEditor == nullptr)
        {
            return false;
        }
        auto const point = juce::Desktop::getMousePosition();
        auto const rulerBounds = rulerPtr->getScreenBounds();
        auto const isVertical = orientation == Zoom::Ruler::Orientation::vertical;
        auto const size = isVertical ? rulerBounds.getWidth() : rulerBounds.getHeight();
        auto const bounds = isVertical ? rulerBounds.withY(point.getY() - size / 2).withHeight(size) : rulerBounds.withX(point.getX() - size / 2).withWidth(size);
        Tooltip::showCallOutBox(std::move(rangeEditor), *rulerPtr, bounds);
        return false;
    };
    ruler->onDoubleClick = [&zoomAccessor]([[maybe_unused]] juce::MouseEvent const& event)
    {
        zoomAccessor.setAttr<Zoom::AttrType::visibleRange>(zoomAccessor.getAttr<Zoom::AttrType::globalRange>(), NotificationType::synchronous);
    };
    return ruler;
}

void Track::Slice::Frame::resized()
{
    auto constexpr rulerSize = 16;
    auto constexpr scrollBarSize = 8;
    auto bounds = getLocalBounds();
    auto const hasHorizontal = mHorizontalRulerDecoration != nullptr && mHorizontalScrollBar != nullptr;
    auto const hasVertical = mVerticalRulerDecoration != nullptr && mVerticalScrollBar != nullptr;
    auto const top = hasHorizontal ? bounds.removeFromTop(rulerSize) : juce::Rectangle<int>{};
    auto const bottom = hasHorizontal ? bounds.removeFromBottom(scrollBarSize) : juce::Rectangle<int>{};
    if(hasVertical)
    {
        mVerticalScrollBar->setBounds(bounds.removeFromRight(scrollBarSize));
        mVerticalRulerDecoration->setBounds(bounds.removeFromRight(rulerSize));
    }
    if(hasHorizontal)
    {
        mHorizontalRulerDecoration->setBounds(top.withX(bounds.getX()).withWidth(bounds.getWidth()));
        mHorizontalScrollBar->setBounds(bottom.withX(bounds.getX()).withWidth(bounds.getWidth()));
    }
    mContentDecoration.setBounds(bounds);
    mNavigator.setBounds(getLocalArea(&mContent, mContent.getLocalBounds()));
}

void Track::Slice::Frame::lookAndFeelChanged()
{
    auto const colour = findColour(Decorator::ColourIds::normalBorderColourId);
    for(auto* ruler : {mHorizontalRuler.get(), mVerticalRuler.get()})
    {
        if(ruler != nullptr)
        {
            ruler->setColour(Zoom::Ruler::ColourIds::gridColourId, colour);
        }
    }
}

bool Track::Slice::isSupported(Accessor const& accessor)
{
    auto const frameType = Tools::getFrameType(accessor);
    return frameType.has_value() && frameType.value() != FrameType::label;
}

double Track::Slice::getTime(Transport::Accessor const& transportAccessor)
{
    auto const isPlaying = transportAccessor.getAttr<Transport::AttrType::playback>();
    return isPlaying ? transportAccessor.getAttr<Transport::AttrType::runningPlayhead>() : transportAccessor.getAttr<Transport::AttrType::startPlayhead>();
}

void Track::Slice::paint(Accessor const& accessor, double time, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const horizontalGridColour, juce::Colour const verticalGridColour)
{
    auto const frameType = Tools::getFrameType(accessor);
    if(!frameType.has_value() || frameType.value() == FrameType::label)
    {
        return;
    }
    auto const& channels = accessor.getAttr<AttrType::channelsLayout>();
    Renderer::paintChannels(g, bounds, channels, horizontalGridColour, [&](juce::Rectangle<int> region, size_t channel)
                            {
                                paintGrid(accessor, g, region, horizontalGridColour, verticalGridColour);
                                if(frameType.value() == FrameType::value)
                                {
                                    paintPoints(accessor, channel, g, region, time);
                                }
                                else
                                {
                                    paintColumns(accessor, channel, g, region, time);
                                }
                            });
}

int Track::Slice::getHorizontalLabelWidth(Accessor const& accessor)
{
    if(Tools::getFrameType(accessor) == FrameType::vector)
    {
        return Tools::hasVerticalZoomInHertz(accessor) ? 120 : 80;
    }
    return 72;
}

void Track::Slice::paintGrid(Accessor const& accessor, juce::Graphics& g, juce::Rectangle<int> const& bounds, juce::Colour const horizontalGridColour, juce::Colour const verticalGridColour)
{
    auto const gridMode = accessor.getAttr<AttrType::grid>();
    if(gridMode == GridMode::hidden || bounds.isEmpty())
    {
        return;
    }
    auto const frameType = Tools::getFrameType(accessor);
    if(!frameType.has_value() || frameType.value() == FrameType::label)
    {
        return;
    }

    using Justification = Zoom::Grid::Justification;
    auto const justificationHorizontal = gridMode == GridMode::partial ? Justification(Justification::left | Justification::right) : Justification(Justification::horizontal);
    auto const justificationVertical = gridMode == GridMode::partial ? Justification(Justification::top | Justification::bottom) : Justification(Justification::vertical);

    // The horizontal axis corresponds to the values of the points or to the bins of the vectors (its labels are drawn by the horizontal ruler)
    if(!horizontalGridColour.isTransparent())
    {
        auto const& horizontalZoomAcsr = frameType.value() == FrameType::value ? accessor.getAcsr<AcsrType::valueZoom>() : accessor.getAcsr<AcsrType::binZoom>();
        g.setColour(horizontalGridColour);
        Zoom::Grid::paintHorizontal(g, horizontalZoomAcsr.getAcsr<Zoom::AcsrType::grid>(), horizontalZoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), bounds, nullptr, getHorizontalLabelWidth(accessor), justificationVertical);
    }

    // The vertical axis corresponds to the values of the vectors
    if(!verticalGridColour.isTransparent() && frameType.value() == FrameType::vector)
    {
        auto const& verticalZoomAcsr = accessor.getAcsr<AcsrType::valueZoom>();
        auto const unit = Tools::getUnit(accessor);
        auto const verticalStringify = [&](double value)
        {
            return Format::valueToString(value, 4) + unit;
        };
        g.setColour(verticalGridColour);
        Zoom::Grid::paintVertical(g, verticalZoomAcsr.getAcsr<Zoom::AcsrType::grid>(), verticalZoomAcsr.getAttr<Zoom::AttrType::visibleRange>(), bounds, verticalStringify, justificationHorizontal);
    }
}

void Track::Slice::paintPoints(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, double time)
{
    auto const& valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    if(bounds.isEmpty() || valueRange.isEmpty())
    {
        return;
    }

    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access) || results.isEmpty())
    {
        return;
    }
    auto const value = Results::getValue(results.getPoints(), channel, time);
    if(!value.has_value() || !std::isfinite(value.value()))
    {
        return;
    }

    auto const logScaleRatio = getPointLogScaleRatio(accessor);
    auto const scaledValue = logScaleRatio.has_value() ? Tools::getMidiFromHertz(static_cast<double>(value.value())) * logScaleRatio.value() : static_cast<double>(value.value());
    auto const fbounds = bounds.toFloat();
    auto const x = valueToHorizontalPixel(scaledValue, valueRange, fbounds);
    if(x < fbounds.getX() - 4.0f || x > fbounds.getRight() + 4.0f)
    {
        return;
    }

    auto const& graphicsSettings = accessor.getAttr<AttrType::graphicsSettings>();
    juce::Path path;
    path.addLineSegment({x, fbounds.getY(), x, fbounds.getBottom()}, graphicsSettings.lineWidth);
    fillPath(g, path, graphicsSettings.colours, {-1.0f, 0.0f});
}

void Track::Slice::paintColumns(Accessor const& accessor, size_t channel, juce::Graphics& g, juce::Rectangle<int> const& bounds, double time)
{
    auto const& binRange = accessor.getAcsr<AcsrType::binZoom>().getAttr<Zoom::AttrType::visibleRange>();
    auto const& valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    if(bounds.isEmpty() || binRange.isEmpty() || valueRange.isEmpty())
    {
        return;
    }

    auto const numBins = Tools::getNumBins(accessor);
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access) || results.isEmpty())
    {
        return;
    }
    auto const columns = results.getColumns();
    if(columns == nullptr || columns->size() <= channel)
    {
        return;
    }
    auto const* column = getColumn(columns->at(channel), time);
    if(column == nullptr || std::get<2_z>(*column).empty())
    {
        return;
    }

    auto const& values = std::get<2_z>(*column);
    auto const fbounds = bounds.toFloat();
    // Limits the positions of the values outside the visible range to avoid huge coordinates
    auto const getY = [&](float value)
    {
        auto const y = Tools::valueToPixel(value, valueRange, fbounds);
        return std::clamp(y, fbounds.getY() - fbounds.getHeight(), fbounds.getBottom() + fbounds.getHeight());
    };

    // The curve is split in segments where the values are not finite
    std::vector<std::vector<juce::Point<float>>> segments;
    auto addPoint = [&](float x, float value)
    {
        if(!std::isfinite(value))
        {
            if(segments.empty() || !segments.back().empty())
            {
                segments.emplace_back();
            }
            return;
        }
        if(segments.empty())
        {
            segments.emplace_back();
        }
        segments.back().push_back({x, getY(value)});
    };

    if(values.size() == 1_z)
    {
        addPoint(fbounds.getX(), values.front());
        addPoint(fbounds.getRight(), values.front());
    }
    else
    {
        BinScale const scale(accessor, numBins.value_or(values.size()));
        auto const getX = [&](size_t index)
        {
            return valueToHorizontalPixel(scale.getZoomValue(index), binRange, fbounds);
        };

        // Only the bins in the visible range (and their direct neighbours) are drawn
        auto index = 0_z;
        while(index + 1_z < values.size() && getX(index + 1_z) < fbounds.getX())
        {
            ++index;
        }
        for(; index < values.size(); ++index)
        {
            auto const x = getX(index);
            addPoint(x, values.at(index));
            if(x > fbounds.getRight())
            {
                break;
            }
        }
    }

    juce::Path linePath;
    juce::Path areaPath;
    for(auto const& segment : segments)
    {
        if(segment.empty())
        {
            continue;
        }
        linePath.startNewSubPath(segment.front());
        areaPath.startNewSubPath(segment.front().x, fbounds.getBottom());
        for(auto const& point : segment)
        {
            linePath.lineTo(point);
            areaPath.lineTo(point);
        }
        areaPath.lineTo(segment.back().x, fbounds.getBottom());
        areaPath.closeSubPath();
    }
    if(linePath.isEmpty())
    {
        return;
    }

    // The colours correspond to the colour map used to render the images of the vectors
    auto const gradient = createColourMapGradient(accessor.getAttr<AttrType::graphicsSettings>().colours.map, fbounds);
    auto areaGradient = gradient;
    areaGradient.multiplyOpacity(0.4f);
    g.setGradientFill(areaGradient);
    g.fillPath(areaPath);

    juce::Path strokePath;
    juce::PathStrokeType(1.5f, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded).createStrokedPath(strokePath, linePath);
    g.setGradientFill(gradient);
    g.fillPath(strokePath);
}

juce::StringArray Track::Slice::getTooltip(Accessor const& accessor, juce::Rectangle<int> const& bounds, juce::Point<int> const& pt, double time)
{
    juce::StringArray lines;
    auto const frameType = Tools::getFrameType(accessor);
    if(!frameType.has_value() || frameType.value() == FrameType::label || bounds.getWidth() <= 0)
    {
        return lines;
    }
    auto const channelRange = Tools::getChannelVerticalRange(accessor, bounds, pt.y, false);
    if(!channelRange.has_value())
    {
        return lines;
    }
    auto const channel = std::get<0_z>(channelRange.value());
    auto const region = bounds.withTop(std::get<1_z>(channelRange.value()).getStart()).withBottom(std::get<1_z>(channelRange.value()).getEnd());

    auto const& description = accessor.getAttr<AttrType::description>();
    auto const name = juce::String(description.output.name.empty() ? "Feature" : description.output.name);
    auto const unit = Tools::getUnit(accessor);
    auto const mouseLine = [](juce::String const& text)
    {
        return juce::translate("Mouse: VALSTR").replace("VALSTR", text);
    };

    if(frameType.value() == FrameType::value)
    {
        auto const& valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
        if(!valueRange.isEmpty())
        {
            auto const mouseValue = horizontalPixelToValue(pt.x, valueRange, region);
            auto const logScaleRatio = getPointLogScaleRatio(accessor);
            auto const value = logScaleRatio.has_value() ? Tools::getHertzFromMidi(mouseValue / logScaleRatio.value()) : mouseValue;
            lines.add(mouseLine(Format::valueToString(value, 4) + unit));
        }

        auto const& results = accessor.getAttr<AttrType::results>();
        auto const access = results.getReadAccess();
        if(!static_cast<bool>(access) || results.isEmpty())
        {
            return lines;
        }
        auto const value = Results::getValue(results.getPoints(), channel, time);
        lines.add(name + ": " + (value.has_value() ? Format::valueToString(value.value(), 4) + unit : juce::String("-")));
        return lines;
    }

    auto const& valueRange = accessor.getAcsr<AcsrType::valueZoom>().getAttr<Zoom::AttrType::visibleRange>();
    if(!valueRange.isEmpty() && !region.isEmpty())
    {
        auto const mouseValue = Tools::pixelToValue(static_cast<float>(pt.y), valueRange, region.toFloat());
        lines.add(mouseLine(Format::valueToString(mouseValue, 4) + unit));
    }

    auto const& binRange = accessor.getAcsr<AcsrType::binZoom>().getAttr<Zoom::AttrType::visibleRange>();
    auto const numBins = Tools::getNumBins(accessor);
    auto const& results = accessor.getAttr<AttrType::results>();
    auto const access = results.getReadAccess();
    if(!static_cast<bool>(access) || results.isEmpty() || binRange.isEmpty())
    {
        return lines;
    }
    auto const columns = results.getColumns();
    auto const* column = columns != nullptr && channel < columns->size() ? getColumn(columns->at(channel), time) : nullptr;
    if(column == nullptr)
    {
        lines.add(name + ": -");
        return lines;
    }
    auto const& values = std::get<2_z>(*column);
    BinScale const scale(accessor, numBins.value_or(values.size()));
    auto const index = scale.getIndex(horizontalPixelToValue(pt.x, binRange, region));
    if(!index.has_value() || index.value() >= values.size())
    {
        lines.add(name + ": -");
        return lines;
    }
    auto const binName = Tools::getBinName(accessor, index.value());
    auto info = juce::translate("Bin IDX").replace("IDX", juce::String(index.value()));
    if(binName.isNotEmpty())
    {
        info += " - " + binName;
    }
    if(scale.isHertz())
    {
        info += " - " + Format::valueToString(scale.getHertz(index.value()), 4) + " Hz";
    }
    lines.add(name + ": " + Format::valueToString(values.at(index.value()), 4) + unit + " (" + info + ")");
    return lines;
}

ANALYSE_FILE_END
