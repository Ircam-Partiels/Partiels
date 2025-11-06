#include "AnlTrackPropertyGraphicalSection.h"
#include "AnlTrackExporter.h"
#include "AnlTrackTools.h"
#include "AnlTrackTooltip.h"

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

static std::vector<std::string> getFontNames()
{
    static auto typefaceNames = juce::Font::findAllTypefaceNames();
    std::vector<std::string> names;
    for(auto const& name : typefaceNames)
    {
        names.push_back(name.toStdString());
    }
    return names;
}

static std::vector<std::string> getFontSizes()
{
    std::vector<std::string> names;
    for(auto size = 8; size <= 20; size += 2)
    {
        names.push_back(std::to_string(size));
    }
    return names;
}

Track::PropertyGraphicalSection::PropertyGraphicalSection(Director& director, PresetList::Accessor& presetListAcsr)
: mDirector(director)
, mPresetListAccessor(presetListAcsr)
, mZoomGridPropertyWindow(juce::translate("Grid Properties"), mZoomGridPropertyPanel)
, mPropertyColourMap(juce::translate("Color Map"), juce::translate("The color map of the graphical renderer."), "", getColourMapNames(), [&](size_t index)
                     {
                         mDirector.startAction();
                         setColourMap(static_cast<ColourMap>(index));
                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change track color map"));
                     })
