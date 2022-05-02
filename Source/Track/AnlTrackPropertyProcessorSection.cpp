#include "AnlTrackPropertyProcessorSection.h"
#include "../Plugin/AnlPluginTools.h"
#include "AnlTrackExporter.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

static std::vector<std::string> getWindowTypeNames()
{
    std::vector<std::string> names;
    for(auto const& name : magic_enum::enum_names<Plugin::WindowType>())
    {
        auto const windowName = std::string(name);
        names.push_back(windowName.substr(0, windowName.find("Window")));
    }
    return names;
}

static std::vector<std::string> getBlockSizeNames()
{
    return {"8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384"};
}

Track::PropertyProcessorSection::PropertyProcessorSection(Director& director)
: mDirector(director)
, mPropertyResultsFile(juce::translate("Results File"), juce::translate("The path of the results file"), [this]()
                       {
                           auto const file = mAccessor.getAttr<AttrType::file>();
                           if(juce::Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isCtrlDown())
                           {
                               mDirector.askToRemoveFile();
                           }
                           else if(file.file.existsAsFile())
                           {
                               file.file.revealToUser();
                           }
                           else if(file.file != juce::File{})
                           {
                               mDirector.askToReloadFile("The file cannot be found!");
                           }
                       })
, mPropertyWindowType(juce::translate("Window Type"), juce::translate("The window type of the FFT used by the track."), "", getWindowTypeNames(), [&](size_t index)
                      {
                          setWindowType(static_cast<Plugin::WindowType>(index));
                      })
, mPropertyBlockSize(juce::translate("Block Size"), juce::translate("The block size used by the track."), "samples", getBlockSizeNames(), [&](size_t index)
                     {
                         juce::ignoreUnused(index);
                         auto const newValue = mPropertyBlockSize.entry.getText().getIntValue();
                         setBlockSize(static_cast<size_t>(std::clamp(newValue, 1, 65536)));
                     })
, mPropertyStepSize(juce::translate("Step Size"), juce::translate("The step size (overlapping) used by the track."), "x", std::vector<std::string>{}, [&](size_t index)
                    {
                        juce::ignoreUnused(index);
                        auto const newValue = mPropertyStepSize.entry.getText().getIntValue();
                        setStepSize(static_cast<size_t>(std::clamp(newValue, 1, 65536)));
                    })
