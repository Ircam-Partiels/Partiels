#include "AnlGroupPropertyGraphicalsSection.h"
#include "../Track/AnlTrackTools.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

static std::vector<std::string> getColourMapNames()
{
    std::vector<std::string> names;
    for(auto const& name : magic_enum::enum_names<Track::ColourMap>())
    {
        names.push_back(std::string(name));
    }
    return names;
}

Group::PropertyGraphicalsSection::PropertyGraphicalsSection(Director& director)
: mDirector(director)
, mPropertyColourMap(juce::translate("Color Map"), juce::translate("The color map used by the graphical renderers of the tracks of the group."), "", getColourMapNames(), [&](size_t index)
                     {
                         mDirector.startAction(true);
                         setColourMap(static_cast<Track::ColourMap>(index));
                         mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's color map"));
                     })
, mPropertyForegroundColour(
      juce::translate("Foreground Color"), juce::translate("The foreground color used by the graphical renderers of the tracks of the group."), juce::translate("Select the foreground color"), [&](juce::Colour const& colour)
      {
          if(!mPropertyForegroundColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction(true);
          }
          setForegroundColour(colour);
          if(!mPropertyForegroundColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's foreground color"));
          }
      },
      [&]()
      {
          mDirector.startAction(true);
      },
      [&]()
      {
          mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's foreground color"));
      })
, mPropertyBackgroundColour(
      juce::translate("Background Color"), juce::translate("The background color used by the graphical renderers of the tracks of the group."), juce::translate("Select the background color"), [&](juce::Colour const& colour)
      {
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction(true);
          }
          setBackgroundColour(colour);
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's background color"));
          }
      },
      [&]()
      {
          mDirector.startAction(true);
      },
      [&]()
      {
          mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's background color"));
      })
, mPropertyTextColour(
      juce::translate("Text Color"), juce::translate("The text color used by the graphical renderers of the tracks of the group."), juce::translate("Select the text color"), [&](juce::Colour const& colour)
      {
          if(!mPropertyTextColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction(true);
          }
          setTextColour(colour);
          if(!mPropertyTextColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's text color"));
          }
      },
      [&]()
      {
          mDirector.startAction(true);
      },
      [&]()
      {
          mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's text color"));
      })
, mPropertyShadowColour(
      juce::translate("Shadow Color"), juce::translate("The shadow color used by the graphical renderers of the tracks of the group."), juce::translate("Select the shadow color"), [&](juce::Colour const& colour)
      {
          if(!mPropertyShadowColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction(true);
          }
          setShadowColour(colour);
          if(!mPropertyShadowColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's shadow color"));
          }
      },
      [&]()
      {
          mDirector.startAction(true);
      },
      [&]()
      {
          mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's shadow color"));
      })
, mPropertyChannelLayout(juce::translate("Channel Layout"), juce::translate("The visible state of the channels."), [this]()
                         {
                             showChannelLayout();
                         })
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::name, Track::AttrType::colours, Track::AttrType::description, Track::AttrType::results})
{
    addAndMakeVisible(mPropertyColourMap);
    addAndMakeVisible(mPropertyForegroundColour);
    addAndMakeVisible(mPropertyTextColour);
    addAndMakeVisible(mPropertyBackgroundColour);
    addAndMakeVisible(mPropertyShadowColour);
    addAndMakeVisible(mPropertyChannelLayout);
}

void Group::PropertyGraphicalsSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyColourMap);
    setBounds(mPropertyForegroundColour);
    setBounds(mPropertyTextColour);
    setBounds(mPropertyBackgroundColour);
    setBounds(mPropertyShadowColour);
    setBounds(mPropertyChannelLayout);
    setSize(getWidth(), bounds.getY());
}

