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
                    mDirector.endAction(ActionState::newTransaction, juce::translate("Change track name"));
                })

, mPropertyResultsFile("Results File", "The path of the results file", [this]()
                       {
                           auto const trackFile = mAccessor.getAttr<AttrType::file>();
                           if(trackFile.file.existsAsFile())
                           {
                               if(juce::Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isCtrlDown())
                               {
                                   mDirector.startAction();
                                   if(!canModifyProcessor())
                                   {
                                       mDirector.endAction(ActionState::abort);
                                       return;
                                   }
                                   mDirector.endAction(ActionState::newTransaction, juce::translate("Detach the track's results file"));
                               }
                               else
                               {
                                   trackFile.file.revealToUser();
                               }
                           }
                           else
                           {
                               auto const options = juce::MessageBoxOptions()
                                                        .withIconType(juce::AlertWindow::WarningIcon)
                                                        .withTitle(juce::translate("Results file cannot be found!"))
                                                        .withMessage(juce::translate("The results file cannot be found. Would you like to select another file, to run the plugin or to continue with the missing file?"))
                                                        .withButton(juce::translate("Select File"))
                                                        .withButton(juce::translate("Run Plugin"))
                                                        .withButton(juce::translate("Continue"));
                               juce::WeakReference<juce::Component> safePointer(this);
                               juce::AlertWindow::showAsync(options, [=, this](int result)
                                                            {
                                                                if(safePointer.get() == nullptr)
                                                                {
                                                                    return;
                                                                }
                                                                if(result == 1)
                                                                {
                                                                    mDirector.askForResultsFile(juce::translate("Load analysis results..."), trackFile.file, NotificationType::synchronous);
                                                                }
                                                                else if(result == 2)
                                                                {
                                                                    mDirector.startAction();
                                                                    mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
                                                                    mAccessor.setAttr<AttrType::file>(FileInfo{}, NotificationType::synchronous);
                                                                    mDirector.endAction(ActionState::newTransaction, juce::translate("Remove results file"));
                                                                }
                                                            });
                           }
                       })
, mPropertyWindowType("Window Type", "The window type of the FFT.", "", std::vector<std::string>{"Rectangular", "Triangular", "Hamming", "Hanning", "Blackman", "Nuttall", "Blackman-Harris"}, [&](size_t index)
                      {
                          mDirector.startAction();
                          if(!canModifyProcessor())
                          {
                              mListener.onAttrChanged(mAccessor, AttrType::results);
                              mDirector.endAction(ActionState::abort);
                              return;
                          }

                          auto state = mAccessor.getAttr<AttrType::state>();
                          state.windowType = static_cast<Plugin::WindowType>(index);
                          mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                          mDirector.endAction(ActionState::newTransaction, juce::translate("Change track window type"));
                      })
, mPropertyBlockSize("Block Size", "The block size used by the track.", "samples", std::vector<std::string>{"8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384"}, [&](size_t index)
                     {
                         juce::ignoreUnused(index);
                         auto const blockSize = static_cast<size_t>(mPropertyBlockSize.entry.getText().getIntValue());
                         auto state = mAccessor.getAttr<AttrType::state>();
                         if(state.blockSize == blockSize)
                         {
                             return;
                         }
                         mDirector.startAction();
                         if(!canModifyProcessor())
                         {
                             mListener.onAttrChanged(mAccessor, AttrType::results);
                             mDirector.endAction(ActionState::abort);
                             return;
                         }
                         auto const overlapping = static_cast<double>(state.blockSize) / static_cast<double>(state.stepSize);
                         state.blockSize = blockSize;
                         state.stepSize = static_cast<size_t>(std::round(static_cast<double>(state.blockSize) / overlapping));
                         mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change track block size"));
                     })
, mPropertyStepSize("Step Size", "The step size (overlapping) used by the track.", "x", std::vector<std::string>{}, [&](size_t index)
                    {
                        juce::ignoreUnused(index);
                        auto state = mAccessor.getAttr<AttrType::state>();
                        auto const stepSize = static_cast<size_t>(mPropertyStepSize.entry.getText().getIntValue());
                        if(state.stepSize == stepSize)
                        {
                            return;
                        }

                        mDirector.startAction();
                        if(!canModifyProcessor())
                        {
                            mListener.onAttrChanged(mAccessor, AttrType::results);
                            mDirector.endAction(ActionState::abort);
                            return;
                        }

                        state.stepSize = stepSize;
                        mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                        mDirector.endAction(ActionState::newTransaction, juce::translate("Change track window overlapping"));
                    })