, mPropertyPreset("Preset", "The preset of the track", "", std::vector<std::string>{"Factory", "Custom", "Load...", "Save..."}, [&](size_t index)
                  {
                      switch(index)
                      {
                          case 0:
                          {
                              restoreDefaultPreset();
                          }
                          break;
                          case 1:
                          {
                              // Ignore (custom)
                              updateState();
                          }
                          break;
                          default:
                          {
                              auto const& programs = mAccessor.getAttr<AttrType::description>().programs;
                              if(index - 2_z == programs.size())
                              {
                                  loadPreset();
                              }
                              else if(index - 2_z == programs.size() + 1)
                              {
                                  savePreset();
                              }
                              else
                              {
                                  changePreset(index - 2_z);
                              }
                          }
                          break;
                      };
                  })
{
    mPropertyBlockSize.entry.getProperties().set("isNumber", true);
    NumberField::Label::storeProperties(mPropertyBlockSize.entry.getProperties(), {1.0, 65536.0}, 1.0, 0, "samples");
    mPropertyStepSize.entry.getProperties().set("isNumber", true);
    NumberField::Label::storeProperties(mPropertyStepSize.entry.getProperties(), {1.0, 65536.0}, 1.0, 0, "samples");

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::key:
            case AttrType::description:
            case AttrType::file:
            {
                auto const& description = acsr.getAttr<AttrType::description>();
                auto const file = acsr.getAttr<AttrType::file>();
                auto const fileName = file.file != juce::File{} ? file.file.getFullPathName() : "Modified";
                mPropertyResultsFile.entry.setButtonText(fileName);
                mPropertyResultsFile.entry.setTooltip(file.file.getFullPathName());
                auto const hasFile = Tools::hasResultFile(acsr);
                mPropertyResultsFileInfo.setVisible(hasFile);
                mPropertyResultsFile.setVisible(hasFile);
                auto const hasPlugin = Tools::hasPluginKey(acsr);
                mPropertyPreset.setVisible(hasPlugin);

                mPropertyWindowType.setEnabled(hasPlugin);
                mPropertyBlockSize.setEnabled(hasPlugin);
                mPropertyStepSize.setEnabled(hasPlugin);

                mPropertyWindowType.setVisible(Tools::supportsWindowType(acsr));
                mPropertyBlockSize.setVisible(Tools::supportsBlockSize(acsr));
                mPropertyStepSize.setVisible(Tools::supportsStepSize(acsr));

                mParameterProperties.clear();
                juce::WeakReference<juce::Component> weakReference(this);
                auto applyValue = [=, this](auto const& parameter, float value)
                {
                    if(weakReference.get() == nullptr)
                    {
                        return;
                    }
                    applyParameterValue(parameter, value);
                };
                for(auto const& parameter : description.parameters)
                {
                    auto property = Plugin::Tools::createProperty(parameter, applyValue);
                    anlWeakAssert(property != nullptr);
                    if(property != nullptr)
                    {
                        addAndMakeVisible(property.get());
                        property->setEnabled(hasPlugin);
                        mParameterProperties[parameter.identifier] = std::move(property);
                    }
                }

                auto const& programs = acsr.getAttr<AttrType::description>().programs;
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
                resized();
            }
            case AttrType::state:
            {
                updateState();
            }
            break;
            case AttrType::results:
            case AttrType::name:
            case AttrType::graphics:
            case AttrType::processing:
            case AttrType::warnings:
            case AttrType::colours:
            case AttrType::channelsLayout:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::zoomAcsr:
            case AttrType::focused:
            case AttrType::grid:
            case AttrType::zoomLink:
                break;
        }
    };

    addAndMakeVisible(mPropertyResultsFileInfo);
    addAndMakeVisible(mPropertyResultsFile);
    addAndMakeVisible(mPropertyWindowType);
    addAndMakeVisible(mPropertyBlockSize);
    addAndMakeVisible(mPropertyStepSize);
    addAndMakeVisible(mPropertyPreset);
    addAndMakeVisible(mProgressBarAnalysis);

    mProgressBarAnalysis.setSize(300, 36);
    mPropertyResultsFileInfo.setText(juce::translate("Analysis results were modified, consolidated or loaded from a file."), juce::NotificationType::dontSendNotification);
    mPropertyResultsFileInfo.setSize(300, 24);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::PropertyProcessorSection::~PropertyProcessorSection()
{
    mAccessor.removeListener(mListener);
}

void Track::PropertyProcessorSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyResultsFileInfo);
    setBounds(mPropertyResultsFile);
    setBounds(mPropertyWindowType);
    setBounds(mPropertyBlockSize);
    setBounds(mPropertyStepSize);
    for(auto& property : mParameterProperties)
    {
        MiscWeakAssert(property.second != nullptr);
        if(property.second != nullptr)
        {
            setBounds(*property.second.get());
        }
    }
    setBounds(mPropertyPreset);
    setBounds(mProgressBarAnalysis);
    setSize(getWidth(), bounds.getY());
}

