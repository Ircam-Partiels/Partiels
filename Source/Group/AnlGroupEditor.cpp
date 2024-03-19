#include "AnlGroupEditor.h"
#include "../Track/AnlTrackTools.h"
#include "../Track/AnlTrackTooltip.h"

ANALYSE_FILE_BEGIN

Group::Editor::Editor(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor, juce::ApplicationCommandManager& commandManager, juce::Component& content)
: mContent(content)
, mDirector(director)
, mAccessor(director.getAccessor())
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
, mCommandManager(commandManager)
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::showInGroup})
{
    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::expanded:
            case AttrType::layout:
            case AttrType::tracks:
                break;
            case AttrType::referenceid:
            {
                updateContent();
                break;
            }
            case AttrType::focused:
            case AttrType::colour:
            case AttrType::name:
            {
                repaint();
                updateTrackEditor();
                break;
            }
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Editor::~Editor()
{
    mAccessor.removeListener(mListener);
}

void Group::Editor::resized()
{
    if(mTrackEditor != nullptr)
    {
        mTrackEditor->setBounds(getLocalBounds());
    }
}

void Group::Editor::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colour>());
}

void Group::Editor::updateContent()
{
    auto const trackAcsr = Tools::getReferenceTrackAcsr(mAccessor);
    if(!trackAcsr.has_value())
    {
        mTrackEditor.reset();
        mTrackIdentifier.clear();
    }
    else if(mTrackIdentifier != trackAcsr.value().get().getAttr<Track::AttrType::identifier>())
    {
        auto const trackIdentifier = trackAcsr.value().get().getAttr<Track::AttrType::identifier>();
        mTrackEditor = std::make_unique<Track::Editor>(
            mDirector.getTrackDirector(trackIdentifier), mTimeZoomAccessor, mTransportAccessor, mCommandManager, mContent, [this](juce::Point<int> const& pt)
            {
                return getBubbleTooltip(pt);
            },
            false);
        if(mTrackEditor != nullptr)
        {
            mTrackIdentifier = trackIdentifier;
            mTrackEditor->onMouseDown = [this](juce::MouseEvent const& event)
            {
                if(!event.mods.isPopupMenu())
                {
                    return;
                }
                showPopupMenu();
            };
        }
        addAndMakeVisible(mTrackEditor.get());
    }
    updateTrackEditor();
    resized();
}

void Group::Editor::updateTrackEditor()
{
    if(mTrackEditor != nullptr)
    {
        mTrackEditor->setSnapshotNameAndColour(mAccessor.getAttr<AttrType::name>(), mAccessor.getAttr<AttrType::colour>());
        mTrackEditor->setFocusInfo(mAccessor.getAttr<AttrType::focused>());
    }
}

void Group::Editor::showPopupMenu(juce::Point<int> const position)
{
    auto const switchAction = [this](PopupMenuAction const nextAction)
    {
        auto const action = std::exchange(mPopupMenuAction, nextAction);
        if(action != nextAction)
        {
            switch(action)
            {
                case PopupMenuAction::none:
                    break;
                case PopupMenuAction::referenceTrack:
                    mDirector.endAction(false, ActionState::newTransaction, juce::translate("Change track reference of the group"));
                    break;
                case PopupMenuAction::trackLayout:
                    mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change track reference of the group"));
                    break;
                case PopupMenuAction::channelLayout:
                    mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change the visibility of the channels of the group"));
                    break;
            }
            switch(nextAction)
            {
                case PopupMenuAction::none:
                    break;
                case PopupMenuAction::referenceTrack:
                    mDirector.startAction(false);
                    break;
                case PopupMenuAction::trackLayout:
                case PopupMenuAction::channelLayout:
                    mDirector.startAction(true);
                    break;
            }
        }
    };
    juce::PopupMenu mainMenu;
    {
        juce::PopupMenu subMenu;
        auto const referenceId = mAccessor.getAttr<AttrType::referenceid>();
        subMenu.addItem(juce::translate("Front"), !referenceId.isEmpty(), referenceId.isEmpty(), [=, this]()
                        {
                            switchAction(PopupMenuAction::referenceTrack);
                            mAccessor.setAttr<AttrType::referenceid>(juce::String(), NotificationType::synchronous);
                            showPopupMenu(position);
                        });
        subMenu.addSeparator();
        auto const layout = mAccessor.getAttr<AttrType::layout>();
        for(auto layoutIndex = 0_z; layoutIndex < layout.size(); ++layoutIndex)
        {
            auto const trackId = layout.at(layoutIndex);
            auto const trackAcsr = Tools::getTrackAcsr(mAccessor, trackId);
            if(trackAcsr.has_value())
            {
                auto const& trackName = trackAcsr.value().get().getAttr<Track::AttrType::name>();
                auto const itemLabel = trackName.isEmpty() ? juce::translate("Track IDX").replace("IDX", juce::String(layoutIndex + 1)) : trackName;
                subMenu.addItem(itemLabel, trackId != referenceId, trackId == referenceId, [=, this]()
                                {
                                    switchAction(PopupMenuAction::referenceTrack);
                                    mAccessor.setAttr<AttrType::referenceid>(trackId, NotificationType::synchronous);
                                    showPopupMenu(position);
                                });
            }
        }
        if(subMenu.getNumItems() > 2)
        {
            mainMenu.addSubMenu(juce::translate("Track Reference"), subMenu);
        }
    }
    {
        juce::PopupMenu subMenu;
        Tools::fillMenuForTrackVisibility(
            mAccessor, subMenu, [=]()
            {
                switchAction(PopupMenuAction::trackLayout);
            },
            [=, this]()
            {
                showPopupMenu(position);
            });
        if(subMenu.getNumItems() > 0)
        {
            mainMenu.addSubMenu(juce::translate("Track Layout"), subMenu);
        }
    }
    {
        juce::PopupMenu subMenu;
        Tools::fillMenuForChannelVisibility(
            mAccessor, subMenu, [=]()
            {
                switchAction(PopupMenuAction::channelLayout);
            },
            [=, this]()
            {
                showPopupMenu(position);
            });
        if(subMenu.getNumItems() > 1)
        {
            mainMenu.addSubMenu(juce::translate("Channel Layout"), subMenu);
        }
    }
    auto options = juce::PopupMenu::Options().withDeletionCheck(*this).withTargetScreenArea(juce::Rectangle<int>{}.withPosition(position));
    mainMenu.showMenuAsync(options, [=](int result)
                           {
                               if(result == 0)
                               {
                                   switchAction(PopupMenuAction::none);
                               }
                           });
}

juce::String Group::Editor::getBubbleTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        return "";
    }
    juce::StringArray lines;
    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    lines.add(juce::translate("Time: TIME").replace("TIME", Format::secondsToString(time)));
    auto const referenceTrackAcsr = Tools::getReferenceTrackAcsr(mAccessor);
    if(referenceTrackAcsr.has_value() && Track::Tools::hasVerticalZoom(referenceTrackAcsr.value()))
    {
        lines.add(juce::translate("Mouse: VALUE").replace("VALUE", Track::Tools::getZoomTootip(referenceTrackAcsr.value().get(), *this, pt.y)));
    }
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>())
        {
            lines.add(trackAcsr.value().get().getAttr<Track::AttrType::name>() + ":");
            for(auto const& line : Track::Tools::getValueTootip(trackAcsr.value().get(), mTimeZoomAccessor, *this, pt.y, time))
            {
                lines.addArray("\t" + line);
            }
        }
    }
    return lines.joinIntoString("\n");
}

ANALYSE_FILE_END
