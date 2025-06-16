#include "AnlTrackEditor.h"
#include "../AnlTrackTools.h"
#include "../AnlTrackTooltip.h"

ANALYSE_FILE_BEGIN

Track::Editor::Editor(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor, juce::ApplicationCommandManager& commandManager, juce::Component& content, std::function<juce::String(juce::Point<int> const&)> getTooltip, bool paintBackground)
: mContent(content)
, mAccessor(director.getAccessor())
, mTimeZoomAccessor(timeZoomAccessor)
, mApplicationCommandManager(commandManager)
, mPaintBackground(paintBackground)
, mSnapshotName(mAccessor.getAttr<AttrType::name>())
, mSnapshotColour(mAccessor.getAttr<AttrType::colours>().background)
, mGetTooltip(std::move(getTooltip))
, mWriter(director, mTimeZoomAccessor, transportAccessor)
, mNavigator(mAccessor, mTimeZoomAccessor)
, mScroller(mAccessor, mTimeZoomAccessor, transportAccessor, mApplicationCommandManager)
, mSelector(mAccessor, mTimeZoomAccessor, transportAccessor, mApplicationCommandManager)
{
    addAndMakeVisible(mContent);
    addAndMakeVisible(mScroller);
    addAndMakeVisible(mWriter);
    addAndMakeVisible(mNavigator);
    addAndMakeVisible(mSelector);
    mSelector.addMouseListener(std::addressof(mWriter), true);
    mSelector.addMouseListener(std::addressof(mNavigator), true);
    mSelector.addMouseListener(std::addressof(mScroller), true);
    mSelector.addMouseListener(this, true);

    if(mGetTooltip == nullptr)
    {
        mGetTooltip = [this](juce::Point<int> const& pt) -> juce::String
        {
            if(!getLocalBounds().contains(pt))
            {
                return {};
            }
            juce::StringArray lines;
            auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
            lines.add(juce::translate("Time: TIME").replace("TIME", Format::secondsToString(time)));
            lines.add(juce::translate("Mouse: VALUE").replace("VALUE", Tools::getZoomTootip(mAccessor, *this, pt.y)));
            lines.addArray(Tools::getValueTootip(mAccessor, mTimeZoomAccessor, *this, pt.y, time));
            return lines.joinIntoString("\n");
        };
    }

    mNavigator.onModeUpdated = mWriter.onModeUpdated = [this]()
    {
        editionUpdated();
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::key:
            case AttrType::input:
            case AttrType::state:
            case AttrType::height:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::graphics:
            case AttrType::font:
            case AttrType::lineWidth:
            case AttrType::warnings:
            case AttrType::grid:
            case AttrType::processing:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::focused:
            case AttrType::labelLayout:
            case AttrType::showInGroup:
            case AttrType::sendViaOsc:
            case AttrType::hasPluginColourMap:
                break;
            case AttrType::colours:
            {
                if(mPaintBackground)
                {
                    repaint();
                }
                break;
            }
            case AttrType::file:
            case AttrType::description:
            case AttrType::name:
            case AttrType::unit:
            {
                juce::SettableTooltipClient::setTooltip(Tools::getInfoTooltip(acsr));
                break;
            }
            case AttrType::channelsLayout:
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
            {
                Tooltip::BubbleClient::setTooltip(mGetTooltip(getMouseXYRelative()));
                break;
            }
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
                Tooltip::BubbleClient::setTooltip(mGetTooltip(getMouseXYRelative()));
                break;
            }
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
    mApplicationCommandManager.addListener(this);
    mApplicationCommandManager.registerAllCommandsForTarget(this);
    applicationCommandListChanged();
}

Track::Editor::~Editor()
{
    mApplicationCommandManager.removeListener(this);
    mTimeZoomAccessor.removeListener(mZoomListener);
    mAccessor.removeListener(mListener);
}

void Track::Editor::setSnapshotNameAndColour(juce::String const& name, juce::Colour const& colour)
{
    mSnapshotName = name;
    mSnapshotColour = colour;
}

void Track::Editor::setFocusInfo(FocusInfo const& info)
{
    mSelector.setFocusInfo(info);
}

void Track::Editor::resized()
{
    auto const bounds = getLocalBounds();
    mContent.setBounds(bounds);
    mWriter.setBounds(bounds);
    mNavigator.setBounds(bounds);
    mSelector.setBounds(bounds);
    mScroller.setBounds(bounds);
}

