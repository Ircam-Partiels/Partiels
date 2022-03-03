#include "AnlTrackPropertyGraphicalSection.h"
#include "AnlTrackTools.h"

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

Track::PropertyGraphicalSection::PropertyGraphicalSection(Director& director)
: mDirector(director)
, mPropertyColourMap(juce::translate("Color Map"), juce::translate("The color map of the graphical renderer."), "", getColourMapNames(), [&](size_t index)
                     {
                         mDirector.startAction();
                         setColourMap(static_cast<ColourMap>(index));
                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change track color map"));
                     })
, mPropertyForegroundColour(
      juce::translate("Foreground Color"), juce::translate("The foreground color of the graphical renderer."), juce::translate("Select the foreground color"), [&](juce::Colour const& colour)
      {
          if(!mPropertyForegroundColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction();
          }
          setForegroundColour(colour);
          if(!mPropertyForegroundColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(ActionState::newTransaction, juce::translate("Change track foreground color"));
          }
      },
      [&]()
      {
          mDirector.startAction();
      },
      [&]()
      {
          mDirector.endAction(ActionState::newTransaction, juce::translate("Change track foreground color"));
      })
, mPropertyBackgroundColour(
      juce::translate("Background Color"), juce::translate("The background color of the graphical renderer."), juce::translate("Select the background color"), [&](juce::Colour const& colour)
      {
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction();
          }
          setBackgroundColour(colour);
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(ActionState::newTransaction, juce::translate("Change track background color"));
          }
      },
      [&]()
      {
          mDirector.startAction();
      },
      [&]()
      {
          mDirector.endAction(ActionState::newTransaction, juce::translate("Change track background color"));
      })
, mPropertyTextColour(
      juce::translate("Text Color"), juce::translate("The text color of the graphical renderer."), juce::translate("Select the text color"), [&](juce::Colour const& colour)
      {
          if(!mPropertyTextColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction();
          }
          setTextColour(colour);
          if(!mPropertyTextColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(ActionState::newTransaction, juce::translate("Change track text color"));
          }
      },
      [&]()
      {
          mDirector.startAction();
      },
      [&]()
      {
          mDirector.endAction(ActionState::newTransaction, juce::translate("Change track text color"));
      })
, mPropertyShadowColour(
      juce::translate("Shadow Color"), juce::translate("The shadow color of the graphical renderer."), juce::translate("Select the shadow color"), [&](juce::Colour const& colour)
      {
          if(!mPropertyShadowColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction();
          }
          setShadowColour(colour);
          if(!mPropertyShadowColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(ActionState::newTransaction, juce::translate("Change track shadow color"));
          }
      },
      [&]()
      {
          mDirector.startAction();
      },
      [&]()
      {
          mDirector.endAction(ActionState::newTransaction, juce::translate("Change track shadow color"));
      })
, mPropertyValueRangeMode(juce::translate("Value Range Mode"), juce::translate("The mode of the value range."), "", std::vector<std::string>{"Default", "Results", "Manual"}, [&](size_t index)
                          {
                              switch(index)
                              {
                                  case 0:
                                  {
                                      mDirector.startAction();
                                      setPluginValueRange();
                                      mDirector.endAction(ActionState::newTransaction, juce::translate("Change track minimum and maximum values"));
                                  }
                                  break;
                                  case 1:
                                  {
                                      mDirector.startAction();
                                      setResultValueRange();
                                      mDirector.endAction(ActionState::newTransaction, juce::translate("Change track minimum and maximum values"));
                                  }
                                  break;
                                  default:
                                      break;
                              }
                          })
, mPropertyValueRangeMin(juce::translate("Value Range Min."), juce::translate("The minimum value of the output."), "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float value)
                         {
                             mDirector.startAction();
                             setValueRangeMin(static_cast<double>(value));
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track minimum value"));
                         })
, mPropertyValueRangeMax(juce::translate("Value Range Max."), juce::translate("The maximum value of the output."), "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float value)
                         {
                             mDirector.startAction();
                             setValueRangeMax(static_cast<double>(value));
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track maximum value"));
                         })