void Group::PropertyGraphicalsSection::setColourMap(Track::ColourMap const& colourMap)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getDisplayType(trackAcsr.get()) != Track::Tools::DisplayType::columns;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.map = colourMap;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setForegroundColour(juce::Colour const& colour)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getDisplayType(trackAcsr.get()) == Track::Tools::DisplayType::columns;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.foreground = colour;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setBackgroundColour(juce::Colour const& colour)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getDisplayType(trackAcsr.get()) == Track::Tools::DisplayType::columns;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.background = colour;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setTextColour(juce::Colour const& colour)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getDisplayType(trackAcsr.get()) == Track::Tools::DisplayType::columns;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.text = colour;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setShadowColour(juce::Colour const& colour)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getDisplayType(trackAcsr.get()) == Track::Tools::DisplayType::columns;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.shadow = colour;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::showChannelLayout()
{
    auto const channelslayout = Tools::getChannelVisibilityStates(mAccessor);
    auto const numChannels = channelslayout.size();
    juce::PopupMenu menu;
    if(numChannels > 2_z)
    {
        auto const oneHidden = std::any_of(channelslayout.cbegin(), channelslayout.cend(), [](auto const state)
                                           {
                                               return state != ChannelVisibilityState::visible;
                                           });
        menu.addItem(juce::translate("All Channels"), oneHidden, !oneHidden, [this]()
                     {
                         for(auto const& trackIdentifer : mAccessor.getAttr<AttrType::layout>())
                         {
                             auto trackAcsr = Group::Tools::getTrackAcsr(mAccessor, trackIdentifer);
                             if(trackAcsr.has_value())
                             {
                                 auto copy = trackAcsr->get().getAttr<Track::AttrType::channelsLayout>();
                                 std::fill(copy.begin(), copy.end(), true);
                                 trackAcsr->get().setAttr<Track::AttrType::channelsLayout>(copy, NotificationType::synchronous);
                             }
                         }
                         showChannelLayout();
                     });

        auto const firstHidden = channelslayout[0_z] != ChannelVisibilityState::visible || std::any_of(std::next(channelslayout.cbegin()), channelslayout.cend(), [](auto const state)
                                                                                                       {
                                                                                                           return state == ChannelVisibilityState::visible;
                                                                                                       });
        menu.addItem(juce::translate("Channel 1 Only"), firstHidden, !firstHidden, [this]()
                     {
                         for(auto const& trackIdentifer : mAccessor.getAttr<AttrType::layout>())
                         {
                             auto trackAcsr = Group::Tools::getTrackAcsr(mAccessor, trackIdentifer);
                             if(trackAcsr.has_value())
                             {
                                 auto copy = trackAcsr->get().getAttr<Track::AttrType::channelsLayout>();
                                 if(!copy.empty())
                                 {
                                     copy[0_z] = true;
                                     std::fill(std::next(copy.begin()), copy.end(), false);
                                     trackAcsr->get().setAttr<Track::AttrType::channelsLayout>(copy, NotificationType::synchronous);
                                 }
                             }
                         }
                         showChannelLayout();
                     });
    }

    auto const colour = juce::Colour::contrasting(findColour(juce::PopupMenu::ColourIds::backgroundColourId), findColour(juce::PopupMenu::ColourIds::textColourId));
    for(size_t channel = 0_z; channel < channelslayout.size(); ++channel)
    {
        juce::PopupMenu::Item item(juce::translate("Channel CHINDEX").replace("CHINDEX", juce::String(channel + 1)));
        item.isEnabled = !channelslayout.empty();
        item.isTicked = channelslayout[channel] != ChannelVisibilityState::hidden;
        if(channelslayout[channel] == ChannelVisibilityState::both)
        {
            item.colour = colour;
        }
        auto const newState = channelslayout[channel] != ChannelVisibilityState::visible ? ChannelVisibilityState::visible : ChannelVisibilityState::hidden;
        item.action = [this, channel, newState]()
        {
            for(auto const& trackIdentifer : mAccessor.getAttr<AttrType::layout>())
            {
                auto trackAcsr = Group::Tools::getTrackAcsr(mAccessor, trackIdentifer);
                if(trackAcsr.has_value())
                {
                    auto copy = trackAcsr->get().getAttr<Track::AttrType::channelsLayout>();
                    if(channel < copy.size())
                    {
                        copy[channel] = newState;
                        if(std::none_of(copy.cbegin(), copy.cend(), [](auto const& state)
                                        {
                                            return state == ChannelVisibilityState::visible;
                                        }))
                        {
                            if(channel + 1_z < copy.size())
                            {
                                copy[channel + 1_z] = ChannelVisibilityState::visible;
                            }
                            else if(channel > 0_z)
                            {
                                copy[channel - 1_z] = ChannelVisibilityState::visible;
                            }
                        }
                        trackAcsr->get().setAttr<Track::AttrType::channelsLayout>(copy, NotificationType::synchronous);
                    }
                }
            }
            showChannelLayout();
        };

        menu.addItem(std::move(item));
    }

    if(!std::exchange(mChannelLayoutActionStarted, true))
    {
        mDirector.startAction(true);
    }
    juce::WeakReference<juce::Component> safePointer(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertyChannelLayout.entry), [=, this](int menuResult)
                       {
                           if(safePointer.get() != nullptr && menuResult == 0 && std::exchange(mChannelLayoutActionStarted, false))
                           {
                               mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group channels layout"));
                           }
                       });
}

