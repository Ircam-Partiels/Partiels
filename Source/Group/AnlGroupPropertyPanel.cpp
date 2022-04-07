#include "AnlGroupPropertyPanel.h"
#include "../Track/AnlTrackTools.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::PropertyPanel::PropertyPanel(Director& director)
: FloatingWindowContainer("Properties", mViewport)
, mDirector(director)

, mPropertyName(juce::translate("Group Name"), juce::translate("The name of the group"), [this](juce::String text)
                {
                    mDirector.startAction(false);
                    mAccessor.setAttr<AttrType::name>(text, NotificationType::synchronous);
                    mDirector.endAction(false, ActionState::newTransaction, juce::translate("Change group name"));
                })
, mPropertyBackgroundColour(
      juce::translate("Group Background Color"), juce::translate("The background color of the group."), juce::translate("Select the background color"), [this](juce::Colour const& colour)
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
, mPropertyZoomTrack(juce::translate("Group Zoom Reference"), juce::translate("The selected track used for the zoom"), "", std::vector<std::string>{""}, [this](size_t index)
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
                         mDirector.endAction(false, ActionState::newTransaction, juce::translate("Change group zoom"));
                     })
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::name, Track::AttrType::description, Track::AttrType::results})
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
    addAndMakeVisible(mPropertyZoomTrack);
    addAndMakeVisible(mProcessorsSection);
    addAndMakeVisible(mGraphicalsSection);

    mComponentListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        mProcessorsSection.setEnabled(mPropertyProcessorsSection.getHeight() > 0);
        mGraphicalsSection.setEnabled(mPropertyGraphicalsSection.getHeight() > 0);
        resized();
    };

    mComponentListener.attachTo(mProcessorsSection);
    mComponentListener.attachTo(mGraphicalsSection);
    mProcessorsSection.setComponents({mPropertyProcessorsSection});
    mGraphicalsSection.setComponents({mPropertyGraphicalsSection});

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
    mFloatingWindow.clearContentComponent();
    mComponentListener.detachFrom(mGraphicalsSection);
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
    setBounds(mProcessorsSection);
    setBounds(mGraphicalsSection);
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
    mPropertyZoomTrack.setEnabled(!layout.empty());

    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    auto const hasNoColumns = std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcrs)
                                           {
                                               return Track::Tools::getDisplayType(trackAcrs.get()) == Track::Tools::DisplayType::columns;
                                           });
    mPropertyBackgroundColour.setVisible(!layout.empty() && hasNoColumns);
    resized();
}

ANALYSE_FILE_END
