#include "AnlTrackSection.h"
#include "../Zoom/AnlZoomTools.h"
#include "AnlTrackExporter.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::Section::Section(Director& director, Zoom::Accessor& timeZoomAcsr, Transport::Accessor& transportAcsr)
: mDirector(director)
, mTimeZoomAccessor(timeZoomAcsr)
, mTransportAccessor(transportAcsr)
{
    mValueRuler.onDoubleClick = [&]()
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
        auto const& range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };

    mBinRuler.onDoubleClick = [&]()
    {
        auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
        auto const range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
        zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType type)
    {
        switch(type)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::key:
                break;
            case AttrType::description:
            {
                mValueRuler.setVisible(Tools::getDisplayType(mAccessor) == Tools::DisplayType::segments);
                mValueScrollBar.setVisible(Tools::getDisplayType(mAccessor) == Tools::DisplayType::segments);
                mBinRuler.setVisible(Tools::getDisplayType(mAccessor) == Tools::DisplayType::grid);
                mBinScrollBar.setVisible(Tools::getDisplayType(mAccessor) == Tools::DisplayType::grid);
            }
            break;
            case AttrType::state:
                break;
            case AttrType::height:
            {
                setSize(getWidth(), acsr.getAttr<AttrType::height>() + 1);
            }
            break;
            case AttrType::colours:
            case AttrType::zoomLink:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::time:
            case AttrType::warnings:
            case AttrType::processing:
            case AttrType::zoomAcsr:
                break;
            case AttrType::focused:
            {
                auto const focused = mAccessor.getAttr<AttrType::focused>();
                mThumbnailDecoration.setHighlighted(focused);
                mSnapshotDecoration.setHighlighted(focused);
                mPlotDecoration.setHighlighted(focused);
            }
            break;
        }
    };

    mResizerBar.onMoved = [&](int size)
    {
        mAccessor.setAttr<AttrType::height>(size, NotificationType::synchronous);
    };

    mThumbnail.onExportButtonClicked = [&](juce::Button& button)
    {
        auto const processing = mAccessor.getAttr<AttrType::processing>();
        auto const& results = mAccessor.getAttr<AttrType::results>();
        auto const resultsAreReady = !std::get<0>(processing) && results != nullptr && !results->empty();
        auto const graphicsAreReady = !std::get<2>(processing) && results != nullptr && !results->empty();
        juce::PopupMenu menu;
        menu.addItem("Export as image", graphicsAreReady, false, [this]()
                     {
                         juce::FileChooser fc(juce::translate("Export to image"), {}, "*.png;*.jpeg;*.jpg");
                         if(!fc.browseForFileToSave(true))
                         {
                             return;
                         }
                         auto const width = mPlot.getWidth();
                         auto const height = mPlot.getHeight();
                         auto const result = Exporter::toImage(mAccessor, mTimeZoomAccessor, fc.getResult(), width, height);
                         if(result.failed())
                         {
                             AlertWindow::showMessage(AlertWindow::MessageType::warning, "Export as image failed!", result.getErrorMessage());
                         }
                     });
        menu.addItem("Export as CSV...", resultsAreReady, false, [this]()
                     {
                         Exporter::toCsv(mAccessor, AlertType::window);
                     });
        menu.addItem("Export as XML...", resultsAreReady, false, [this]()
                     {
                         Exporter::toXml(mAccessor, AlertType::window);
                     });
        menu.showAt(&button);
        // Force to repaint to update the state
        button.setState(juce::Button::ButtonState::buttonNormal);
    };

    addChildComponent(mValueRuler);
    addChildComponent(mValueScrollBar);
    addChildComponent(mBinRuler);
    addChildComponent(mBinScrollBar);
    addAndMakeVisible(mThumbnailDecoration);
    addAndMakeVisible(mSnapshotDecoration);
    addAndMakeVisible(mPlotDecoration);
    addAndMakeVisible(mResizerBar);
    setSize(80, 100);
    setWantsKeyboardFocus(true);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Section::~Section()
{
    mAccessor.removeListener(mListener);
}

juce::Rectangle<int> Track::Section::getPlotBounds() const
{
    return mPlot.getBounds();
}