void Group::PropertyGraphicalsSection::updateContent()
{
    updateColourMap();
    updateColours();
    mPropertyChannelLayout.setVisible(!Tools::getTrackAcsrs(mAccessor).empty());
    resized();
}

void Group::PropertyGraphicalsSection::updateColourMap()
{
    juce::StringArray trackNames;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    std::set<Track::ColourMap> colourMaps;
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::getDisplayType(trackAcsr.get()) == Track::Tools::DisplayType::columns)
        {
            auto const colourMap = trackAcsr.get().getAttr<Track::AttrType::colours>().map;
            colourMaps.insert(colourMap);
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyColourMap.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The color map used by the graphical renderers of the tracks of the group."));
    mPropertyColourMap.setVisible(!colourMaps.empty());
    if(colourMaps.size() == 1_z)
    {
        mPropertyColourMap.entry.setSelectedId(static_cast<int>(*colourMaps.cbegin()) + 1, juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyColourMap.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
}

void Group::PropertyGraphicalsSection::updateColours()
{
    juce::StringArray trackNames;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    std::set<juce::uint32> foregroundColours;
    std::set<juce::uint32> backgrounColours;
    std::set<juce::uint32> textColours;
    std::set<juce::uint32> shadowColours;
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::getDisplayType(trackAcsr.get()) != Track::Tools::DisplayType::columns)
        {
            auto const& colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
            foregroundColours.insert(colours.foreground.getARGB());
            backgrounColours.insert(colours.background.getARGB());
            textColours.insert(colours.text.getARGB());
            shadowColours.insert(colours.shadow.getARGB());
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyForegroundColour.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The foreground color used by the graphical renderers of the tracks of the group."));
    mPropertyForegroundColour.setVisible(!foregroundColours.empty());
    if(foregroundColours.size() == 1_z)
    {
        mPropertyForegroundColour.entry.setCurrentColour(juce::Colour(*foregroundColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyForegroundColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyBackgroundColour.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The background color used by the graphical renderers of the tracks of the group."));
    mPropertyBackgroundColour.setVisible(!backgrounColours.empty());
    if(backgrounColours.size() == 1_z)
    {
        mPropertyBackgroundColour.entry.setCurrentColour(juce::Colour(*backgrounColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyBackgroundColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyTextColour.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The text color used by the graphical renderers of the tracks of the group."));
    mPropertyTextColour.setVisible(!textColours.empty());
    if(textColours.size() == 1_z)
    {
        mPropertyTextColour.entry.setCurrentColour(juce::Colour(*textColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyTextColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyShadowColour.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The shadow color used by the graphical renderers of the tracks of the group."));
    mPropertyShadowColour.setVisible(!shadowColours.empty());
    if(shadowColours.size() == 1_z)
    {
        mPropertyShadowColour.entry.setCurrentColour(juce::Colour(*shadowColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyShadowColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }
}

ANALYSE_FILE_END
