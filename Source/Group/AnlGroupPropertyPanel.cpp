#include "AnlGroupPropertyPanel.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::PropertyPanel::PropertyPanel(Director& director)
: FloatingWindowContainer("Properties", mViewport)
, mDirector(director)

, mPropertyName("Name", "The name of the group", [this](juce::String text)
                {
                    mDirector.startAction(false);
                    mAccessor.setAttr<AttrType::name>(text, NotificationType::synchronous);
                    mDirector.endAction(false, ActionState::newTransaction, juce::translate("Change group name"));
                })
, mPropertyBackgroundColour(
      "Background Color", "The background current color of the graphical renderer.", "Select the background color", [this](juce::Colour const& colour)
      {
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction(false);
          }
          mAccessor.setAttr<AttrType::colour>(colour, NotificationType::synchronous);
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(false, ActionState::newTransaction, juce::translate("Change group background color"));
          }
      },
      [&]()
      {
          mDirector.startAction(false);
      },
      [&]()
      {
          mDirector.endAction(false, ActionState::newTransaction, juce::translate("Change group background color"));
      })
, mPropertyZoomTrack("Zoom Track", "The selected track used for the zoom", "", std::vector<std::string>{""}, [this](size_t index)
                     {
                         mDirector.startAction(false);
                         auto const layout = mAccessor.getAttr<AttrType::layout>();
                         if(index == 0_z || index - 1_z >= layout.size())
                         {
                             mAccessor.setAttr<AttrType::zoomid>("", NotificationType::synchronous);
                         }
                         else
                         {
                             mAccessor.setAttr<AttrType::zoomid>(layout[index - 1_z], NotificationType::synchronous);
                         }
                         mDirector.endAction(false, ActionState::newTransaction, "Change group zoom");
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
    addAndMakeVisible(mProcessorsSection);

    mComponentListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        resized();
    };
    mComponentListener.attachTo(mProcessorsSection);
    mProcessorsSection.setComponents({mPropertyProcessorsSection});

    setSize(300, 400);
    mViewport.setSize(getWidth() + mViewport.getVerticalScrollBar().getWidth() + 2, 400);
    mViewport.setScrollBarsShown(true, false, true, false);
    mViewport.setViewedComponent(this, false);
    mFloatingWindow.setResizable(true, false);
    mBoundsConstrainer.setMinimumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMaximumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMinimumHeight(120);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::PropertyPanel::~PropertyPanel()
{
    mComponentListener.detachFrom(mProcessorsSection);
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
    setBounds(mProcessorsSection);
    setSize(getWidth(), std::max(bounds.getY(), 120) + 2);
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
    for(auto layoutIndex = 0_z; layoutIndex < layout.size(); ++layoutIndex)
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

ANALYSE_FILE_END
