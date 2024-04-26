#include "AnlGroupPropertyPanel.h"
#include "../Track/AnlTrackTools.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::PropertyPanel::WindowContainer::WindowContainer(PropertyPanel& propertyPanel)
: FloatingWindowContainer(juce::translate("Properties"), mViewport)
, mPropertyPanel(propertyPanel)
{
    mViewport.setSize(mPropertyPanel.getWidth() + mViewport.getVerticalScrollBar().getWidth() + 2, 400);
    mViewport.setScrollBarsShown(true, false, true, false);
    mViewport.setViewedComponent(&mPropertyPanel, false);
    mFloatingWindow.setResizable(true, false);
    mBoundsConstrainer.setMinimumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMaximumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMinimumHeight(120);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);
}

Group::PropertyPanel::PropertyPanel(Director& director)
: mDirector(director)

, mPropertyName(juce::translate("Group Name"), juce::translate("The name of the group"), [this](juce::String text)
                {
                    mDirector.startAction(false);
                    mAccessor.setAttr<AttrType::name>(text, NotificationType::synchronous);
                    mDirector.endAction(false, ActionState::newTransaction, juce::translate("Change group name"));
                })
, mPropertyBackgroundColour(juce::translate("Group Background Color"), juce::translate("The background color of the group."), juce::translate("Select the background color"), [this](juce::Colour const& colour)
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
, mPropertyReferenceTrack(juce::translate("Group Track Reference"), juce::translate("The track used for the edition and the navigation"), "", std::vector<std::string>{""}, [this](size_t index)
                          {
                              mDirector.startAction(false);
                              auto const layout = mAccessor.getAttr<AttrType::layout>();
                              if(index == 0_z || index - 1_z >= layout.size())
                              {
                                  mAccessor.setAttr<AttrType::referenceid>("", NotificationType::synchronous);
                              }
                              else
                              {
                                  mAccessor.setAttr<AttrType::referenceid>(layout[index - 1_z], NotificationType::synchronous);
                              }
                              mDirector.endAction(false, ActionState::newTransaction, juce::translate("Change track reference of the group"));
                          })
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::name, Track::AttrType::description, Track::AttrType::results})
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            {
                mPropertyName.entry.setText(mAccessor.getAttr<AttrType::name>(), juce::NotificationType::dontSendNotification);
                parentHierarchyChanged();
            }
            break;
            case AttrType::colour:
            {
                mPropertyBackgroundColour.entry.setCurrentColour(acsr.getAttr<AttrType::colour>(), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::referenceid:
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
    addAndMakeVisible(mPropertyReferenceTrack);
    addAndMakeVisible(mProcessorsSection);
    addAndMakeVisible(mGraphicalsSection);
    addAndMakeVisible(mOscSection);

    mComponentListener.onComponentResized = [&]([[maybe_unused]] juce::Component& component)
    {
        mProcessorsSection.setEnabled(mPropertyProcessorsSection.getHeight() > 0);
        mGraphicalsSection.setEnabled(mPropertyGraphicalsSection.getHeight() > 0);
        mGraphicalsSection.setEnabled(mPropertyOscSection.getHeight() > 0);
        resized();
    };

    mComponentListener.attachTo(mProcessorsSection);
    mComponentListener.attachTo(mGraphicalsSection);
    mComponentListener.attachTo(mOscSection);
    mProcessorsSection.setComponents({mPropertyProcessorsSection});
    mGraphicalsSection.setComponents({mPropertyGraphicalsSection});
    mOscSection.setComponents({mPropertyOscSection});

    setSize(300, 400);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::PropertyPanel::~PropertyPanel()
{
    mComponentListener.detachFrom(mGraphicalsSection);
    mComponentListener.detachFrom(mProcessorsSection);
    mComponentListener.detachFrom(mOscSection);
    mAccessor.removeListener(mListener);
}

void Group::PropertyPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyName);
    setBounds(mPropertyBackgroundColour);
    setBounds(mPropertyReferenceTrack);
    setBounds(mProcessorsSection);
    setBounds(mGraphicalsSection);
    setBounds(mOscSection);
    setSize(getWidth(), std::max(bounds.getY(), 120) + 2);
}

void Group::PropertyPanel::updateContent()
{
    auto const zoomId = mAccessor.getAttr<AttrType::referenceid>();
    auto& entry = mPropertyReferenceTrack.entry;
    entry.clear(juce::NotificationType::dontSendNotification);
    entry.addItem(juce::translate("Front"), 1);
    entry.addSeparator();

    auto const layout = mAccessor.getAttr<AttrType::layout>();
    for(auto layoutIndex = 0_z; layoutIndex < layout.size(); ++layoutIndex)
    {
        auto const trackId = layout.at(layoutIndex);
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, trackId);
        if(trackAcsr.has_value())
        {
            auto const& trackName = trackAcsr->get().getAttr<Track::AttrType::name>();
            auto const itemLabel = trackName.isEmpty() ? juce::translate("Track IDX").replace("IDX", juce::String(layoutIndex + 1)) : trackName;
            entry.addItem(itemLabel, static_cast<int>(layoutIndex) + 2);
        }
    }

    auto const it = std::find(layout.cbegin(), layout.cend(), zoomId);
    if(!zoomId.isEmpty() && it != layout.cend())
    {
        entry.setSelectedId(static_cast<int>(std::distance(layout.cbegin(), it)) + 2, juce::NotificationType::dontSendNotification);
    }
    else
    {
        entry.setSelectedId(1, juce::NotificationType::dontSendNotification);
    }

    mPropertyReferenceTrack.setEnabled(!layout.empty());

    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    auto const hasNoColumns = std::none_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcrs)
                                           {
                                               return Track::Tools::getFrameType(trackAcrs.get()) == Track::FrameType::vector;
                                           });
    mPropertyBackgroundColour.setVisible(!layout.empty() && hasNoColumns);
    resized();
}

void Group::PropertyPanel::parentHierarchyChanged()
{
    if(auto* window = findParentComponentOfClass<juce::DialogWindow>())
    {
        window->setName(juce::translate("ANLNAME PROPERTIES").replace("ANLNAME", mAccessor.getAttr<AttrType::name>().toUpperCase()));
    }
}

ANALYSE_FILE_END
