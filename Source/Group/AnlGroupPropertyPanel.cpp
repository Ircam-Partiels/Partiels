#include "AnlGroupPropertyPanel.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::PropertyPanel::PropertyPanel(Director& director)
: FloatingWindowContainer("Properties", *this)
, mDirector(director)

, mPropertyName("Name", "The name of the group", [this](juce::String text)
                {
                    mDirector.startAction();
                    mAccessor.setAttr<AttrType::name>(text, NotificationType::synchronous);
                    mDirector.endAction(ActionState::newTransaction, juce::translate("Change group name"));
                })
, mPropertyBackgroundColour(
      "Background Color", "The background current color of the graphical renderer.", "Select the background color", [this](juce::Colour const& colour)
      {
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction();
          }
          mAccessor.setAttr<AttrType::colour>(colour, NotificationType::synchronous);
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(ActionState::newTransaction, juce::translate("Change group background color"));
          }
      },
      [&]()
      {
          mDirector.startAction();
      },
      [&]()
      {
          mDirector.endAction(ActionState::newTransaction, juce::translate("Change group background color"));
      })
, mPropertyZoomTrack("Zoom Track", "The selected track used for the zoom", "", std::vector<std::string>{""}, [this](size_t index)
                     {
                         mDirector.startAction();
                         auto const layout = mAccessor.getAttr<AttrType::layout>();
                         if(index == 0_z || index - 1_z >= layout.size())
                         {
                             mAccessor.setAttr<AttrType::zoomid>("", NotificationType::synchronous);
                         }
                         else
                         {
                             mAccessor.setAttr<AttrType::zoomid>(layout[index - 1_z], NotificationType::synchronous);
                         }
                         mDirector.endAction(ActionState::newTransaction, "Change group zoom");
                     })
, mPropertyChannelLayout("Channel Layout", "The visible state of the channels.", [this]()
                         {
                             showChannelLayout();
                         })
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::name})
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        auto constexpr silent = juce::NotificationType::dontSendNotification;
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            {
                mPropertyName.entry.setText(acsr.getAttr<AttrType::name>(), silent);
                mFloatingWindow.setName(juce::translate("ANLNAME PROPERTIES").replace("ANLNAME", acsr.getAttr<AttrType::name>().toUpperCase()));
            }
            break;
            case AttrType::colour:
            {
                mPropertyBackgroundColour.entry.setCurrentColour(acsr.getAttr<AttrType::colour>(), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::zoomid:
            {
                updateContent();
            }
            break;
            case AttrType::height:
            case AttrType::expanded:
            case AttrType::layout:
            case AttrType::tracks:
            case AttrType::focused:
                break;
        }
    };

    addAndMakeVisible(mPropertyName);
    addAndMakeVisible(mPropertyBackgroundColour);
    addAndMakeVisible(mPropertyChannelLayout);
    addAndMakeVisible(mPropertyZoomTrack);
    mFloatingWindow.setResizable(false, false);
    setSize(sInnerWidth, 400);

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::PropertyPanel::~PropertyPanel()
{
    mAccessor.removeListener(mListener);
}

void Group::PropertyPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyName);
    setBounds(mPropertyBackgroundColour);
    setBounds(mPropertyZoomTrack);
    setBounds(mPropertyChannelLayout);
    setSize(sInnerWidth, std::max(bounds.getY(), 120) + 2);
}