void Track::PropertyProcessorSection::askToModifyProcessor(std::function<bool(bool)> prepare, std::function<void(void)> perform)
{
    auto const file = mAccessor.getAttr<AttrType::file>();
    auto const& key = mAccessor.getAttr<AttrType::key>();
    MiscWeakAssert(!key.identifier.empty() && !key.feature.empty());
    if(!file.isEmpty() && !key.identifier.empty() && !key.feature.empty())
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Plugin results are locked!"))
                                 .withMessage(juce::translate("The plugin analysis results were modified, consolidated or loaded from a file. Do you want to detach the file to modify the parameters and restart the analysis?"))
                                 .withButton(juce::translate("Detach the file"))
                                 .withButton(juce::translate("Cancel changes"));
        juce::WeakReference<juce::Component> weakReference(this);
        juce::AlertWindow::showAsync(options, [=, this](auto result)
                                     {
                                         if(weakReference.get() == nullptr)
                                         {
                                             return;
                                         }
                                         auto const hasChanged = result == 1;
                                         MiscWeakAssert(prepare != nullptr);
                                         if(prepare != nullptr && prepare(hasChanged))
                                         {
                                             mAccessor.setAttr<AttrType::warnings>(WarningType::none, NotificationType::synchronous);
                                             mAccessor.setAttr<AttrType::results>(Results{}, NotificationType::synchronous);
                                             mAccessor.setAttr<AttrType::file>(FileInfo{}, NotificationType::synchronous);
                                             MiscWeakAssert(perform != nullptr);
                                             if(perform != nullptr)
                                             {
                                                 perform();
                                             }
                                         }
                                         updateState();
                                     });
    }
    else
    {
        MiscWeakAssert(prepare != nullptr);
        if(prepare != nullptr && prepare(true))
        {
            MiscWeakAssert(perform != nullptr);
            if(perform != nullptr)
            {
                perform();
            }
        }
        updateState();
    }
}

void Track::PropertyProcessorSection::applyParameterValue(Plugin::Parameter const& parameter, float value)
{
    askToModifyProcessor([this](bool result)
                         {
                             if(result)
                             {
                                 mDirector.startAction();
                             }
                             return result;
                         },
                         [=, this]()
                         {
                             auto state = mAccessor.getAttr<AttrType::state>();
                             anlWeakAssert(value >= parameter.minValue && value <= parameter.maxValue);
                             state.parameters[parameter.identifier] = std::clamp(value, parameter.minValue, parameter.maxValue);
                             mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track property"));
                             updateState();
                         });
}

void Track::PropertyProcessorSection::setWindowType(Plugin::WindowType const& windowType)
{
    askToModifyProcessor([this](bool result)
                         {
                             if(result)
                             {
                                 mDirector.startAction();
                             }
                             return result;
                         },
                         [=, this]()
                         {
                             auto state = mAccessor.getAttr<AttrType::state>();
                             state.windowType = windowType;
                             mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track's window type"));
                         });
}

void Track::PropertyProcessorSection::setBlockSize(size_t const blockSize)
{
    auto const currentBlockSize = mAccessor.getAttr<AttrType::state>().blockSize;
    if(currentBlockSize == blockSize)
    {
        updateState();
        return;
    }
    askToModifyProcessor([this](bool result)
                         {
                             if(result)
                             {
                                 mDirector.startAction();
                             }
                             return result;
                         },
                         [=, this]()
                         {
                             auto state = mAccessor.getAttr<AttrType::state>();
                             auto const overlapping = static_cast<double>(state.blockSize) / static_cast<double>(state.stepSize);
                             state.blockSize = blockSize;
                             auto const defaultStep = mAccessor.getAttr<AttrType::description>().defaultState.stepSize;
                             if(defaultStep == 0_z)
                             {
                                 state.stepSize = state.blockSize;
                             }
                             else
                             {
                                 state.stepSize = static_cast<size_t>(std::round(static_cast<double>(state.blockSize) / overlapping));
                             }
                             mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track's block size"));
                         });
}

void Track::PropertyProcessorSection::setStepSize(size_t const stepSize)
{
    auto const currentStepSize = mAccessor.getAttr<AttrType::state>().stepSize;
    if(currentStepSize == stepSize)
    {
        updateState();
        return;
    }
    askToModifyProcessor([this](bool result)
                         {
                             if(result)
                             {
                                 mDirector.startAction();
                             }
                             return result;
                         },
                         [=, this]()
                         {
                             auto state = mAccessor.getAttr<AttrType::state>();
                             state.stepSize = stepSize;
                             mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Change track's step size"));
                         });
}