, mPropertyPreset("Preset", "The preset of the track", "", std::vector<std::string>{"Factory", "Custom", "Load...", "Save..."}, [&](size_t index)
                  {
                      switch(index)
                      {
                          case 0:
                          {
                              mDirector.startAction();
                              if(!canModifyProcessor())
                              {
                                  mListener.onAttrChanged(mAccessor, AttrType::results);
                                  mDirector.endAction(ActionState::abort);
                                  return;
                              }

                              mAccessor.setAttr<AttrType::state>(mAccessor.getAttr<AttrType::description>().defaultState, NotificationType::synchronous);
                              mDirector.endAction(ActionState::newTransaction, juce::translate("Restore track factory properties"));
                          }
                          break;
                          case 1:
                          {
                              // Ignore (custom)
                          }
                          break;
                          default:
                          {
                              index -= 2;
                              auto const& programs = mAccessor.getAttr<AttrType::description>().programs;
                              if(static_cast<size_t>(index) == programs.size())
                              {
                                  mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Load from preset..."), juce::File{}, App::getFileWildCardFor("preset"));
                                  if(mFileChooser == nullptr)
                                  {
                                      return;
                                  }
                                  using Flags = juce::FileBrowserComponent::FileChooserFlags;
                                  mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [this](juce::FileChooser const& fileChooser)
                                                            {
                                                                auto const results = fileChooser.getResults();
                                                                if(results.isEmpty())
                                                                {
                                                                    return;
                                                                }
                                                                mDirector.startAction();
                                                                if(!canModifyProcessor())
                                                                {
                                                                    mListener.onAttrChanged(mAccessor, AttrType::results);
                                                                    mDirector.endAction(ActionState::abort);
                                                                    return;
                                                                }

                                                                auto const result = Exporter::fromPreset(mAccessor, results.getFirst());
                                                                if(result.failed())
                                                                {
                                                                    mDirector.endAction(ActionState::abort);
                                                                    AlertWindow::showMessage(AlertWindow::MessageType::warning, "Load from preset failed!", result.getErrorMessage());
                                                                }
                                                                else
                                                                {
                                                                    mDirector.endAction(ActionState::newTransaction, juce::translate("Load track properties from preset file"));
                                                                }
                                                            });
                                  break;
                              }
                              else if(static_cast<size_t>(index) == programs.size() + 1)
                              {
                                  mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Save as preset..."), juce::File{}, App::getFileWildCardFor("preset"));
                                  if(mFileChooser == nullptr)
                                  {
                                      return;
                                  }
                                  using Flags = juce::FileBrowserComponent::FileChooserFlags;
                                  mFileChooser->launchAsync(Flags::saveMode | Flags::canSelectFiles | Flags::warnAboutOverwriting, [this](juce::FileChooser const& fileChooser)
                                                            {
                                                                auto const results = fileChooser.getResults();
                                                                if(results.isEmpty())
                                                                {
                                                                    return;
                                                                }

                                                                auto const result = Exporter::toPreset(mAccessor, results.getFirst());
                                                                if(result.failed())
                                                                {
                                                                    AlertWindow::showMessage(AlertWindow::MessageType::warning, "Save as preset failed!", result.getErrorMessage());
                                                                }
                                                            });
                                  break;
                              }
                              anlWeakAssert(static_cast<size_t>(index) < programs.size());
                              if(static_cast<size_t>(index) >= programs.size())
                              {
                                  break;
                              }
                              auto const it = std::next(programs.cbegin(), static_cast<long>(index));
                              mDirector.startAction();
                              if(!canModifyProcessor())
                              {
                                  mListener.onAttrChanged(mAccessor, AttrType::results);
                                  mDirector.endAction(ActionState::abort);
                                  return;
                              }

                              mAccessor.setAttr<AttrType::state>(it->second, NotificationType::synchronous);
                              mDirector.endAction(ActionState::newTransaction, juce::translate("Apply track preset properties"));
                          }
                          break;
                      };
                      updatePresets();
                  })