void Group::PropertyPanel::updateContent()
{
    auto const zoomId = mAccessor.getAttr<AttrType::zoomid>();
    auto const currentTrack = Tools::getTrackAcsr(mAccessor, zoomId);
    auto& entry = mPropertyZoomTrack.entry;
    entry.clear(juce::NotificationType::dontSendNotification);
    entry.addItem(juce::translate("Front"), 1);
    if(!currentTrack.has_value())
    {
        entry.setSelectedId(1, juce::NotificationType::dontSendNotification);
    }

    auto const layout = mAccessor.getAttr<AttrType::layout>();
    for(size_t layoutIndex = 0_z; layoutIndex < layout.size(); ++layoutIndex)
    {
        auto const trackId = layout[layoutIndex];
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, trackId);
        if(trackAcsr.has_value())
        {
            auto const& trackName = trackAcsr->get().getAttr<Track::AttrType::name>();
            if(!trackName.isEmpty())
            {
                entry.addItem(trackName, static_cast<int>(layoutIndex) + 2);
                if(trackId == zoomId)
                {
                    entry.setSelectedId(static_cast<int>(layoutIndex) + 2, juce::NotificationType::dontSendNotification);
                }
            }
        }
    }
}

void Group::PropertyPanel::showChannelLayout()
{
    std::vector<int> channelslayout;
    for(auto const& trackIdentifer : mAccessor.getAttr<AttrType::layout>())
    {
        auto trackAcsr = Group::Tools::getTrackAcsr(mAccessor, trackIdentifer);
        if(trackAcsr.has_value())
        {
            auto const& trackChannelsLayout = trackAcsr->get().getAttr<Track::AttrType::channelsLayout>();
            for(size_t i = 0; i < channelslayout.size(); ++i)
            {
                if(i < trackChannelsLayout.size())
                {
                    if(channelslayout[i] != static_cast<int>(trackChannelsLayout[i]))
                    {
                        channelslayout[i] = -1;
                    }
                }
            }
            for(size_t i = channelslayout.size(); i < trackChannelsLayout.size(); ++i)
            {
                channelslayout.push_back(trackChannelsLayout[i] ? 1 : 0);
            }
        }
    }

    auto const numChannels = channelslayout.size();
    juce::PopupMenu menu;
    if(numChannels > 2_z)
    {
        auto const allActive = std::any_of(channelslayout.cbegin(), channelslayout.cend(), [](int state)
                                           {
                                               return state < 1;
                                           });
        menu.addItem(juce::translate("All Channels"), allActive, !allActive, [this]()
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

        auto const oneActive = channelslayout[0_z] != 1 || std::any_of(std::next(channelslayout.cbegin()), channelslayout.cend(), [](int state)
                                                                       {
                                                                           return state == 1;
                                                                       });
        menu.addItem(juce::translate("Channel 1 Only"), oneActive, !oneActive, [this]()
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
        item.isTicked = channelslayout[channel] != 0;
        if(channelslayout[channel] == -1)
        {
            item.colour = colour;
        }
        item.action = [this, channel, newState = channelslayout[channel] != 1]()
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
                                            return state == true;
                                        }))
                        {
                            if(channel + 1_z < copy.size())
                            {
                                copy[channel + 1_z] = true;
                            }
                            else if(channel > 0_z)
                            {
                                copy[channel - 1_z] = true;
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
        mDirector.startAction();
        for(auto const& trackIdentifer : mAccessor.getAttr<AttrType::layout>())
        {
            auto trackAcsr = Group::Tools::getTrackAcsr(mAccessor, trackIdentifer);
            if(trackAcsr.has_value())
            {
                mDirector.getTrackDirector(trackIdentifer).startAction();
            }
        }
    }
    juce::WeakReference<juce::Component> safePointer(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertyChannelLayout.entry), [=, this](int menuResult)
                       {
                           if(safePointer.get() != nullptr && menuResult == 0 && std::exchange(mChannelLayoutActionStarted, false))
                           {
                               mDirector.endAction(ActionState::newTransaction, juce::translate("Change group channels layout"));
                               for(auto const& trackIdentifer : mAccessor.getAttr<AttrType::layout>())
                               {
                                   auto trackAcsr = Group::Tools::getTrackAcsr(mAccessor, trackIdentifer);
                                   if(trackAcsr.has_value())
                                   {
                                       mDirector.getTrackDirector(trackIdentifer).endAction(ActionState::continueTransaction);
                                   }
                               }
                           }
                       });
}

ANALYSE_FILE_END