void Track::Editor::paint(juce::Graphics& g)
{
    if(mPaintBackground)
    {
        g.fillAll(mAccessor.getAttr<AttrType::colours>().background);
    }
}

void Track::Editor::mouseMove(juce::MouseEvent const& event)
{
    Tooltip::BubbleClient::setTooltip(mGetTooltip(getLocalPoint(event.eventComponent, event.getPosition())));
}

void Track::Editor::mouseDown(juce::MouseEvent const& event)
{
    if(onMouseDown != nullptr)
    {
        onMouseDown(event);
    }
    else if(event.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        fillPopupMenu(menu);
        menu.showMenuAsync(juce::PopupMenu::Options().withDeletionCheck(*this));
    }
}

void Track::Editor::mouseEnter(juce::MouseEvent const& event)
{
    Tooltip::BubbleClient::setTooltip(mGetTooltip(getLocalPoint(event.eventComponent, event.getPosition())));
}

void Track::Editor::mouseExit([[maybe_unused]] juce::MouseEvent const& event)
{
    Tooltip::BubbleClient::setTooltip("");
}

void Track::Editor::takeSnapshot()
{
    ComponentSnapshot::takeSnapshot(mContent, mSnapshotName, mSnapshotColour);
}

void Track::Editor::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch(info.commandID)
    {
        case ApplicationCommandIDs::frameToggleDrawing:
        {
            auto const canEdit = info.commandFlags & juce::ApplicationCommandInfo::CommandFlags::isTicked;
            auto& activeComponent = canEdit ? static_cast<juce::Component&>(mWriter) : static_cast<juce::Component&>(mNavigator);
            auto& inactiveComponent = canEdit ? static_cast<juce::Component&>(mNavigator) : static_cast<juce::Component&>(mWriter);
            activeComponent.setVisible(true);
            inactiveComponent.setVisible(false);
            mSelector.addMouseListener(std::addressof(activeComponent), true);
            mSelector.removeMouseListener(std::addressof(inactiveComponent));
            editionUpdated();
            break;
        }
        default:
            break;
    }
}

void Track::Editor::applicationCommandListChanged()
{
    Utils::notifyListener(mApplicationCommandManager, *this, {ApplicationCommandIDs::frameToggleDrawing});
}

void Track::Editor::editionUpdated()
{
    auto const canEdit = Utils::isCommandTicked(mApplicationCommandManager, ApplicationCommandIDs::frameToggleDrawing);
    auto const performinAction = (canEdit && mWriter.isPerformingAction()) || (!canEdit && mNavigator.isPerformingAction());
    mSelector.setInterceptsMouseClicks(!performinAction, !performinAction);
    if(performinAction)
    {
        mWriter.addMouseListener(std::addressof(mScroller), true);
        mNavigator.addMouseListener(std::addressof(mScroller), true);
        mWriter.addMouseListener(this, true);
        mNavigator.addMouseListener(this, true);
    }
    else
    {
        mWriter.removeMouseListener(std::addressof(mScroller));
        mNavigator.removeMouseListener(std::addressof(mScroller));
        mWriter.removeMouseListener(this);
        mNavigator.removeMouseListener(this);
    }
}

void Track::Editor::fillPopupMenu(juce::PopupMenu& menu)
{
    using CommandIDs = ApplicationCommandIDs;
    menu.addCommandItem(&mApplicationCommandManager, CommandIDs::editUndo);
    menu.addCommandItem(&mApplicationCommandManager, CommandIDs::editRedo);
    menu.addSeparator();
    // clang-format off
    static auto constexpr commandIds =
    {
          CommandIDs::frameSelectAll
        , CommandIDs::frameDelete
        , CommandIDs::frameCopy
        , CommandIDs::frameCut
        , CommandIDs::framePaste
        , CommandIDs::frameDuplicate
        , CommandIDs::frameInsert
        , CommandIDs::frameBreak
        , CommandIDs::frameResetDurationToZero
        , CommandIDs::frameResetDurationToFull
    };
    // clang-format on
    for(auto const& commandId : commandIds)
    {
        menu.addCommandItem(&mApplicationCommandManager, commandId);
    }
    menu.addSeparator();
    menu.addCommandItem(&mApplicationCommandManager, CommandIDs::frameSystemCopy);
    menu.addSeparator();
    menu.addCommandItem(&mApplicationCommandManager, CommandIDs::frameToggleDrawing);
}

ANALYSE_FILE_END