, mPropertyColourMap("Color Map", "The color map of the graphical renderer.", "", std::vector<std::string>{"Parula", "Heat", "Jet", "Turbo", "Hot", "Gray", "Magma", "Inferno", "Plasma", "Viridis", "Cividis", "Github"}, [&](size_t index)
                     {
                         mDirector.startAction();
                         auto colours = mAccessor.getAttr<AttrType::colours>();
                         colours.map = static_cast<ColourMap>(index);
                         mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change track color map"));
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
, mPropertyValueRangeMode("Value Range Mode", "The mode of the value range.", "", std::vector<std::string>{"Default", "Results", "Manual"}, [&](size_t index)
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
                                  mDirector.endAction(ActionState::newTransaction, juce::translate("Change track minimum and maximum values"));
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
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track minimum value"));
                         })
, mPropertyValueRangeMax("Value Range Max.", "The maximum value of the output.", "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float value)
                         {
                             mDirector.startAction();
                             auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                             auto const start = std::min(zoomAcsr.getAttr<Zoom::AttrType::globalRange>().getStart(), static_cast<double>(value) - 1.0);
                             zoomAcsr.setAttr<Zoom::AttrType::globalRange>(Zoom::Range{start, static_cast<double>(value)}, NotificationType::synchronous);
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track maximum value"));
                         })
, mPropertyValueRange("Value Range", "The range of the output.", "", {static_cast<float>(Zoom::lowest()), static_cast<float>(Zoom::max())}, 0.0f, [&](float min, float max)
                      {
                          mDirector.startAction();
                          auto& zoomAcsr = mAccessor.getAcsr<AcsrType::valueZoom>();
                          zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(Zoom::Range(min, max), NotificationType::synchronous);
                          mDirector.endAction(ActionState::newTransaction, juce::translate("Change track value range"));
                      })
, mPropertyGrid("Grid", "Edit the grid properties", [&]()
                {
                    auto& zoomAcsr = Tools::getDisplayType(mAccessor) == Tools::DisplayType::columns ? mAccessor.getAcsr<AcsrType::binZoom>() : mAccessor.getAcsr<AcsrType::valueZoom>();
                    auto& gridAcsr = zoomAcsr.getAcsr<Zoom::AcsrType::grid>();
                    mZoomGridPropertyPanel.setGrid(gridAcsr);
                    mZoomGridPropertyPanel.onChanged = [&](Zoom::Grid::Accessor const& acsr)
                    {
                        gridAcsr.copyFrom(acsr, NotificationType::synchronous);
                    };
                    mZoomGridPropertyPanel.show();
                })
, mPropertyRangeLink("Range Link", "Toggle the group link for zoom range.", [&](bool value)
                     {
                         mDirector.startAction();
                         mAccessor.setAttr<AttrType::zoomLink>(value, NotificationType::synchronous);
                         mDirector.endAction(ActionState::newTransaction, juce::translate("Change range link state"));
                     })
, mPropertyNumBins("Num Bins", "The number of bins.", "", {0.0f, static_cast<float>(Zoom::max())}, 1.0f, nullptr)
, mPropertyChannelLayout("Channel Layout", "The visible state of the channels.", [&]()
                         {
                             showChannelLayout();
                         })
{
    mPropertyBlockSize.entry.getProperties().set("isNumber", true);
    NumberField::Label::storeProperties(mPropertyBlockSize.entry.getProperties(), {1.0, 65536.0}, 1.0, 0, "samples");
    mPropertyStepSize.entry.getProperties().set("isNumber", true);
    NumberField::Label::storeProperties(mPropertyStepSize.entry.getProperties(), {1.0, 65536.0}, 1.0, 0, "samples");

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
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
            case AttrType::description:
            case AttrType::file:
            case AttrType::results:
            {
                auto createProperty = [&](Plugin::Parameter const& parameter) -> std::unique_ptr<juce::Component>
                {
                    auto const name = juce::String(Format::withFirstCharUpperCase(parameter.name));
                    if(!parameter.valueNames.empty())
                    {
                        return std::make_unique<PropertyList>(name, parameter.description, parameter.unit, parameter.valueNames, [=, this](size_t index)
                                                              {
                                                                  applyParameterValue(parameter, static_cast<float>(index));
                                                              });
                    }
                    else if(parameter.isQuantized && std::abs(parameter.quantizeStep - 1.0f) < std::numeric_limits<float>::epsilon() && std::abs(parameter.minValue) < std::numeric_limits<float>::epsilon() && std::abs(parameter.maxValue - 1.0f) < std::numeric_limits<float>::epsilon())
                    {
                        return std::make_unique<PropertyToggle>(name, parameter.description, [=, this](bool state)
                                                                {
                                                                    applyParameterValue(parameter, state ? 1.0f : 0.f);
                                                                });
                    }

                    auto const description = juce::String(parameter.description) + " [" + juce::String(parameter.minValue, 2) + ":" + juce::String(parameter.maxValue, 2) + (!parameter.isQuantized ? "" : ("-" + juce::String(parameter.quantizeStep, 2))) + "]";
                    return std::make_unique<PropertyNumber>(name, description, parameter.unit, juce::Range<float>{parameter.minValue, parameter.maxValue}, parameter.isQuantized ? parameter.quantizeStep : 0.0f, [=, this](float value)
                                                            {
                                                                applyParameterValue(parameter, value);
                                                            });
                };

                auto const key = mAccessor.getAttr<AttrType::key>();
                auto const description = mAccessor.getAttr<AttrType::description>();
                auto const results = mAccessor.getAttr<AttrType::results>();
                std::vector<ConcertinaTable::ComponentRef> components;

                // Processor Part
                auto const resultsFile = mAccessor.getAttr<AttrType::file>().file;
                mPropertyResultsFile.entry.setButtonText(resultsFile.getFileName());
                mPropertyResultsFile.entry.setTooltip(resultsFile.getFullPathName());
                if(resultsFile != juce::File{})
                {
                    components.push_back(mPropertyResultsFileInfo);
                    components.push_back(mPropertyResultsFile);
                }
                if(!key.identifier.empty())
                {
                    if(description.inputDomain == Plugin::InputDomain::FrequencyDomain)
                    {
                        components.push_back(mPropertyWindowType);
                    }

                    components.push_back(mPropertyBlockSize);
                    components.push_back(mPropertyStepSize);

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
                }

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
                                                            , mPropertyChannelLayout
                                                        });
                        // clang-format on
                    }
                    break;
                    case Tools::DisplayType::points:
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
                                                            , mPropertyGrid
                                                            , mPropertyChannelLayout
                                                        });
                        // clang-format on
                    }
                    break;
                    case Tools::DisplayType::columns:
                    {
                        mPropertyRangeLink.title.setText("Bin Range Link", juce::NotificationType::dontSendNotification);
                        mPropertyNumBins.entry.setEnabled(false);
                        // clang-format off
                        mGraphicalSection.setComponents({
                                                              mPropertyColourMap
                                                            , mPropertyValueRangeMode
                                                            , mPropertyValueRangeMin
                                                            , mPropertyValueRangeMax
                                                            , mPropertyValueRange
                                                            , mPropertyNumBins
                                                            , mPropertyRangeLink
                                                            , mPropertyGrid
                                                            , mPropertyChannelLayout
                                                            , mProgressBarRendering
                                                        });
                        // clang-format on
                    }
                    break;
                }

                // Plugin Information Part
                mPluginSection.setVisible(!key.identifier.empty());
                mPropertyPluginName.entry.setText(description.name, silent);
                mPropertyPluginFeature.entry.setText(description.output.name, silent);
                mPropertyPluginMaker.entry.setText(description.maker, silent);
                mPropertyPluginVersion.entry.setText(juce::String(description.version), silent);
                mPropertyPluginCategory.entry.setText(description.category.isEmpty() ? "-" : description.category, silent);
                auto const details = juce::String(description.output.description) + (description.output.description.empty() ? "" : "\n") + description.details;
                mPropertyPluginDetails.setText(details, silent);

                auto const& programs = mAccessor.getAttr<AttrType::description>().programs;
                mPropertyPreset.entry.clear(juce::NotificationType::dontSendNotification);

                mPropertyPreset.entry.addItem("Factory", 1);
                mPropertyPreset.entry.addItem("Custom", 2);
                mPropertyPreset.entry.setItemEnabled(2, false);
                mPropertyPreset.entry.addSeparator();
                juce::StringArray items;
                for(auto const& program : programs)
                {
                    auto const name = Format::withFirstCharUpperCase(program.first);
                    items.add(juce::String(name));
                }
                mPropertyPreset.entry.addItemList(items, 3);
                mPropertyPreset.entry.addSeparator();
                mPropertyPreset.entry.addItem("Load...", items.size() + 3);
                mPropertyPreset.entry.addItem("Save...", items.size() + 4);

                updateZoomMode();
                resized();
            }
            case AttrType::state:
            {
                auto const description = mAccessor.getAttr<AttrType::description>();
                auto const state = mAccessor.getAttr<AttrType::state>();
                mPropertyWindowType.entry.setSelectedId(static_cast<int>(state.windowType) + 1, silent);
                mPropertyBlockSize.entry.setEditableText(description.inputDomain == Plugin::InputDomain::TimeDomain);
                mPropertyBlockSize.entry.setText(juce::String(state.blockSize) + "samples", silent);
                mPropertyStepSize.entry.setEditableText(description.inputDomain == Plugin::InputDomain::TimeDomain);
                mPropertyStepSize.entry.clear(silent);
                for(int i = 1; static_cast<size_t>(i) <= state.blockSize; i *= 2)
                {
                    mPropertyStepSize.entry.addItem(juce::String(i) + "samples", i);
                }
                mPropertyStepSize.entry.setText(juce::String(state.stepSize) + "samples", silent);

                for(auto const& parameter : state.parameters)
                {
                    auto it = mParameterProperties.find(parameter.first);
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
                            anlWeakAssert(false && "property unsupported");
                        }
                    }
                }

                updatePresets();
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
            }
            break;
            case AttrType::channelsLayout:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::zoomAcsr:
            case AttrType::focused:
            case AttrType::grid:
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

    mComponentListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        resized();
    };
    mComponentListener.attachTo(mProcessorSection);
    mComponentListener.attachTo(mGraphicalSection);
    mComponentListener.attachTo(mPluginSection);

    mZoomGridPropertyPanel.onShow = [this]()
    {
        mDirector.startAction();
    };

    mZoomGridPropertyPanel.onHide = [this]()
    {
        mDirector.endAction(ActionState::newTransaction, "Edit Grid Properties");
    };

    mProgressBarAnalysis.setSize(sInnerWidth, 36);
    mProgressBarRendering.setSize(sInnerWidth, 36);

    mPropertyResultsFileInfo.setText("Analysis results were consolidated or loaded from a file.", false);
    mPropertyResultsFileInfo.setSize(sInnerWidth, 24);

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
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);

    // This avoids to move the focus to the first component
    // (the name property) when recreating properties.
    setWantsKeyboardFocus(true);
    mAccessor.getAcsr<AcsrType::valueZoom>().addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAcsr<AcsrType::binZoom>().addListener(mBinZoomListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::PropertyPanel::~PropertyPanel()
{
    mComponentListener.detachFrom(mProcessorSection);
    mComponentListener.detachFrom(mGraphicalSection);
    mComponentListener.detachFrom(mPluginSection);
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
    if(mPluginSection.isVisible())
    {
        mPluginSection.setBounds(bounds.removeFromTop(mPluginSection.getHeight()));
    }
    setSize(sInnerWidth, std::max(bounds.getY(), 120) + 2);
}

void Track::PropertyPanel::lookAndFeelChanged()
{
    {
        auto const text = mPropertyPluginDetails.getText();
        mPropertyPluginDetails.clear();
        mPropertyPluginDetails.setText(text);
    }
    {
        auto const text = mPropertyResultsFileInfo.getText();
        mPropertyResultsFileInfo.clear();
        mPropertyResultsFileInfo.setText(text);
    }
}

bool Track::PropertyPanel::canModifyProcessor()
{
    auto const file = mAccessor.getAttr<AttrType::file>().file;
    auto const& key = mAccessor.getAttr<AttrType::key>();
    MiscWeakAssert(!key.identifier.empty() && !key.feature.empty());
    if(file != juce::File{} && !key.identifier.empty() && !key.feature.empty())
    {
        if(!AlertWindow::showOkCancel(AlertWindow::MessageType::question, "Plugin Locked!", "Analysis results were consolidated or loaded from a file. Do you want to detach the file to modify the parameters and restart the analysis?"))
        {
            return false;
        }
        mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::file>(FileInfo{}, NotificationType::synchronous);
    }
    return true;
}

void Track::PropertyPanel::applyParameterValue(Plugin::Parameter const& parameter, float value)
{
    mDirector.startAction();
    if(!canModifyProcessor())
    {
        mListener.onAttrChanged(mAccessor, AttrType::results);
        mDirector.endAction(ActionState::abort);
        return;
    }

    auto state = mAccessor.getAttr<AttrType::state>();
    anlWeakAssert(value >= parameter.minValue && value <= parameter.maxValue);
    state.parameters[parameter.identifier] = std::min(std::max(value, parameter.minValue), parameter.maxValue);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
    mDirector.endAction(ActionState::newTransaction, juce::translate("Change track property"));
}

void Track::PropertyPanel::updatePresets()
{
    auto const& state = mAccessor.getAttr<AttrType::state>();
    auto const& description = mAccessor.getAttr<AttrType::description>();
    if(state == description.defaultState)
    {
        mPropertyPreset.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
        return;
    }
    int index = 2;
    for(auto const& program : mAccessor.getAttr<AttrType::description>().programs)
    {
        if(state == program.second)
        {
            mPropertyPreset.entry.setSelectedItemIndex(index, juce::NotificationType::dontSendNotification);
            return;
        }
        ++index;
    }
    mPropertyPreset.entry.setSelectedItemIndex(1, juce::NotificationType::dontSendNotification);
}

void Track::PropertyPanel::updateZoomMode()
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

void Track::PropertyPanel::showChannelLayout()
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

ANALYSE_FILE_END
