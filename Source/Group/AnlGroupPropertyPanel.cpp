#include "AnlGroupPropertyPanel.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::PropertyPanel::PropertyPanel(Director& director)
: FloatingWindowContainer("Properties", *this)
, mDirector(director)

, mPropertyName("Name", "The name of the group", [&](juce::String text)
                {
                    mDirector.startAction();
                    mAccessor.setAttr<AttrType::name>(text, NotificationType::synchronous);
                    mDirector.endAction(ActionState::newTransaction, juce::translate("Change track name"));
                })
, mPropertyBackgroundColour(
      "Background Color", "The background current color of the graphical renderer.", "Select the background color", [&](juce::Colour const& colour)
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
, mPropertyChannelLayout("Channel Layout", "The visible state of the channels.", [&]()
                         {
                             showChannelLayout();
                         })
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
    mPropertyName.setBounds(bounds.removeFromTop(mPropertyName.getHeight()));
    mPropertyBackgroundColour.setBounds(bounds.removeFromTop(mPropertyBackgroundColour.getHeight()));
    mPropertyChannelLayout.setBounds(bounds.removeFromTop(mPropertyChannelLayout.getHeight()));
    setSize(sInnerWidth, std::max(bounds.getY(), 120) + 2);
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
    auto const result = menu.showAt(&mPropertyChannelLayout.entry);
    if(result == 0 && std::exchange(mChannelLayoutActionStarted, false))
    {
        mDirector.endAction(ActionState::newTransaction, juce::translate("Change tracks' channels layout"));
        for(auto const& trackIdentifer : mAccessor.getAttr<AttrType::layout>())
        {
            auto trackAcsr = Group::Tools::getTrackAcsr(mAccessor, trackIdentifer);
            if(trackAcsr.has_value())
            {
                mDirector.getTrackDirector(trackIdentifer).endAction(ActionState::continueTransaction);
            }
        }
    }
}

ANALYSE_FILE_END
