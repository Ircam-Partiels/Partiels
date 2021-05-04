#include "AnlTrackPropertyPanel.h"
#include "../Zoom/AnlZoomTools.h"
#include "AnlTrackExporter.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::PropertyPanel::PropertyPanel(Director& director)
: FloatingWindowContainer("Properties", mViewport)
, mDirector(director)

, mPropertyName("Name", "The name of the track", [&](juce::String text)
                {
                    mDirector.startAction();
                    mAccessor.setAttr<AttrType::name>(text, NotificationType::synchronous);
                    mDirector.endAction(juce::translate("Change track name"), ActionState::apply);
                })

, mPropertyWindowType("Window Type", "The window type of the FFT.", "", std::vector<std::string>{"Rectangular", "Triangular", "Hamming", "Hanning", "Blackman", "Nuttall", "BlackmanHarris"}, [&](size_t index)
                      {
                          mDirector.startAction();
                          auto state = mAccessor.getAttr<AttrType::state>();
                          state.windowType = static_cast<Plugin::WindowType>(index);
                          mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                          mDirector.endAction(juce::translate("Change track window type"), ActionState::apply);
                      })
, mPropertyWindowSize("Window Size", "The window size of the FFT.", "samples", std::vector<std::string>{"8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096"}, [&](size_t index)
                      {
                          mDirector.startAction();
                          auto state = mAccessor.getAttr<AttrType::state>();
                          auto const overlapping = state.blockSize / state.stepSize;
                          state.blockSize = static_cast<size_t>(std::pow(2.0, static_cast<int>(index) + 3));
                          state.stepSize = state.blockSize / overlapping;
                          mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                          mDirector.endAction(juce::translate("Change track window size"), ActionState::apply);
                      })
, mPropertyWindowOverlapping("Window Overlapping", "The window overlapping of the FFT.", "x", std::vector<std::string>{}, [&](size_t index)
                             {
                                 mDirector.startAction();
                                 auto state = mAccessor.getAttr<AttrType::state>();
                                 state.stepSize = state.blockSize / std::max(static_cast<size_t>(std::pow(2.0, static_cast<int>(index))), 1_z);
                                 mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                                 mDirector.endAction(juce::translate("Change track window overlapping"), ActionState::apply);
                             })
, mPropertyBlockSize("Block Size", "The block size used by the track. [1:65536]", "samples", {1.0f, 65536.0f}, 1.0f, [&](float value)
                     {
                         mDirector.startAction();
                         auto state = mAccessor.getAttr<AttrType::state>();
                         state.blockSize = static_cast<size_t>(value);
                         mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                         mDirector.endAction(juce::translate("Change track block size"), ActionState::apply);
                     })
, mPropertyStepSize("Step Size", "The step size used by the track. [1:65536]", "samples", {1.0f, 65536.0f}, 1.0f, [&](float value)
                    {
                        mDirector.startAction();
                        auto state = mAccessor.getAttr<AttrType::state>();
                        state.stepSize = static_cast<size_t>(value);
                        mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                        mDirector.endAction(juce::translate("Change track step size"), ActionState::apply);
                    })
, mPropertyPreset("Preset", "The preset of the track", "", std::vector<std::string>{"Factory", "Custom", "Load...", "Save..."}, [&](size_t index)
                  {
                      switch(index)
                      {
                          case 0:
                          {
                              mDirector.startAction();
                              mAccessor.setAttr<AttrType::state>(mAccessor.getAttr<AttrType::description>().defaultState, NotificationType::synchronous);
                              mDirector.endAction(juce::translate("Restore track factory properties"), ActionState::apply);
                          }
                          break;
                          case 1:
                          {
                              // Ignore (custom)
                          }
                          break;
                          case 2:
                          {
                              mDirector.startAction();
                              if(Exporter::fromPreset(mAccessor, AlertType::window))
                              {
                                  mDirector.endAction(juce::translate("Load track properties from file"), ActionState::apply);
                              }
                              else
                              {
                                  mDirector.endAction(juce::translate("Load track properties from file"), ActionState::abort);
                              }
                          }
                          break;
                          case 3:
                          {
                              Exporter::toPreset(mAccessor, AlertType::window);
                          }
                          break;
                          default:
                              break;
                      };
                      auto const& state = mAccessor.getAttr<AttrType::state>();
                      auto const& description = mAccessor.getAttr<AttrType::description>();
                      mPropertyPreset.entry.setSelectedItemIndex(state != description.defaultState, juce::NotificationType::dontSendNotification);
                  })