void Track::Section::resized()
{
    mResizerBar.setBounds(getLocalBounds().removeFromBottom(2).reduced(2, 0));

    auto bounds = getLocalBounds();
    mThumbnailDecoration.setBounds(bounds.removeFromLeft(48));
    mSnapshotDecoration.setBounds(bounds.removeFromLeft(48));

    mValueScrollBar.setBounds(bounds.removeFromRight(8));
    mBinScrollBar.setBounds(mValueScrollBar.getBounds());
    mValueRuler.setBounds(bounds.removeFromRight(16));
    mBinRuler.setBounds(mValueRuler.getBounds());
    mPlotDecoration.setBounds(bounds);
}

void Track::Section::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
}

void Track::Section::colourChanged()
{
    setOpaque(findColour(ColourIds::backgroundColourId).isOpaque());
}

void Track::Section::mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel)
{
    if(!event.mods.isCommandDown())
    {
        Component::mouseWheelMove(event, wheel);
        return;
    }

    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
            break;
        case Tools::DisplayType::segments:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
            auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const offset = static_cast<double>(-wheel.deltaY) * visibleRange.getLength();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
        }
        break;
        case Tools::DisplayType::grid:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
            auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
            auto const offset = static_cast<double>(-wheel.deltaY) * visibleRange.getLength();
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange - offset, NotificationType::synchronous);
        }
        break;
    }
}

void Track::Section::mouseMagnify(juce::MouseEvent const& event, float magnifyAmount)
{
    if(!event.mods.isCommandDown())
    {
        Component::mouseMagnify(event, magnifyAmount);
        return;
    }

    switch(Tools::getDisplayType(mAccessor))
    {
        case Tools::DisplayType::markers:
            break;
        case Tools::DisplayType::segments:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
            auto const globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            auto const amount = static_cast<double>(1.0f - magnifyAmount) / 5.0 * globalRange.getLength();
            auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();

            auto const anchor = Zoom::Tools::getScaledValueFromHeight(zoomAcsr, *this, event.y);
            auto const amountLeft = (anchor - visibleRange.getStart()) / visibleRange.getEnd() * amount;
            auto const amountRight = (visibleRange.getEnd() - anchor) / visibleRange.getEnd() * amount;

            auto const minDistance = zoomAcsr.getAttr<Zoom::AttrType::minimumLength>() / 2.0;
            auto const start = std::min(anchor - minDistance, visibleRange.getStart() - amountLeft);
            auto const end = std::max(anchor + minDistance, visibleRange.getEnd() + amountRight);

            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{start, end}, NotificationType::synchronous);
        }
        break;
        case Tools::DisplayType::grid:
        {
            auto& zoomAcsr = mAccessor.getAcsr<AcsrType::binZoom>();
            auto const globalRange = zoomAcsr.getAttr<Zoom::AttrType::globalRange>();
            auto const amount = static_cast<double>(1.0f - magnifyAmount) / 5.0 * globalRange.getLength();
            auto const visibleRange = zoomAcsr.getAttr<Zoom::AttrType::visibleRange>();

            auto const anchor = Zoom::Tools::getScaledValueFromHeight(zoomAcsr, *this, event.y);
            auto const amountLeft = (anchor - visibleRange.getStart()) / visibleRange.getEnd() * amount;
            auto const amountRight = (visibleRange.getEnd() - anchor) / visibleRange.getEnd() * amount;

            auto const minDistance = zoomAcsr.getAttr<Zoom::AttrType::minimumLength>() / 2.0;
            auto const start = std::min(anchor - minDistance, visibleRange.getStart() - amountLeft);
            auto const end = std::max(anchor + minDistance, visibleRange.getEnd() + amountRight);

            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{start, end}, NotificationType::synchronous);
        }
        break;
    }
}

void Track::Section::focusOfChildComponentChanged(juce::Component::FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    juce::WeakReference<juce::Component> target(this);
    juce::MessageManager::callAsync([=, this]
                                    {
                                        if(target.get() != nullptr)
                                        {
                                            mAccessor.setAttr<AttrType::focused>(hasKeyboardFocus(true), NotificationType::synchronous);
                                        }
                                    });
}

ANALYSE_FILE_END