, mPropertyValueRange(juce::translate("Value Range"), juce::translate("The range of the output."), "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float min, float max)
                      {
                          mDirector.startAction();
                          setValueRange(Zoom::Range(min, max));
                          mDirector.endAction(ActionState::newTransaction, juce::translate("Change track value range"));
                      })
, mPropertyGrid(juce::translate("Grid"), juce::translate("Edit the grid properties"), [&]()
                {
                    auto& zoomAcsr = getCurrentZoomAcsr();
                    auto& gridAcsr = zoomAcsr.getAcsr<Zoom::AcsrType::grid>();
                    mZoomGridPropertyPanel.setGrid(gridAcsr);
                    mZoomGridPropertyPanel.show();
                })
, mPropertyRangeLink(juce::translate("Range Link"), juce::translate("Toggle the group link for zoom range."), [&](bool value)
                     {
                         mDirector.startAction();
                         mAccessor.setAttr<AttrType::zoomLink>(value, NotificationType::synchronous);
                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change range link state"));
                     })
, mPropertyNumBins(juce::translate("Num Bins"), juce::translate("The number of bins."), "", {0.0f, static_cast<float>(Zoom::max())}, 1.0f, nullptr)
, mPropertyChannelLayout(juce::translate("Channel Layout"), juce::translate("The visible state of the channels."), [&]()
                         {
                             showChannelLayout();
                         })
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::description:
            case AttrType::results:
            {
                for(auto* child : getChildren())
                {
                    MiscWeakAssert(child != nullptr);
                    if(child != nullptr)
                    {
                        child->setVisible(false);
                    }
                }
                switch(Tools::getDisplayType(acsr))
                {
                    case Tools::DisplayType::markers:
                    {
                        mPropertyForegroundColour.setVisible(true);
                        mPropertyTextColour.setVisible(true);
                        mPropertyBackgroundColour.setVisible(true);
                        mPropertyShadowColour.setVisible(true);
                        mPropertyChannelLayout.setVisible(true);
                    }
                    break;
                    case Tools::DisplayType::points:
                    {
                        auto const& output = acsr.getAttr<AttrType::description>().output;
                        mPropertyValueRangeMin.entry.setTextValueSuffix(output.unit);
                        mPropertyValueRangeMax.entry.setTextValueSuffix(output.unit);
                        mPropertyRangeLink.title.setText(juce::translate("Value Range Link"), juce::NotificationType::dontSendNotification);
                        mPropertyForegroundColour.setVisible(true);
                        mPropertyTextColour.setVisible(true);
                        mPropertyBackgroundColour.setVisible(true);
                        mPropertyShadowColour.setVisible(true);
                        mPropertyValueRangeMode.setVisible(true);
                        mPropertyValueRangeMin.setVisible(true);
                        mPropertyValueRangeMax.setVisible(true);
                        mPropertyRangeLink.setVisible(true);
                        mPropertyGrid.setVisible(true);
                        mPropertyChannelLayout.setVisible(true);
                    }
                    break;
                    case Tools::DisplayType::columns:
                    {
                        auto const& output = acsr.getAttr<AttrType::description>().output;
                        mPropertyValueRangeMin.entry.setTextValueSuffix(output.unit);
                        mPropertyValueRangeMax.entry.setTextValueSuffix(output.unit);
                        mPropertyRangeLink.title.setText(juce::translate("Bin Range Link"), juce::NotificationType::dontSendNotification);
                        mPropertyColourMap.setVisible(true);
                        mPropertyValueRangeMode.setVisible(true);
                        mPropertyValueRangeMin.setVisible(true);
                        mPropertyValueRangeMax.setVisible(true);
                        mPropertyValueRange.setVisible(true);
                        mPropertyNumBins.setVisible(true);
                        mPropertyRangeLink.setVisible(true);
                        mPropertyGrid.setVisible(true);
                        mPropertyChannelLayout.setVisible(true);
                        mProgressBarRendering.setVisible(true);
                    }
                    break;
                }
                updateZoomMode();
                resized();
            }
            break;
            case AttrType::colours:
            {
                auto const colours = acsr.getAttr<AttrType::colours>();
                mPropertyBackgroundColour.entry.setCurrentColour(colours.background, juce::NotificationType::dontSendNotification);
                mPropertyForegroundColour.entry.setCurrentColour(colours.foreground, juce::NotificationType::dontSendNotification);
                mPropertyTextColour.entry.setCurrentColour(colours.text, juce::NotificationType::dontSendNotification);
                mPropertyShadowColour.entry.setCurrentColour(colours.shadow, juce::NotificationType::dontSendNotification);
                mPropertyColourMap.entry.setSelectedItemIndex(static_cast<int>(colours.map), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::zoomLink:
            {
                mPropertyRangeLink.entry.setToggleState(acsr.getAttr<AttrType::zoomLink>(), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::name:
            case AttrType::key:
            case AttrType::file:
            case AttrType::state:
            case AttrType::graphics:
            case AttrType::processing:
            case AttrType::warnings:
            case AttrType::channelsLayout:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::zoomAcsr:
            case AttrType::focused:
            case AttrType::grid:
                break;
        }
    };

    mValueZoomListener.onAttrChanged = [=, this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::minimumLength:
            {
                auto const range = acsr.getAttr<Zoom::AttrType::globalRange>();
                auto const isEnable = !range.isEmpty();
                mPropertyValueRangeMode.entry.setEnabled(isEnable);
                mPropertyValueRangeMin.entry.setEnabled(isEnable);
                mPropertyValueRangeMax.entry.setEnabled(isEnable);
                mPropertyValueRange.entry.setEnabled(isEnable);
                if(isEnable)
                {
                    anlWeakAssert(std::isfinite(range.getStart()) && std::isfinite(range.getEnd()));
                    auto const interval = acsr.getAttr<Zoom::AttrType::minimumLength>();
                    mPropertyValueRangeMin.entry.setValue(range.getStart(), juce::NotificationType::dontSendNotification);
                    mPropertyValueRangeMax.entry.setValue(range.getEnd(), juce::NotificationType::dontSendNotification);
                    mPropertyValueRange.entry.setRange(range.getStart(), range.getEnd(), interval);
                }

                updateZoomMode();
                mPropertyValueRange.repaint();
            }
            break;
            case Zoom::AttrType::visibleRange:
            {
                auto const range = acsr.getAttr<Zoom::AttrType::visibleRange>();
                mPropertyValueRange.entry.setMinAndMaxValues(range.getStart(), range.getEnd(), juce::NotificationType::dontSendNotification);
            }
            break;
            case Zoom::AttrType::anchor:
                break;
        }
    };

    mBinZoomListener.onAttrChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            {
                auto const range = acsr.getAttr<Zoom::AttrType::globalRange>();
                mPropertyNumBins.entry.setValue(range.getEnd(), juce::NotificationType::dontSendNotification);
            }
            break;
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::visibleRange:
            case Zoom::AttrType::anchor:
                break;
        }
    };

    mZoomGridPropertyPanel.onChangeBegin = [this](Zoom::Grid::Accessor const& acsr)
    {
        juce::ignoreUnused(acsr);
        mDirector.startAction();
    };

    mZoomGridPropertyPanel.onChangeEnd = [this](Zoom::Grid::Accessor const& acsr)
    {
        mDirector.startAction();
        auto& zoomAcsr = getCurrentZoomAcsr();
        auto& gridAcsr = zoomAcsr.getAcsr<Zoom::AcsrType::grid>();
        gridAcsr.copyFrom(acsr, NotificationType::synchronous);
        mDirector.endAction(ActionState::newTransaction, juce::translate("Edit Grid Properties"));
    };

    mPropertyNumBins.entry.setEnabled(false);
    mProgressBarRendering.setSize(300, 36);
    addAndMakeVisible(mPropertyColourMap);
    addAndMakeVisible(mPropertyForegroundColour);
    addAndMakeVisible(mPropertyTextColour);
    addAndMakeVisible(mPropertyBackgroundColour);
    addAndMakeVisible(mPropertyShadowColour);
    addAndMakeVisible(mPropertyValueRangeMode);
    addAndMakeVisible(mPropertyValueRangeMin);
    addAndMakeVisible(mPropertyValueRangeMax);
    addAndMakeVisible(mPropertyValueRange);
    addAndMakeVisible(mPropertyNumBins);
    addAndMakeVisible(mPropertyRangeLink);
    addAndMakeVisible(mPropertyGrid);
    addAndMakeVisible(mPropertyChannelLayout);
    addAndMakeVisible(mProgressBarRendering);
    setSize(300, 400);

    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mBinZoomListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::PropertyGraphicalSection::~PropertyGraphicalSection()
{
    mAccessor.removeListener(mListener);
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mBinZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mValueZoomListener);
}