, mPropertyForegroundColour(juce::translate("Foreground Color"), juce::translate("The foreground color of the graphical renderer."), juce::translate("Select the foreground color"), [&](juce::Colour const& colour)
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
, mPropertyDurationColour(juce::translate("Duration Color"), juce::translate("The duration color of the graphical renderer."), juce::translate("Select the duration color"), [&](juce::Colour const& colour)
                          {
                              if(!mPropertyDurationColour.entry.isColourSelectorVisible())
                              {
                                  mDirector.startAction();
                              }
                              setDurationColour(colour);
                              if(!mPropertyDurationColour.entry.isColourSelectorVisible())
                              {
                                  mDirector.endAction(ActionState::newTransaction, juce::translate("Change track duration color"));
                              }
                          },
                          [&]()
                          {
                              mDirector.startAction();
                          },
                          [&]()
                          {
                              mDirector.endAction(ActionState::newTransaction, juce::translate("Change track duration color"));
                          })
, mPropertyBackgroundColour(juce::translate("Background Color"), juce::translate("The background color of the graphical renderer."), juce::translate("Select the background color"), [&](juce::Colour const& colour)
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
, mPropertyTextColour(juce::translate("Text Color"), juce::translate("The text color of the graphical renderer."), juce::translate("Select the text color"), [&](juce::Colour const& colour)
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
, mPropertyShadowColour(juce::translate("Shadow Color"), juce::translate("The shadow color of the graphical renderer."), juce::translate("Select the shadow color"), [&](juce::Colour const& colour)
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
, mPropertyFontName(juce::translate("Font Name"), juce::translate("The name of the font for the graphical renderer."), "", getFontNames(), [&]([[maybe_unused]] size_t index)
                    {
                        mDirector.startAction();
                        auto const name = mPropertyFontName.entry.getText();
                        auto const font = mAccessor.getAttr<AttrType::font>();
                        auto newFont = juce::FontOptions(name, font.getHeight(), juce::Font::FontStyleFlags::plain);
                        if(juce::Font(newFont).getAvailableStyles().contains(font.getStyle()))
                        {
                            newFont = newFont.withStyle(font.getStyle());
                        }
                        mAccessor.setAttr<AttrType::font>(newFont, NotificationType::synchronous);
                        mDirector.endAction(ActionState::newTransaction, juce::translate("Change track font name"));
                    })
, mPropertyFontStyle(juce::translate("Font Style"), juce::translate("The style of the font for the graphical renderer."), "", {}, [&]([[maybe_unused]] size_t index)
                     {
                         mDirector.startAction();
                         auto const style = mPropertyFontStyle.entry.getText();
                         auto const font = mAccessor.getAttr<AttrType::font>().withStyle(style);
                         mAccessor.setAttr<AttrType::font>(font, NotificationType::synchronous);
                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change track font style"));
                     })
, mPropertyFontSize(juce::translate("Font Size"), juce::translate("The size of the font for the graphical renderer."), "", getFontSizes(), [&]([[maybe_unused]] size_t index)
                    {
                        mDirector.startAction();
                        auto const size = mPropertyFontSize.entry.getText().getFloatValue();
                        auto const font = mAccessor.getAttr<AttrType::font>().withHeight(size);
                        mAccessor.setAttr<AttrType::font>(font, NotificationType::synchronous);
                        mDirector.endAction(ActionState::newTransaction, juce::translate("Change track font size"));
                    })
, mPropertyLineWidth(juce::translate("Line Width"), juce::translate("The line width for the graphical renderer."), "", {1.0f, 100.0f}, 0.5f, [&](float value)
                     {
                         mDirector.startAction();
                         mAccessor.setAttr<AttrType::lineWidth>(value, NotificationType::synchronous);
                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change the line width for the graphical renderer"));
                     })
, mPropertyUnit("Unit", "The unit of the values", [&](juce::String text)
                {
                    setUnit(text);
                })
, mPropertyLabelJustification(juce::translate("Label Justification"), juce::translate("The justification of the labels."), "", std::vector<std::string>{"Top", "Centred", "Bottom"}, [&](size_t index)
                              {
                                  mDirector.startAction();
                                  setLabelJustification(magic_enum::enum_cast<LabelLayout::Justification>(static_cast<int>(index)).value_or(mAccessor.getAttr<AttrType::labelLayout>().justification));
                                  mDirector.endAction(ActionState::newTransaction, juce::translate("Change the justification of the labels"));
                              })
, mPropertyLabelPosition(juce::translate("Label Position"), juce::translate("The position of the labels."), "", {-120.0f, 120.0f}, 0.1f, [this](float position)
                         {
                             mDirector.startAction();
                             setLabelPosition(position);
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change the position of the labels"));
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
, mPropertyValueRangeMin(juce::translate("Minimum Range Value"), juce::translate("The minimum value of the output."), "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float value)
                         {
                             mDirector.startAction();
                             setValueRangeMin(static_cast<double>(value));
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track value range"));
                         })
, mPropertyValueRangeMax(juce::translate("Maximum Range Value"), juce::translate("The maximum value of the output."), "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float value)
                         {
                             mDirector.startAction();
                             setValueRangeMax(static_cast<double>(value));
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track value range"));
                         })
, mPropertyValueRange(juce::translate("Value Range"), juce::translate("The range of the output."), "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&]()
                      {
                          mDirector.startAction();
                      },
                      [&](float min, float max)
                      {
                          setValueRange(Zoom::Range(min, max));
                      },
                      [&]()
                      {
                          mDirector.endAction(ActionState::newTransaction, juce::translate("Change track value range"));
                      })
, mPropertyValueRangeLogScale(juce::translate("Value Log. Scale"), juce::translate("Toggle the logarithmic scale of the zoom range."), [&](bool value)
                              {
                                  mDirector.startAction();
                                  mAccessor.setAttr<AttrType::zoomLogScale>(value, NotificationType::synchronous);
                                  mDirector.endAction(ActionState::newTransaction, juce::translate("Change track log scale"));
                              })
, mPropertyGrid(juce::translate("Grid"), juce::translate("Edit the grid properties"), [&]()
                {
                    updateGridPanel();
                    mZoomGridPropertyWindow.show();
                })
, mPropertyRangeLink(juce::translate("Value Range Link"), juce::translate("Toggle the link with the group for zoom range."), [&](bool value)
                     {
                         mDirector.startAction();
                         mAccessor.setAttr<AttrType::zoomLink>(value, NotificationType::synchronous);
                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change track zoom link"));
                     })
, mPropertyNumBins(juce::translate("Num Bins"), juce::translate("The number of bins."), "", {0.0f, static_cast<float>(Zoom::max())}, 1.0f, nullptr)
, mPropertyChannelLayout(juce::translate("Channel Layout"), juce::translate("The visible state of the channels."), [&]()
                         {
                             showChannelLayout();
                         })
, mPropertyShowInGroup(juce::translate("Show in the group overlay view"), juce::translate("Toggle the visibility of the track in the group overlay view."), [&](bool state)
                       {
                           mDirector.startAction();
                           mAccessor.setAttr<AttrType::showInGroup>(state, NotificationType::synchronous);
                           mDirector.endAction(ActionState::newTransaction, juce::translate("Change the visibility of the track in the group overlay view"));
                       })
, mPropertyGraphicsPreset(juce::translate("Graphic Preset"), juce::translate("The graphic preset of the track"), "", std::vector<std::string>{}, [&]([[maybe_unused]] size_t index)
                          {
                              auto const selectedId = mPropertyGraphicsPreset.entry.getSelectedId();
                              switch(selectedId)
                              {
                                  case MenuGraphicsPresetId::loadPresetId:
                                  {
                                      loadGraphicsPreset();
                                      break;
                                  }
                                  case MenuGraphicsPresetId::savePresetId:
                                  {
                                      saveGraphicsPreset();
                                      break;
                                  }
                                  case MenuGraphicsPresetId::saveDefaultPresetId:
                                  {
                                      saveAsDefaultGraphicsPreset();
                                      break;
                                  }
                                  case MenuGraphicsPresetId::deleteDefaultPresetId:
                                  {
                                      deleteDefaultGraphicsPreset();
                                      break;
                                  }
                                  default:
                                      break;
                              };
                          })
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::description:
            case AttrType::results:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::zoomValueMode:
            case AttrType::hasPluginColourMap:
            {
                for(auto* child : getChildren())
                {
                    MiscWeakAssert(child != nullptr);
                    if(child != nullptr)
                    {
                        child->setVisible(false);
                    }
                }

                addExtraThresholdProperties();
                auto const& channelsLayout = acsr.getAttr<AttrType::channelsLayout>();
                auto const numChannels = channelsLayout.size();
                auto const frameType = Tools::getFrameType(acsr);
                if(frameType.has_value())
                {
                    switch(frameType.value())
                    {
                        case Track::FrameType::label:
                        {
                            mPropertyForegroundColour.setVisible(true);
                            mPropertyDurationColour.setVisible(true);
                            mPropertyTextColour.setVisible(true);
                            mPropertyBackgroundColour.setVisible(true);
                            mPropertyShadowColour.setVisible(true);
                            mPropertyFontName.setVisible(true);
                            mPropertyFontStyle.setVisible(true);
                            mPropertyFontSize.setVisible(true);
                            mPropertyLineWidth.setVisible(true);
                            mPropertyLabelJustification.setVisible(true);
                            mPropertyLabelPosition.setVisible(true);
                            mPropertyChannelLayout.setVisible(numChannels > 1_z);
                            mPropertyShowInGroup.setVisible(true);
                            break;
                        }
                        case Track::FrameType::value:
                        {
                            auto const unit = Tools::getUnit(acsr);
                            mPropertyValueRangeMin.entry.setTextValueSuffix(unit);
                            mPropertyValueRangeMax.entry.setTextValueSuffix(unit);
                            mPropertyUnit.entry.setText(unit.isEmpty() ? " " : unit, juce::NotificationType::dontSendNotification);
                            mPropertyRangeLink.title.setText(juce::translate("Value Range Link"), juce::NotificationType::dontSendNotification);
                            mPropertyForegroundColour.setVisible(true);
                            mPropertyTextColour.setVisible(true);
                            mPropertyBackgroundColour.setVisible(true);
                            mPropertyShadowColour.setVisible(true);
                            mPropertyFontName.setVisible(true);
                            mPropertyFontStyle.setVisible(true);
                            mPropertyFontSize.setVisible(true);
                            mPropertyLineWidth.setVisible(true);
                            mPropertyUnit.setVisible(true);
                            mPropertyValueRangeMode.setVisible(true);
                            mPropertyValueRangeLogScale.setVisible(Tools::hasVerticalZoomInHertz(acsr));
                            mPropertyValueRangeMin.setVisible(true);
                            mPropertyValueRangeMax.setVisible(true);
                            mPropertyRangeLink.setVisible(true);
                            mPropertyGrid.setVisible(true);
                            mPropertyChannelLayout.setVisible(numChannels > 1_z);
                            mPropertyShowInGroup.setVisible(true);
                            break;
                        }
                        case Track::FrameType::vector:
                        {
                            auto const unit = Tools::getUnit(acsr);
                            mPropertyValueRangeMin.entry.setTextValueSuffix(unit);
                            mPropertyValueRangeMax.entry.setTextValueSuffix(unit);
                            mPropertyValueRange.numberFieldLow.setTextValueSuffix(unit);
                            mPropertyValueRange.numberFieldHigh.setTextValueSuffix(unit);
                            mPropertyUnit.entry.setText(unit.isEmpty() ? " " : unit, juce::NotificationType::dontSendNotification);
                            mPropertyRangeLink.title.setText(juce::translate("Bin Range Link"), juce::NotificationType::dontSendNotification);
                            mPropertyColourMap.setVisible(!acsr.getAttr<AttrType::hasPluginColourMap>());
                            mPropertyUnit.setVisible(true);
                            mPropertyValueRangeMode.setVisible(!acsr.getAttr<AttrType::hasPluginColourMap>());
                            mPropertyValueRangeLogScale.setVisible(Tools::hasVerticalZoomInHertz(acsr));
                            mPropertyValueRangeMin.setVisible(true);
                            mPropertyValueRangeMax.setVisible(true);
                            mPropertyValueRange.setVisible(true);
                            mPropertyNumBins.setVisible(true);
                            mPropertyRangeLink.setVisible(true);
                            mPropertyGrid.setVisible(true);
                            mPropertyChannelLayout.setVisible(numChannels > 1_z);
                            mProgressBarRendering.setVisible(true);
                            mPropertyShowInGroup.setVisible(true);
                            break;
                        }
                    }
                }
                updateZoomMode();
                updateExtraTheshold();
                resized();
            }
            break;
            case AttrType::colours:
            {
                auto const colours = acsr.getAttr<AttrType::colours>();
                mPropertyBackgroundColour.entry.setCurrentColour(colours.background, juce::NotificationType::dontSendNotification);
                mPropertyForegroundColour.entry.setCurrentColour(colours.foreground, juce::NotificationType::dontSendNotification);
                mPropertyDurationColour.entry.setCurrentColour(colours.duration, juce::NotificationType::dontSendNotification);
                mPropertyTextColour.entry.setCurrentColour(colours.text, juce::NotificationType::dontSendNotification);
                mPropertyShadowColour.entry.setCurrentColour(colours.shadow, juce::NotificationType::dontSendNotification);
                mPropertyColourMap.entry.setSelectedItemIndex(static_cast<int>(colours.map), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::font:
            {
                auto const font = acsr.getAttr<AttrType::font>();
                mPropertyFontName.entry.setText(font.getName(), juce::NotificationType::dontSendNotification);
                mPropertyFontName.entry.setEnabled(mPropertyFontName.entry.getNumItems() > 1);

                mPropertyFontStyle.entry.clear(juce::NotificationType::dontSendNotification);
                mPropertyFontStyle.entry.addItemList(juce::Font(font).getAvailableStyles(), 1);
                mPropertyFontStyle.entry.setText(font.getStyle(), juce::NotificationType::dontSendNotification);
                mPropertyFontStyle.entry.setEnabled(mPropertyFontStyle.entry.getNumItems() > 1);

                auto const roundedHeight = std::round(font.getHeight());
                if(std::abs(font.getHeight() - roundedHeight) < 0.1f)
                {
                    mPropertyFontSize.entry.setText(juce::String(static_cast<int>(roundedHeight)), juce::NotificationType::dontSendNotification);
                }
                else
                {
                    mPropertyFontSize.entry.setText(juce::String(font.getHeight(), 1), juce::NotificationType::dontSendNotification);
                }
            }
            break;
            case AttrType::lineWidth:
            {
                auto const& lineWidth = acsr.getAttr<AttrType::lineWidth>();
                mPropertyLineWidth.entry.setValue(static_cast<double>(lineWidth), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::labelLayout:
            {
                auto const& labelLayout = acsr.getAttr<AttrType::labelLayout>();
                auto const index = magic_enum::enum_index(labelLayout.justification).value_or(mPropertyLabelJustification.entry.getSelectedItemIndex());
                mPropertyLabelJustification.entry.setSelectedItemIndex(static_cast<int>(index), juce::NotificationType::dontSendNotification);
                mPropertyLabelPosition.entry.setValue(static_cast<double>(labelLayout.position), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::zoomLink:
            {
                mPropertyRangeLink.entry.setToggleState(acsr.getAttr<AttrType::zoomLink>(), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::zoomLogScale:
            {
                mPropertyValueRangeLogScale.entry.setToggleState(acsr.getAttr<AttrType::zoomLogScale>(), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::showInGroup:
            {
                mPropertyShowInGroup.entry.setToggleState(acsr.getAttr<AttrType::showInGroup>(), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::extraThresholds:
            {
                updateExtraTheshold();
            }
            break;
            case AttrType::edit:
            case AttrType::name:
            case AttrType::key:
            case AttrType::input:
            case AttrType::file:
            case AttrType::state:
            case AttrType::graphics:
            case AttrType::processing:
            case AttrType::warnings:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::zoomAcsr:
            case AttrType::focused:
            case AttrType::grid:
            case AttrType::oscIdentifier:
            case AttrType::sendViaOsc:
            case AttrType::sampleRate:
            case AttrType::graphicsSettings:
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
                    mPropertyValueRange.numberFieldLow.setRange(range, interval, juce::NotificationType::dontSendNotification);
                    mPropertyValueRange.numberFieldHigh.setRange(range, interval, juce::NotificationType::dontSendNotification);
                }

                updateZoomMode();
                mPropertyValueRange.repaint();
            }
            break;
            case Zoom::AttrType::visibleRange:
            {
                auto const range = acsr.getAttr<Zoom::AttrType::visibleRange>();
                mPropertyValueRange.entry.setMinAndMaxValues(range.getStart(), range.getEnd(), juce::NotificationType::dontSendNotification);
                mPropertyValueRange.numberFieldLow.setValue(range.getStart(), juce::NotificationType::dontSendNotification);
                mPropertyValueRange.numberFieldHigh.setValue(range.getEnd(), juce::NotificationType::dontSendNotification);
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

    mZoomGridPropertyPanel.onChangeBegin = [this]([[maybe_unused]] Zoom::Grid::Accessor const& acsr)
    {
        mDirector.startAction();
    };

    mZoomGridPropertyPanel.onChangeEnd = [this](Zoom::Grid::Accessor const& acsr)
    {
        auto& zoomAcsr = getCurrentZoomAcsr();
        auto& gridAcsr = zoomAcsr.getAcsr<Zoom::AcsrType::grid>();
        auto const isLog = mAccessor.getAttr<AttrType::zoomLogScale>() && Tools::hasVerticalZoomInHertz(mAccessor);
        if(isLog)
        {
            Zoom::Grid::Accessor temp;
            temp.copyFrom(acsr, NotificationType::synchronous);
            auto const reference = temp.getAttr<Zoom::Grid::AttrType::tickReference>();
            auto const nyquist = mAccessor.getAttr<AttrType::sampleRate>() / 2.0;
            auto const scaleRatio = static_cast<float>(nyquist / std::max(Tools::getMidiFromHertz(nyquist), 1.0));
            temp.setAttr<Zoom::Grid::AttrType::tickReference>(Tools::getMidiFromHertz(reference) * scaleRatio, NotificationType::synchronous);
            gridAcsr.copyFrom(temp, NotificationType::synchronous);
        }
        else
        {
            gridAcsr.copyFrom(acsr, NotificationType::synchronous);
        }
        updateGridPanel();
        mDirector.endAction(ActionState::newTransaction, juce::translate("Edit Grid Properties"));
    };

    mPropertyNumBins.entry.setEnabled(false);
    mProgressBarRendering.setSize(300, 36);
    mPropertyFontSize.entry.setEditableText(true);
    mPropertyFontSize.entry.getProperties().set("isNumber", true);
    NumberField::Label::storeProperties(mPropertyFontSize.entry.getProperties(), {4.0, 200.0}, 0.1, 1, "");
    mPropertyLineWidth.entry.getProperties().set("isNumber", true);
    NumberField::Label::storeProperties(mPropertyLineWidth.entry.getProperties(), {1.0, 100.0}, 0.5, 1, "");
    addAndMakeVisible(mPropertyColourMap);
    addAndMakeVisible(mPropertyForegroundColour);
    addAndMakeVisible(mPropertyDurationColour);
    addAndMakeVisible(mPropertyTextColour);
    addAndMakeVisible(mPropertyBackgroundColour);
    addAndMakeVisible(mPropertyShadowColour);
    addAndMakeVisible(mPropertyFontName);
    addAndMakeVisible(mPropertyFontStyle);
    addAndMakeVisible(mPropertyFontSize);
    addAndMakeVisible(mPropertyLineWidth);
    addAndMakeVisible(mPropertyUnit);
    addAndMakeVisible(mPropertyLabelJustification);
    addAndMakeVisible(mPropertyLabelPosition);
    addAndMakeVisible(mPropertyValueRangeMode);
    addAndMakeVisible(mPropertyValueRangeLogScale);
    addAndMakeVisible(mPropertyValueRangeMin);
    addAndMakeVisible(mPropertyValueRangeMax);
    addAndMakeVisible(mPropertyValueRange);
    addAndMakeVisible(mPropertyNumBins);
    addAndMakeVisible(mPropertyRangeLink);
    addAndMakeVisible(mPropertyGrid);
    addAndMakeVisible(mPropertyChannelLayout);
    addAndMakeVisible(mPropertyShowInGroup);
    addAndMakeVisible(mPropertyGraphicsPreset);
    addAndMakeVisible(mProgressBarRendering);
    setSize(300, 400);

    // Setup graphics preset menu
    mPropertyGraphicsPreset.entry.clear(juce::NotificationType::dontSendNotification);
    mPropertyGraphicsPreset.entry.addItem("Load...", MenuGraphicsPresetId::loadPresetId);
    mPropertyGraphicsPreset.entry.addItem("Save...", MenuGraphicsPresetId::savePresetId);
    mPropertyGraphicsPreset.entry.addSeparator();
    mPropertyGraphicsPreset.entry.addItem("Save as Default", MenuGraphicsPresetId::saveDefaultPresetId);
    mPropertyGraphicsPreset.entry.addItem("Delete Default", MenuGraphicsPresetId::deleteDefaultPresetId);
    updateGraphicsPresetState();

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
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyColourMap);
    setBounds(mPropertyForegroundColour);
    setBounds(mPropertyDurationColour);
    setBounds(mPropertyTextColour);
    setBounds(mPropertyBackgroundColour);
    setBounds(mPropertyShadowColour);
    setBounds(mPropertyFontName);
    setBounds(mPropertyFontStyle);
    setBounds(mPropertyFontSize);
    setBounds(mPropertyLineWidth);
    setBounds(mPropertyUnit);
    setBounds(mPropertyLabelJustification);
    setBounds(mPropertyLabelPosition);
    setBounds(mPropertyValueRangeMode);
    setBounds(mPropertyValueRangeLogScale);
    setBounds(mPropertyValueRangeMin);
    setBounds(mPropertyValueRangeMax);
    setBounds(mPropertyValueRange);
    for(auto& extraOutput : mPropertyExtraThresholds)
    {
        setBounds(*extraOutput.get());
    }
    setBounds(mPropertyNumBins);
    setBounds(mPropertyRangeLink);
    setBounds(mPropertyShowInGroup);
    setBounds(mPropertyGrid);
    setBounds(mPropertyChannelLayout);
    setBounds(mPropertyGraphicsPreset);
    setBounds(mProgressBarRendering);
    setSize(getWidth(), bounds.getY());
}

Zoom::Accessor& Track::PropertyGraphicalSection::getCurrentZoomAcsr()
{
    return Tools::getFrameType(mAccessor) == FrameType::vector ? mAccessor.getAcsr<AcsrType::binZoom>() : mAccessor.getAcsr<AcsrType::valueZoom>();
}

void Track::PropertyGraphicalSection::addExtraThresholdProperties()
{
    if(Tools::getFrameType(mAccessor) == FrameType::vector)
    {
        return;
    }
    mPropertyExtraThresholds.clear();
    auto const& description = mAccessor.getAttr<AttrType::description>();
    auto const size = description.extraOutputs.size();
    for(auto index = 0_z; index < size; ++index)
    {
        auto const& output = description.extraOutputs.at(index);
        auto const range = Tools::getExtraRange(mAccessor, index);
        auto const tooltip = Tools::getExtraTooltip(mAccessor, index);
        auto const name = juce::translate("NAME Threshold").replace("NAME", output.name);
        auto const start = range.has_value() ? static_cast<float>(range.value().getStart()) : 0.0f;
        auto const end = range.has_value() ? static_cast<float>(range.value().getEnd()) : 0.0f;
        auto const step = output.isQuantized ? output.quantizeStep : 0.0f;

        juce::WeakReference<juce::Component> weakReference(this);
        auto const startChange = [=, this]()
        {
            if(weakReference.get() == nullptr)
            {
                return;
            }
            mDirector.startAction();
        };
        auto const endChange = [=, this]()
        {
            if(weakReference.get() == nullptr)
            {
                return;
            }
            mDirector.endAction(ActionState::newTransaction, juce::translate("Modify the NAME threshold").replace("NAME", output.name));
        };
        auto const applyChange = [=, this](float newValue)
        {
            if(weakReference.get() == nullptr)
            {
                return;
            }
            auto thresholds = mAccessor.getAttr<AttrType::extraThresholds>();
            thresholds.resize(size);
            if(newValue <= start)
            {
                thresholds[index].reset();
            }
            else
            {
                thresholds[index] = newValue;
            }
            mAccessor.setAttr<AttrType::extraThresholds>(thresholds, NotificationType::synchronous);
        };
        auto property = std::make_unique<PropertySlider>(name, tooltip, output.unit, juce::Range<float>{start, std::max(end, start + std::numeric_limits<float>::epsilon() * 100.0f)}, step, startChange, applyChange, endChange, true);
        anlWeakAssert(property != nullptr);
        if(property != nullptr)
        {
            addAndMakeVisible(property.get());
            mPropertyExtraThresholds.push_back(std::move(property));
        }
    }
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

void Track::PropertyGraphicalSection::setDurationColour(juce::Colour const& colour)
{
    auto colours = mAccessor.getAttr<AttrType::colours>();
    colours.duration = colour;
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

void Track::PropertyGraphicalSection::setUnit(juce::String const& unit)
{
    if(unit.isEmpty() && !mAccessor.getAttr<AttrType::description>().output.unit.empty())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::QuestionIcon)
                                 .withTitle("Would you like to reset the unit to default?")
                                 .withMessage("The specified unit is empty. Would you like remove the unit or to reset the unit to default using the one provided by the plugin?")
                                 .withButton(juce::translate("Remove the unit"))
                                 .withButton(juce::translate("Reset the unit to default"));
        juce::WeakReference<juce::Component> weakReference(this);
        juce::AlertWindow::showAsync(options, [=, this](int result)
                                     {
                                         mDirector.startAction();
                                         if(result == 1)
                                         {
                                             mAccessor.setAttr<AttrType::unit>(juce::String(), NotificationType::synchronous);
                                         }
                                         else
                                         {
                                             mAccessor.setAttr<AttrType::unit>(std::optional<juce::String>(), NotificationType::synchronous);
                                         }
                                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change unit of the values name"));
                                     });
    }
    else
    {
        mDirector.startAction();
        mAccessor.setAttr<AttrType::unit>(unit, NotificationType::synchronous);
        mDirector.endAction(ActionState::newTransaction, juce::translate("Change unit of the values name"));
    }
}

void Track::PropertyGraphicalSection::setLabelJustification(LabelLayout::Justification justification)
{
    auto labelLayout = mAccessor.getAttr<AttrType::labelLayout>();
    labelLayout.justification = justification;
    mAccessor.setAttr<AttrType::labelLayout>(labelLayout, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setLabelPosition(float position)
{
    auto labelLayout = mAccessor.getAttr<AttrType::labelLayout>();
    labelLayout.position = position;
    mAccessor.setAttr<AttrType::labelLayout>(labelLayout, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setPluginValueRange()
{
    auto globalRange = Tools::getValueRange(mAccessor.getAttr<AttrType::description>());
    if(!globalRange.has_value() || globalRange->isEmpty())
    {
        return;
    }
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    auto const visibleRange = Zoom::Tools::getScaledVisibleRange(zoomAcsr, globalRange.value());
    mDirector.setGlobalValueRange(globalRange.value(), NotificationType::synchronous);
    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setResultValueRange()
{
    auto globalRange = Tools::getResultRange(mAccessor);
    if(!globalRange.has_value() || globalRange->isEmpty())
    {
        return;
    }
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    auto const visibleRange = Zoom::Tools::getScaledVisibleRange(zoomAcsr, globalRange.value());
    mDirector.setGlobalValueRange(globalRange.value(), NotificationType::synchronous);
    zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setValueRangeMin(double value)
{
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    auto const end = std::max(zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getEnd(), value);
    mDirector.setGlobalValueRange({value, end}, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setValueRangeMax(double value)
{
    auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
    auto const start = std::min(zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getStart(), value);
    mDirector.setGlobalValueRange({start, value}, NotificationType::synchronous);
}

void Track::PropertyGraphicalSection::setValueRange(juce::Range<double> const& range)
{
    mAccessor.getAcsr<AcsrType::valueZoom>().setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
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
    mPropertyValueRangeMode.entry.setItemEnabled(1, Tools::getValueRange(mAccessor.getAttr<AttrType::description>()).has_value());
    mPropertyValueRangeMode.entry.setItemEnabled(2, Tools::getResultRange(mAccessor).has_value());
    mPropertyValueRangeMode.entry.setItemEnabled(3, false);
    switch(mAccessor.getAttr<AttrType::zoomValueMode>())
    {
        case ZoomValueMode::plugin:
            mPropertyValueRangeMode.entry.setSelectedId(1, juce::NotificationType::dontSendNotification);
            break;
        case ZoomValueMode::results:
            mPropertyValueRangeMode.entry.setSelectedId(2, juce::NotificationType::dontSendNotification);
            break;
        case ZoomValueMode::undefined:
        case ZoomValueMode::custom:
            mPropertyValueRangeMode.entry.setSelectedId(3, juce::NotificationType::dontSendNotification);
            break;
    }
}

void Track::PropertyGraphicalSection::updateExtraTheshold()
{
    auto const& thresholds = mAccessor.getAttr<AttrType::extraThresholds>();
    for(auto index = 0_z; index < mPropertyExtraThresholds.size(); ++index)
    {
        auto const value = index < thresholds.size() ? thresholds.at(index) : std::optional<float>();
        auto& property = mPropertyExtraThresholds[index];
        MiscWeakAssert(property != nullptr);
        if(property != nullptr) [[likely]]
        {
            auto const effective = value.has_value() ? value.value() : property->entry.getRange().getStart();
            property->entry.setValue(static_cast<double>(effective), juce::NotificationType::dontSendNotification);
            property->numberField.setValue(static_cast<double>(effective), juce::NotificationType::dontSendNotification);
        }
    }
}

void Track::PropertyGraphicalSection::updateGridPanel()
{
    auto const isLog = mAccessor.getAttr<AttrType::zoomLogScale>() && Tools::hasVerticalZoomInHertz(mAccessor);
    if(isLog)
    {
        Zoom::Grid::Accessor temp;
        temp.copyFrom(getCurrentZoomAcsr().getAcsr<Zoom::AcsrType::grid>(), NotificationType::synchronous);
        auto const reference = temp.getAttr<Zoom::Grid::AttrType::tickReference>();
        auto const nyquist = mAccessor.getAttr<AttrType::sampleRate>() / 2.0;
        auto const scaleRatio = static_cast<float>(std::max(Tools::getMidiFromHertz(nyquist), 1.0) / nyquist);
        temp.setAttr<Zoom::Grid::AttrType::tickReference>(Tools::getHertzFromMidi(reference * scaleRatio), NotificationType::synchronous);
        mZoomGridPropertyPanel.setGrid(temp);
    }
    else
    {
        mZoomGridPropertyPanel.setGrid(getCurrentZoomAcsr().getAcsr<Zoom::AcsrType::grid>());
    }
}

void Track::PropertyGraphicalSection::loadGraphicsPreset()
{
    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Load graphics preset from file..."), juce::File{}, "*.ptlgraphics");
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    juce::WeakReference<juce::Component> weakReference(this);
    mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  auto const results = fileChooser.getResults();
                                  if(weakReference.get() == nullptr)
                                  {
                                      return;
                                  }
                                  if(results.isEmpty())
                                  {
                                      updateGraphicsPresetState();
                                      return;
                                  }
                                  mDirector.startAction();
                                  auto const exportResult = Exporter::fromGraphicsPreset(mAccessor, results.getFirst());
                                  if(exportResult.failed())
                                  {
                                      mDirector.endAction(ActionState::abort);
                                      auto const options = juce::MessageBoxOptions()
                                                               .withIconType(juce::AlertWindow::WarningIcon)
                                                               .withTitle(juce::translate("Failed to load from graphics preset file!"))
                                                               .withMessage(exportResult.getErrorMessage())
                                                               .withButton(juce::translate("Ok"));
                                      juce::AlertWindow::showAsync(options, nullptr);
                                  }
                                  else
                                  {
                                      mDirector.endAction(ActionState::newTransaction, juce::translate("Change track's graphic properties from preset file"));
                                  }
                                  updateGraphicsPresetState();
                              });
}

void Track::PropertyGraphicalSection::saveGraphicsPreset()
{
    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Save as graphics preset..."), juce::File{}, "*.ptlgraphics");
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    juce::WeakReference<juce::Component> weakReference(this);
    mFileChooser->launchAsync(Flags::saveMode | Flags::canSelectFiles | Flags::warnAboutOverwriting, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  auto const results = fileChooser.getResults();
                                  if(weakReference.get() == nullptr)
                                  {
                                      return;
                                  }
                                  if(results.isEmpty())
                                  {
                                      updateGraphicsPresetState();
                                      return;
                                  }
                                  // Update graphicsSettings from individual attributes before saving
                                  GraphicsSettings settings;
                                  settings.colours = mAccessor.getAttr<AttrType::colours>();
                                  settings.font = mAccessor.getAttr<AttrType::font>();
                                  settings.lineWidth = mAccessor.getAttr<AttrType::lineWidth>();
                                  settings.unit = mAccessor.getAttr<AttrType::unit>();
                                  settings.labelLayout = mAccessor.getAttr<AttrType::labelLayout>();
                                  mAccessor.setAttr<AttrType::graphicsSettings>(settings, NotificationType::synchronous);

                                  auto const result = Exporter::toGraphicsPreset(mAccessor, results.getFirst());
                                  if(result.failed())
                                  {
                                      auto const options = juce::MessageBoxOptions()
                                                               .withIconType(juce::AlertWindow::WarningIcon)
                                                               .withTitle(juce::translate("Failed to save a graphics preset file!"))
                                                               .withMessage(result.getErrorMessage())
                                                               .withButton(juce::translate("Ok"));
                                      juce::AlertWindow::showAsync(options, nullptr);
                                  }
                                  updateGraphicsPresetState();
                              });
}

void Track::PropertyGraphicalSection::saveAsDefaultGraphicsPreset()
{
    // Update graphicsSettings from individual attributes before saving
    GraphicsSettings settings;
    settings.colours = mAccessor.getAttr<AttrType::colours>();
    settings.font = mAccessor.getAttr<AttrType::font>();
    settings.lineWidth = mAccessor.getAttr<AttrType::lineWidth>();
    settings.unit = mAccessor.getAttr<AttrType::unit>();
    settings.labelLayout = mAccessor.getAttr<AttrType::labelLayout>();
    mAccessor.setAttr<AttrType::graphicsSettings>(settings, NotificationType::synchronous);

    auto preset = mPresetListAccessor.getAttr<PresetList::AttrType::graphic>();
    preset[mAccessor.getAttr<AttrType::key>()] = settings;
    mPresetListAccessor.setAttr<PresetList::AttrType::graphic>(preset, NotificationType::synchronous);
    updateGraphicsPresetState();
}

void Track::PropertyGraphicalSection::deleteDefaultGraphicsPreset()
{
    auto preset = mPresetListAccessor.getAttr<PresetList::AttrType::graphic>();
    preset.erase(mAccessor.getAttr<AttrType::key>());
    mPresetListAccessor.setAttr<PresetList::AttrType::graphic>(preset, NotificationType::synchronous);
    updateGraphicsPresetState();
}

void Track::PropertyGraphicalSection::updateGraphicsPresetState()
{
    auto const& key = mAccessor.getAttr<AttrType::key>();
    auto const& preset = mPresetListAccessor.getAttr<PresetList::AttrType::graphic>();
    mPropertyGraphicsPreset.entry.setItemEnabled(MenuGraphicsPresetId::deleteDefaultPresetId, preset.count(key) > 0_z);
}

ANALYSE_FILE_END