, mPropertyColourMap("Color Map", "The color map of the graphical renderer.", "", std::vector<std::string>{"Parula", "Heat", "Jet", "Turbo", "Hot", "Gray", "Magma", "Inferno", "Plasma", "Viridis", "Cividis", "Github"}, [&](size_t index)
                     {
                         mDirector.startAction();
                         auto colours = mAccessor.getAttr<AttrType::colours>();
                         colours.map = static_cast<ColourMap>(index);
                         mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
                         mDirector.endAction(juce::translate("Change track color map"), ActionState::apply);
                     })
, mPropertyColourMapAlpha("Transparency", "The transparency of the graphical renderer", "", {0.0f, 1.0f}, 0.0f, [&](float value)
                          {
                              mDirector.startAction();
                              auto colours = mAccessor.getAttr<AttrType::colours>();
                              colours.background = juce::Colours::black.withAlpha(value);
                              mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
                              mDirector.endAction(juce::translate("Change track transparency"), ActionState::apply);
                          })
, mPropertyForegroundColour(
      "Foreground Color", "The foreground current color of the graphical renderer.", "Select the foreground color", [&](juce::Colour const& colour)
      {
          if(!mPropertyForegroundColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction();
          }
          auto colours = mAccessor.getAttr<AttrType::colours>();
          colours.foreground = colour;
          mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
          if(!mPropertyForegroundColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(juce::translate("Change track foreground color"), ActionState::apply);
          }
      },
      [&]()
      {
          mDirector.startAction();
      },
      [&]()
      {
          mDirector.endAction(juce::translate("Change track foreground color"), ActionState::apply);
      })
, mPropertyBackgroundColour(
      "Background Color", "The background current color of the graphical renderer.", "Select the background color", [&](juce::Colour const& colour)
      {
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction();
          }
          auto colours = mAccessor.getAttr<AttrType::colours>();
          colours.background = colour;
          mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
          if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(juce::translate("Change track background color"), ActionState::apply);
          }
      },
      [&]()
      {
          mDirector.startAction();
      },
      [&]()
      {
          mDirector.endAction(juce::translate("Change track background color"), ActionState::apply);
      })
, mPropertyTextColour(
      "Text Color", "The text current color of the graphical renderer.", "Select the text color", [&](juce::Colour const& colour)
      {
          if(!mPropertyTextColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction();
          }
          auto colours = mAccessor.getAttr<AttrType::colours>();
          colours.text = colour;
          mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
          if(!mPropertyTextColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(juce::translate("Change track text color"), ActionState::apply);
          }
      },
      [&]()
      {
          mDirector.startAction();
      },
      [&]()
      {
          mDirector.endAction(juce::translate("Change track text color"), ActionState::apply);
      })
, mPropertyShadowColour(
      "Shadow Color", "The shadow current color of the graphical renderer.", "Select the shadow color", [&](juce::Colour const& colour)
      {
          if(!mPropertyShadowColour.entry.isColourSelectorVisible())
          {
              mDirector.startAction();
          }
          auto colours = mAccessor.getAttr<AttrType::colours>();
          colours.shadow = colour;
          mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
          if(!mPropertyShadowColour.entry.isColourSelectorVisible())
          {
              mDirector.endAction(juce::translate("Change track shadow color"), ActionState::apply);
          }
      },
      [&]()
      {
          mDirector.startAction();
      },
      [&]()
      {
          mDirector.endAction(juce::translate("Change track shadow color"), ActionState::apply);
      })