void Track::PropertyProcessorSection::restoreDefaultPreset()
{
    askToModifyProcessor([this](bool result)
                         {
                             if(result)
                             {
                                 mDirector.startAction();
                             }
                             return result;
                         },
                         [=, this]()
                         {
                             mAccessor.setAttr<AttrType::state>(mAccessor.getAttr<AttrType::description>().defaultState, NotificationType::synchronous);
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Restore track's default properties"));
                         });
}

void Track::PropertyProcessorSection::loadPreset()
{
    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Load preset from file..."), juce::File{}, App::getFileWildCardFor("preset"));
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    auto performChanges = [=, this](juce::File const& file)
    {
        auto const exportResult = Exporter::fromPreset(mAccessor, file);
        if(exportResult.failed())
        {
            mDirector.endAction(ActionState::abort);
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::WarningIcon)
                                     .withTitle(juce::translate("Failed to load from preset file!"))
                                     .withMessage(exportResult.getErrorMessage())
                                     .withButton(juce::translate("Ok"));
            juce::AlertWindow::showAsync(options, nullptr);
        }
        else
        {
            mDirector.endAction(ActionState::newTransaction, juce::translate("Change track's properties from preset file"));
        }
        updateState();
    };
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
                                      updateState();
                                      return;
                                  }
                                  askToModifyProcessor([this](bool result)
                                                       {
                                                           if(result)
                                                           {
                                                               mDirector.startAction();
                                                           }
                                                           return result;
                                                       },
                                                       [=, file = results.getFirst()]()
                                                       {
                                                           performChanges(file);
                                                       });
                              });
}

void Track::PropertyProcessorSection::savePreset()
{
    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Save as preset..."), juce::File{}, App::getFileWildCardFor("preset"));
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
                                      updateState();
                                      return;
                                  }
                                  auto const result = Exporter::toPreset(mAccessor, results.getFirst());
                                  if(result.failed())
                                  {
                                      auto const options = juce::MessageBoxOptions()
                                                               .withIconType(juce::AlertWindow::WarningIcon)
                                                               .withTitle(juce::translate("Failed to save a preset file!"))
                                                               .withMessage(result.getErrorMessage())
                                                               .withButton(juce::translate("Ok"));
                                      juce::AlertWindow::showAsync(options, nullptr);
                                  }
                                  updateState();
                              });
}

void Track::PropertyProcessorSection::changePreset(size_t index)
{
    askToModifyProcessor([=, this](bool result)
                         {
                             auto const& programs = mAccessor.getAttr<AttrType::description>().programs;
                             anlWeakAssert(index < programs.size());
                             auto const isValid = result && index < programs.size();
                             if(isValid)
                             {
                                 mDirector.startAction();
                             }
                             return isValid;
                         },
                         [=, this]()
                         {
                             auto const& programs = mAccessor.getAttr<AttrType::description>().programs;
                             anlWeakAssert(index < programs.size());
                             if(index >= programs.size())
                             {
                                 mDirector.endAction(ActionState::abort, juce::translate("Apply track preset properties"));
                                 return;
                             }

                             auto const it = std::next(programs.cbegin(), static_cast<long>(index));
                             mAccessor.setAttr<AttrType::state>(it->second, NotificationType::synchronous);
                             mDirector.endAction(ActionState::newTransaction, juce::translate("Apply track preset properties"));
                         });
}

void Track::PropertyProcessorSection::updateState()
{
    auto constexpr silent = juce::NotificationType::dontSendNotification;
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
                propertyNumber->entry.setValue(static_cast<double>(parameter.second), silent);
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

ANALYSE_FILE_END