void Track::PropertyGraphicalSection::resized()
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
    setBounds(mPropertyValueRangeMode);
    setBounds(mPropertyValueRangeMin);
    setBounds(mPropertyValueRangeMax);
    setBounds(mPropertyValueRange);
    setBounds(mPropertyNumBins);
    setBounds(mPropertyRangeLink);
    setBounds(mPropertyGrid);
    setBounds(mPropertyChannelLayout);
    setBounds(mProgressBarRendering);
    setSize(getWidth(), bounds.getY());
}

Zoom::Accessor& Track::PropertyGraphicalSection::getCurrentZoomAcsr()
{
    return Tools::getDisplayType(mAccessor) == Tools::DisplayType::columns ? mAccessor.getAcsr<AcsrType::binZoom>() : mAccessor.getAcsr<AcsrType::valueZoom>();
}

void Track::PropertyGraphicalSection::setColourMap(ColourMap const& colourMap)
{
    auto colours = mAccessor.getAttr<AttrType::colours>();
    colours.map = colourMap;
    mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setForegroundColour(juce::Colour const& colour)
{
    auto colours = mAccessor.getAttr<AttrType::colours>();
    colours.foreground = colour;
    mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setBackgroundColour(juce::Colour const& colour)
{
    auto colours = mAccessor.getAttr<AttrType::colours>();
    colours.background = colour;
    mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setTextColour(juce::Colour const& colour)
{
    auto colours = mAccessor.getAttr<AttrType::colours>();
    colours.text = colour;
    mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setShadowColour(juce::Colour const& colour)
{
    auto colours = mAccessor.getAttr<AttrType::colours>();
    colours.shadow = colour;
    mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setPluginValueRange()
{
    auto globalRange = Tools::getValueRange(mAccessor.getAttr<AttrType::description>());
    if(!globalRange.has_value() || globalRange->isEmpty())
    {
        return;
    }
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    auto const visibleRange = Zoom::Tools::getScaledVisibleRange(zoomAcsr, *globalRange);
    zoomAcsr.setAttr<Zoom::AttrType::globalRange>(*globalRange, NotificationType::synchronous);
    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setResultValueRange()
{
    auto globalRange = Tools::getValueRange(mAccessor.getAttr<AttrType::results>());
    if(!globalRange.has_value() || globalRange->isEmpty())
    {
        return;
    }
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    auto const visibleRange = Zoom::Tools::getScaledVisibleRange(zoomAcsr, *globalRange);
    zoomAcsr.setAttr<Zoom::AttrType::globalRange>(*globalRange, NotificationType::synchronous);
    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setValueRangeMin(double value)
{
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    auto const end = std::max(zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getEnd(), value + 1.0);
    zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{value, end}, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setValueRangeMax(double value)
{
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    auto const start = std::min(zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getStart(), value - 1.0);
    zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{start, value}, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setValueRange(juce::Range<double> const& range)
{
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::showChannelLayout()
{
    auto const channelsLayout = mAccessor.getAttr<AttrType::channelsLayout>();
    auto const numChannels = channelsLayout.size();
    auto const numVisibleChannels = static_cast<size_t>(std::count(channelsLayout.cbegin(), channelsLayout.cend(), true));
    juce::PopupMenu menu;
    juce::WeakReference<juce::Component> safePointer(this);
    if(numChannels > 2_z)
    {
        auto const allActive = numChannels != numVisibleChannels;
        menu.addItem(juce::translate("All Channels"), allActive, !allActive, [=, this]()
                     {
                         if(safePointer.get() == nullptr)
                         {
                             return;
                         }
                         auto copy = mAccessor.getAttr<AttrType::channelsLayout>();
                         std::fill(copy.begin(), copy.end(), true);
                         mAccessor.setAttr<AttrType::channelsLayout>(copy, NotificationType::synchronous);
                         showChannelLayout();
                     });

        auto const oneActive = !channelsLayout[0_z] || numVisibleChannels > 1_z;
        menu.addItem(juce::translate("Channel 1 Only"), oneActive, !oneActive, [=, this]()
                     {
                         if(safePointer.get() == nullptr)
                         {
                             return;
                         }
                         auto copy = mAccessor.getAttr<AttrType::channelsLayout>();
                         copy[0_z] = true;
                         std::fill(std::next(copy.begin()), copy.end(), false);
                         mAccessor.setAttr<AttrType::channelsLayout>(copy, NotificationType::synchronous);
                         showChannelLayout();
                     });
    }
    for(size_t channel = 0_z; channel < channelsLayout.size(); ++channel)
    {
        menu.addItem(juce::translate("Channel CHINDEX").replace("CHINDEX", juce::String(channel + 1)), numChannels > 1_z, channelsLayout[channel], [=, this]()
                     {
                         if(safePointer.get() == nullptr)
                         {
                             return;
                         }
                         auto copy = mAccessor.getAttr<AttrType::channelsLayout>();
                         copy[channel] = !copy[channel];
                         if(std::none_of(copy.cbegin(), copy.cend(), [](auto const& state)
                                         {
                                             return state == true;
                                         }))
                         {
                             if(channel < copy.size() - 1_z)
                             {
                                 copy[channel + 1_z] = true;
                             }
                             else
                             {
                                 copy[channel - 1_z] = true;
                             }
                         }
                         mAccessor.setAttr<AttrType::channelsLayout>(copy, NotificationType::synchronous);
                         showChannelLayout();
                     });
    }
    if(!std::exchange(mChannelLayoutActionStarted, true))
    {
        mDirector.startAction();
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertyChannelLayout.entry), [=, this](int menuResult)
                       {
                           if(safePointer.get() != nullptr && menuResult == 0 && std::exchange(mChannelLayoutActionStarted, false))
                           {
                               mDirector.endAction(ActionState::newTransaction, juce::translate("Change track channels layout"));
                           }
                       });
}

void Track::PropertyGraphicalSection::updateZoomMode()
{
    auto const& valueZoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    auto const range = valueZoomAcsr.getAttr<Zoom::AttrType::globalRange>();
    anlWeakAssert(std::isfinite(range.getStart()) && std::isfinite(range.getEnd()));
    auto const pluginRange = Tools::getValueRange(mAccessor.getAttr<AttrType::description>());
    auto const resultsRange = Tools::getValueRange(mAccessor.getAttr<AttrType::results>());
    mPropertyValueRangeMode.entry.setItemEnabled(1, pluginRange.has_value());
    mPropertyValueRangeMode.entry.setItemEnabled(2, resultsRange.has_value());
    mPropertyValueRangeMode.entry.setItemEnabled(3, false);
    if(pluginRange.has_value() && !range.isEmpty() && range == *pluginRange)
    {
        mPropertyValueRangeMode.entry.setSelectedId(1, juce::NotificationType::dontSendNotification);
    }
    else if(resultsRange.has_value() && !range.isEmpty() && range == *resultsRange)
    {
        mPropertyValueRangeMode.entry.setSelectedId(2, juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyValueRangeMode.entry.setSelectedId(3, juce::NotificationType::dontSendNotification);
    }
}

ANALYSE_FILE_END