, mPropertyValueRangeMode("Value Range Mode", "The mode of the value range.", "", std::vector<std::string>{"Plugin", "Results", "Manual"}, [&](size_t index)
                          {
                              auto applyRange = [&](std::optional<Zoom::Range> const& globalRange)
                              {
                                  if(!globalRange.has_value() || globalRange->isEmpty())
                                  {
                                      return;
                                  }
                                  mDirector.startAction();
                                  auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                                  auto const visibleRange = Zoom::Tools::getScaledVisibleRange(zoomAcsr, *globalRange);
                                  zoomAcsr.setAttr<Zoom::AttrType::globalRange>(*globalRange, NotificationType::synchronous);
                                  zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(visibleRange, NotificationType::synchronous);
                                  mDirector.endAction(juce::translate("Change track minimum and maximum values"), ActionState::apply);
                              };

                              switch(index)
                              {
                                  case 0:
                                  {
                                      applyRange(Tools::getValueRange(mAccessor.getAttr<AttrType::description>()));
                                  }
                                  break;
                                  case 1:
                                  {
                                      applyRange(Tools::getValueRange(mAccessor.getAttr<AttrType::results>()));
                                  }
                                  break;
                                  default:
                                      break;
                              }
                          })
, mPropertyValueRangeMin("Value Range Min.", "The minimum value of the output.", "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float value)
                         {
                             mDirector.startAction();
                             auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                             auto const end = std::max(zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getEnd(), static_cast<double>(value) + 1.0);
                             zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{static_cast<double>(value), end}, NotificationType::synchronous);
                             mDirector.endAction(juce::translate("Change track minimum value"), ActionState::apply);
                         })
, mPropertyValueRangeMax("Value Range Max.", "The maximum value of the output.", "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float value)
                         {
                             mDirector.startAction();
                             auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                             auto const start = std::min(zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getStart(), static_cast<double>(value) - 1.0);
                             zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{start, static_cast<double>(value)}, NotificationType::synchronous);
                             mDirector.endAction(juce::translate("Change track maximum value"), ActionState::apply);
                         })
, mPropertyValueRange("Value Range", "The range of the output.", "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float min, float max)
                      {
                          mDirector.startAction();
                          auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                          zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range(min, max), NotificationType::synchronous);
                          mDirector.endAction(juce::translate("Change track value range"), ActionState::apply);
                      })
, mPropertyRangeLink("Range Link", "Toggle the group link for zoom range.", [&](bool value)
                     {
                         mDirector.startAction();
                         mAccessor.setAttr<AttrType::zoomLink>(value, NotificationType::synchronous);
                         mDirector.endAction(juce::translate("Change range link state"), ActionState::apply);
                     })
, mPropertyNumBins("Num Bins", "The number of bins.", "", {0.0f, static_cast<float>(Zoom::max())}, 1.0f, nullptr)
{
    auto updateValueZoomMode = [&]()
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
    };

    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        auto constexpr silent = juce::NotificationType::dontSendNotification;
        switch(attribute)
        {
            case AttrType::name:
            {
                mPropertyName.entry.setText(acsr.getAttr<AttrType::name>(), silent);
                mFloatingWindow.setName(juce::translate("ANLNAME PROPERTIES").replace("ANLNAME", acsr.getAttr<AttrType::name>().toUpperCase()));
            }
            break;

            case AttrType::key:
                break;
            case AttrType::description:
            {
                auto createProperty = [&](Plugin::Parameter const& parameter) -> std::unique_ptr<juce::Component>
                {
                    if(!parameter.valueNames.empty())
                    {
                        return std::make_unique<PropertyList>(parameter.name, parameter.description, parameter.unit, parameter.valueNames, [=, this](size_t index)
                                                              {
                                                                  applyParameterValue(parameter, static_cast<float>(index));
                                                              });
                    }
                    else if(parameter.isQuantized && std::abs(parameter.quantizeStep - 1.0f) < std::numeric_limits<float>::epsilon() && std::abs(parameter.minValue) < std::numeric_limits<float>::epsilon() && std::abs(parameter.maxValue - 1.0f) < std::numeric_limits<float>::epsilon())
                    {
                        return std::make_unique<PropertyToggle>(parameter.name, parameter.description, [=, this](bool state)
                                                                {
                                                                    applyParameterValue(parameter, state ? 1.0f : 0.f);
                                                                });
                    }

                    auto const description = juce::String(parameter.description) + " [" + juce::String(parameter.minValue, 2) + ":" + juce::String(parameter.maxValue, 2) + (!parameter.isQuantized ? "" : ("-" + juce::String(parameter.quantizeStep, 2))) + "]";
                    return std::make_unique<PropertyNumber>(parameter.name, description, parameter.unit, juce::Range<float>{parameter.minValue, parameter.maxValue}, parameter.isQuantized ? parameter.quantizeStep : 0.0f, [=, this](float value)
                                                            {
                                                                applyParameterValue(parameter, value);
                                                            });
                };

                auto const description = mAccessor.getAttr<AttrType::description>();
                std::vector<ConcertinaTable::ComponentRef> components;

                // Processor Part
                if(description.inputDomain == Plugin::InputDomain::FrequencyDomain)
                {
                    components.push_back(mPropertyWindowType);
                    components.push_back(mPropertyWindowSize);
                    components.push_back(mPropertyWindowOverlapping);
                }
                else
                {
                    components.push_back(mPropertyBlockSize);
                    components.push_back(mPropertyStepSize);
                }

                mParameterProperties.clear();
                for(auto const& parameter : description.parameters)
                {
                    auto property = createProperty(parameter);
                    anlWeakAssert(property != nullptr);
                    if(property != nullptr)
                    {
                        components.push_back(*property.get());
                        mParameterProperties[parameter.identifier] = std::move(property);
                    }
                }
                components.push_back(mPropertyPreset);
                components.push_back(mProgressBarAnalysis);
                mProcessorSection.setComponents(components);
                components.clear();

                // Graphical Part
                auto const output = description.output;
                mPropertyValueRangeMin.entry.setTextValueSuffix(output.unit);
                mPropertyValueRangeMax.entry.setTextValueSuffix(output.unit);

                switch(Tools::getDisplayType(acsr))
                {
                    case Tools::DisplayType::markers:
                    {
                        // clang-format off
                        mGraphicalSection.setComponents({
                                                              mPropertyForegroundColour
                                                            , mPropertyTextColour
                                                            , mPropertyBackgroundColour
                                                            , mPropertyShadowColour
                                                        });
                        // clang-format on
                    }
                    break;
                    case Tools::DisplayType::segments:
                    {
                        mPropertyRangeLink.title.setText("Value Range Link", juce::NotificationType::dontSendNotification);
                        // clang-format off
                        mGraphicalSection.setComponents({
                                                              mPropertyForegroundColour
                                                            , mPropertyTextColour
                                                            , mPropertyBackgroundColour
                                                            , mPropertyShadowColour
                                                            , mPropertyValueRangeMode
                                                            , mPropertyValueRangeMin
                                                            , mPropertyValueRangeMax
                                                            , mPropertyRangeLink
                                                        });
                        // clang-format on
                    }
                    break;
                    case Tools::DisplayType::grid:
                    {
                        mPropertyRangeLink.title.setText("Bin Range Link", juce::NotificationType::dontSendNotification);
                        mPropertyNumBins.entry.setEnabled(false);
                        // clang-format off
                        mGraphicalSection.setComponents({
                                                              mPropertyColourMap
                                                            , mPropertyColourMapAlpha
                                                            , mPropertyValueRangeMode
                                                            , mPropertyValueRangeMin
                                                            , mPropertyValueRangeMax
                                                            , mPropertyValueRange
                                                            , mPropertyNumBins
                                                            , mPropertyRangeLink
                                                            , mProgressBarRendering
                                                        });
                        // clang-format on
                    }
                    break;
                }

                // Plugin Information Part
                mPropertyPluginName.entry.setText(description.name, silent);
                mPropertyPluginFeature.entry.setText(description.output.name, silent);
                mPropertyPluginMaker.entry.setText(description.maker, silent);
                mPropertyPluginVersion.entry.setText(juce::String(description.version), silent);
                mPropertyPluginCategory.entry.setText(description.category.isEmpty() ? "-" : description.category, silent);
                mPropertyPluginDetails.setText(description.details + " - " + description.output.description, silent);

                updateValueZoomMode();
            }
            case AttrType::state:
            {
                auto const description = mAccessor.getAttr<AttrType::description>();
                auto const state = mAccessor.getAttr<AttrType::state>();
                if(description.inputDomain == Plugin::InputDomain::FrequencyDomain)
                {
                    mPropertyWindowType.entry.setSelectedId(static_cast<int>(state.windowType) + 1, silent);
                    auto const windowSizeIndex = static_cast<int>(std::log(state.blockSize) / std::log(2)) - 2;
                    mPropertyWindowSize.entry.setSelectedId(windowSizeIndex, silent);
                    mPropertyWindowOverlapping.entry.clear(silent);
                    for(int i = 1; static_cast<size_t>(i) <= state.blockSize; i *= 2)
                    {
                        mPropertyWindowOverlapping.entry.addItem(juce::String(i) + "x", static_cast<int>(state.blockSize) / i);
                    }
                    mPropertyWindowOverlapping.entry.setSelectedId(static_cast<int>(state.stepSize), silent);
                }
                else
                {
                    mPropertyBlockSize.entry.setValue(static_cast<double>(state.blockSize), silent);
                    mPropertyStepSize.entry.setValue(static_cast<double>(state.stepSize), silent);
                }
                for(auto const& parameter : state.parameters)
                {
                    auto it = mParameterProperties.find(parameter.first);
                    anlWeakAssert(it != mParameterProperties.end());
                    if(it != mParameterProperties.end() && it->second != nullptr)
                    {
                        if(auto* propertyList = dynamic_cast<PropertyList*>(it->second.get()))
                        {
                            propertyList->entry.setSelectedItemIndex(static_cast<int>(parameter.second), silent);
                        }
                        else if(auto* propertyNumber = dynamic_cast<PropertyNumber*>(it->second.get()))
                        {
                            propertyNumber->entry.setValue(static_cast<float>(parameter.second), silent);
                        }
                        else if(auto* propertyToggle = dynamic_cast<PropertyToggle*>(it->second.get()))
                        {
                            propertyToggle->entry.setToggleState(static_cast<bool>(parameter.second), silent);
                        }
                        else
                        {
                            anlStrongAssert(false && "property unsupported");
                        }
                    }
                }

                mPropertyPreset.entry.setSelectedItemIndex(state != description.defaultState, juce::NotificationType::dontSendNotification);
                mPropertyPreset.entry.setItemEnabled(1, true);
                mPropertyPreset.entry.setItemEnabled(2, false);
            }
            break;
            case AttrType::results:
            {
                updateValueZoomMode();
            }
            break;
            case AttrType::graphics:
            case AttrType::processing:
            case AttrType::warnings:
                break;
            case AttrType::colours:
            {
                auto const colours = acsr.getAttr<AttrType::colours>();
                mPropertyBackgroundColour.entry.setCurrentColour(colours.background, juce::NotificationType::dontSendNotification);
                mPropertyForegroundColour.entry.setCurrentColour(colours.foreground, juce::NotificationType::dontSendNotification);
                mPropertyTextColour.entry.setCurrentColour(colours.text, juce::NotificationType::dontSendNotification);
                mPropertyShadowColour.entry.setCurrentColour(colours.shadow, juce::NotificationType::dontSendNotification);
                mPropertyColourMap.entry.setSelectedItemIndex(static_cast<int>(colours.map), juce::NotificationType::dontSendNotification);
                mPropertyColourMapAlpha.entry.setValue(static_cast<double>(colours.background.getFloatAlpha()), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::time:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::zoomAcsr:
            case AttrType::focused:
                break;
            case AttrType::zoomLink:
            {
                mPropertyRangeLink.entry.setToggleState(acsr.getAttr<AttrType::zoomLink>(), juce::NotificationType::dontSendNotification);
            }
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

                updateValueZoomMode();
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

    mBoundsListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        resized();
    };
    mBoundsListener.attachTo(mProcessorSection);
    mBoundsListener.attachTo(mGraphicalSection);
    mBoundsListener.attachTo(mPluginSection);

    mProgressBarAnalysis.setSize(sInnerWidth, 36);
    mProgressBarRendering.setSize(sInnerWidth, 36);

    mPropertyPluginDetails.setTooltip(juce::translate("The details of the plugin"));
    mPropertyPluginDetails.setSize(sInnerWidth, 48);
    mPropertyPluginDetails.setJustification(juce::Justification::horizontallyJustified);
    mPropertyPluginDetails.setMultiLine(true);
    mPropertyPluginDetails.setReadOnly(true);
    mPropertyPluginDetails.setScrollbarsShown(true);
    // clang-format off
    mPluginSection.setComponents({
                                      mPropertyPluginName
                                    , mPropertyPluginFeature
                                    , mPropertyPluginMaker
                                    , mPropertyPluginVersion
                                    , mPropertyPluginCategory
                                    , mPropertyPluginDetails
                                 });
    // clang-format on

    addAndMakeVisible(mPropertyName);
    addAndMakeVisible(mProcessorSection);
    addAndMakeVisible(mGraphicalSection);
    addAndMakeVisible(mPluginSection);

    setSize(sInnerWidth, 400);
    mViewport.setSize(sInnerWidth + mViewport.getVerticalScrollBar().getWidth() + 2, 400);
    mViewport.setScrollBarsShown(true, false, true, false);
    mViewport.setViewedComponent(this, false);
    mFloatingWindow.setResizable(true, false);
    mBoundsConstrainer.setMinimumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMaximumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMinimumHeight(120);
    mBoundsConstrainer.setMinimumOnscreenAmounts(0, 40, 40, 40);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);

    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mBinZoomListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::PropertyPanel::~PropertyPanel()
{
    mBoundsListener.detachFrom(mProcessorSection);
    mBoundsListener.detachFrom(mGraphicalSection);
    mBoundsListener.detachFrom(mPluginSection);
    mAccessor.removeListener(mListener);
    mAccessor.getAcsr<AcsrType::binZoom>().removeListener(mBinZoomListener);
    mAccessor.getAcsr<AcsrType::valueZoom>().removeListener(mValueZoomListener);
}

void Track::PropertyPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mPropertyName.setBounds(bounds.removeFromTop(mPropertyName.getHeight()));
    mProcessorSection.setBounds(bounds.removeFromTop(mProcessorSection.getHeight()));
    mGraphicalSection.setBounds(bounds.removeFromTop(mGraphicalSection.getHeight()));
    mPluginSection.setBounds(bounds.removeFromTop(mPluginSection.getHeight()));
    setSize(sInnerWidth, std::max(bounds.getY(), 120) + 2);
}

void Track::PropertyPanel::applyParameterValue(Plugin::Parameter const& parameter, float value)
{
    mDirector.startAction();
    auto state = mAccessor.getAttr<AttrType::state>();
    anlWeakAssert(value >= parameter.minValue && value <= parameter.maxValue);
    state.parameters[parameter.identifier] = std::min(std::max(value, parameter.minValue), parameter.maxValue);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
    mDirector.endAction(juce::translate("Change track property"), ActionState::apply);
}

ANALYSE_FILE_END
