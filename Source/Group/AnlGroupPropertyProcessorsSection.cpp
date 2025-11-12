#include "AnlGroupPropertyProcessorsSection.h"
#include "../Plugin/AnlPluginTools.h"
#include "../Track/AnlTrackTools.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

static std::vector<std::string> getWindowTypeNames()
{
    std::vector<std::string> names;
    for(auto const& name : magic_enum::enum_names<Plugin::WindowType>())
    {
        names.push_back(std::string(name));
    }
    return names;
}

static std::vector<std::string> getBlockSizeNames()
{
    return {"8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096", "8192", "16384"};
}

Group::PropertyProcessorsSection::PropertyProcessorsSection(Director& director)
: mDirector(director)
, mPropertyWindowType(juce::translate("Window Type"), juce::translate("The window type of the FFT used by the tracks of the group."), "", getWindowTypeNames(), [&](size_t index)
                      {
                          setWindowType(static_cast<Plugin::WindowType>(index));
                      })
, mPropertyBlockSize(juce::translate("Block Size"), juce::translate("The block size used by the tracks of the group."), "samples", getBlockSizeNames(), [&](size_t index)
                     {
                         juce::ignoreUnused(index);
                         setBlockSize(static_cast<size_t>(mPropertyBlockSize.entry.getText().getIntValue()));
                     })
, mPropertyStepSize(juce::translate("Step Size"), juce::translate("The step size (overlapping) used by the tracks of the group."), "x", std::vector<std::string>{}, [&](size_t index)
                    {
                        juce::ignoreUnused(index);
                        setStepSize(static_cast<size_t>(mPropertyStepSize.entry.getText().getIntValue()));
                    })
, mPropertyInputTrack(juce::translate("Input Track"), juce::translate("The input track used by the tracks of the group."), "", std::vector<std::string>{}, [&]([[maybe_unused]] size_t index)
                      {
                          auto const listId = mPropertyInputTrack.entry.getSelectedId();
                          if(listId == 1)
                          {
                              setInputTrack({});
                          }
                          else
                          {
                              auto it = mPropertyInputTrackList.find(listId);
                              if(it != mPropertyInputTrackList.cend())
                              {
                                  setInputTrack(it->second);
                              }
                              else
                              {
                                  updateInputTrack();
                              }
                          }
                      })
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::name, Track::AttrType::key, Track::AttrType::description, Track::AttrType::file, Track::AttrType::results})
, mStateNotifier(mAccessor, [this]()
                 {
                     updateState();
                 },
                 {Track::AttrType::state})
{
    mPropertyBlockSize.entry.getProperties().set("isNumber", true);
    NumberField::Label::storeProperties(mPropertyBlockSize.entry.getProperties(), {1.0, 65536.0}, 1.0, 0, "samples");
    mPropertyStepSize.entry.getProperties().set("isNumber", true);
    NumberField::Label::storeProperties(mPropertyStepSize.entry.getProperties(), {1.0, 65536.0}, 1.0, 0, "samples");

    addAndMakeVisible(mPropertyWindowType);
    addAndMakeVisible(mPropertyBlockSize);
    addAndMakeVisible(mPropertyStepSize);
    addAndMakeVisible(mPropertyInputTrack);
}

void Group::PropertyProcessorsSection::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyInputTrack);
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
    setSize(getWidth(), bounds.getY());
}

void Group::PropertyProcessorsSection::askToModifyProcessors(std::function<bool(bool)> prepare, std::function<void(void)> perform, std::function<bool(Track::Accessor const&)> trackFilter)
{
    auto unlockTracks = [=, this]()
    {
        auto trackAcsrs = Tools::getTrackAcsrs(mAccessor);
        for(auto& trackAcsr : trackAcsrs)
        {
            if(trackFilter == nullptr || trackFilter(trackAcsr.get()))
            {
                trackAcsr.get().setAttr<Track::AttrType::warnings>(Track::WarningType::none, NotificationType::synchronous);
                trackAcsr.get().setAttr<Track::AttrType::results>(Track::Results{}, NotificationType::synchronous);
                trackAcsr.get().setAttr<Track::AttrType::file>(Track::FileInfo{}, NotificationType::synchronous);
            }
        }
    };

    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    auto const shouldBeUnlock = std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [=](auto const& trackAcsr)
                                            {
                                                auto const& file = trackAcsr.get().template getAttr<Track::AttrType::file>().file;
                                                auto const& key = trackAcsr.get().template getAttr<Track::AttrType::key>();
                                                return file != juce::File{} && !key.identifier.empty() && !key.feature.empty() && (trackFilter == nullptr || trackFilter(trackAcsr.get()));
                                            });
    if(shouldBeUnlock)
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Plugins' results are locked!"))
                                 .withMessage(juce::translate("The analysis results of one or more were consolidated or loaded from a file. Do you want to detach the files to modify the parameters and restart the analyses?"))
                                 .withButton(juce::translate("Detach the files"))
                                 .withButton(juce::translate("Cancel changes"));
        juce::WeakReference<juce::Component> weakReference(this);
        juce::AlertWindow::showAsync(options, [=](auto result)
                                     {
                                         if(weakReference.get() == nullptr)
                                         {
                                             return;
                                         }
                                         auto const hasChanged = result == 1;
                                         MiscWeakAssert(prepare != nullptr);
                                         if(prepare != nullptr && prepare(hasChanged))
                                         {
                                             unlockTracks();
                                             MiscWeakAssert(perform != nullptr);
                                             if(perform != nullptr)
                                             {
                                                 perform();
                                             }
                                         }
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
    }
}

void Group::PropertyProcessorsSection::applyParameterValue(Plugin::Parameter const& parameter, float value)
{
    askToModifyProcessors([this](bool result)
                          {
                              if(result)
                              {
                                  mDirector.startAction(true);
                              }
                              return result;
                          },
                          [=, this]()
                          {
                              auto trackAcsrs = Tools::getTrackAcsrs(mAccessor);
                              for(auto& trackAcsr : trackAcsrs)
                              {
                                  auto state = trackAcsr.get().getAttr<Track::AttrType::state>();
                                  auto it = state.parameters.find(parameter.identifier);
                                  if(it != state.parameters.end())
                                  {
                                      anlWeakAssert(value >= parameter.minValue && value <= parameter.maxValue);
                                      it->second = std::min(std::max(value, parameter.minValue), parameter.maxValue);
                                      trackAcsr.get().setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
                                  }
                              }
                              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group properties"));
                          },
                          [=](Track::Accessor const& trackAcsr)
                          {
                              return trackAcsr.getAttr<Track::AttrType::state>().parameters.count(parameter.identifier) > 0_z;
                          });
}

void Group::PropertyProcessorsSection::setWindowType(Plugin::WindowType const& windowType)
{
    askToModifyProcessors([this](bool result)
                          {
                              if(result)
                              {
                                  mDirector.startAction(true);
                              }
                              return result;
                          },
                          [=, this]()
                          {
                              auto trackAcsrs = Tools::getTrackAcsrs(mAccessor);
                              for(auto& trackAcsr : trackAcsrs)
                              {
                                  if(Track::Tools::supportsWindowType(trackAcsr.get()))
                                  {
                                      auto state = trackAcsr.get().getAttr<Track::AttrType::state>();
                                      state.windowType = windowType;
                                      trackAcsr.get().setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
                                  }
                              }
                              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's window type"));
                          },
                          [=](Track::Accessor const& trackAcsr)
                          {
                              return Track::Tools::supportsWindowType(trackAcsr);
                          });
}

void Group::PropertyProcessorsSection::setBlockSize(size_t const blockSize)
{
    askToModifyProcessors([this](bool result)
                          {
                              if(result)
                              {
                                  mDirector.startAction(true);
                              }
                              else
                              {
                                  updateBlockSize();
                              }
                              return result;
                          },
                          [=, this]()
                          {
                              auto trackAcsrs = Tools::getTrackAcsrs(mAccessor);
                              for(auto& trackAcsr : trackAcsrs)
                              {
                                  if(Track::Tools::supportsBlockSize(trackAcsr.get()))
                                  {
                                      auto state = trackAcsr.get().getAttr<Track::AttrType::state>();
                                      auto const overlapping = static_cast<double>(state.blockSize) / static_cast<double>(state.stepSize);
                                      state.blockSize = blockSize;
                                      auto const defaultStep = trackAcsr.get().getAttr<Track::AttrType::description>().defaultState.stepSize;
                                      if(defaultStep == 0_z)
                                      {
                                          state.stepSize = state.blockSize;
                                      }
                                      else
                                      {
                                          state.stepSize = static_cast<size_t>(std::round(static_cast<double>(state.blockSize) / overlapping));
                                      }
                                      trackAcsr.get().setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
                                  }
                              }
                              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's block size"));
                              updateBlockSize();
                          },
                          [=](Track::Accessor const& trackAcsr)
                          {
                              return Track::Tools::supportsBlockSize(trackAcsr);
                          });
}

void Group::PropertyProcessorsSection::setStepSize(size_t const stepSize)
{
    askToModifyProcessors([this](bool result)
                          {
                              if(result)
                              {
                                  mDirector.startAction(true);
                              }
                              else
                              {
                                  updateStepSize();
                              }
                              return result;
                          },
                          [=, this]()
                          {
                              auto trackAcsrs = Tools::getTrackAcsrs(mAccessor);
                              for(auto& trackAcsr : trackAcsrs)
                              {
                                  if(Track::Tools::supportsStepSize(trackAcsr.get()))
                                  {
                                      auto state = trackAcsr.get().getAttr<Track::AttrType::state>();
                                      state.stepSize = stepSize;
                                      trackAcsr.get().setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
                                  }
                              }
                              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's step size"));
                              updateStepSize();
                          },
                          [=](Track::Accessor const& trackAcsr)
                          {
                              return Track::Tools::supportsStepSize(trackAcsr);
                          });
}

void Group::PropertyProcessorsSection::setInputTrack(juce::String const& identifier)
{
    auto const supportsInputTrack = [=, this](Track::Accessor const& acsr)
    {
        if(!Track::Tools::supportsInputTrack(acsr))
        {
            return false;
        }
        auto const& hierarchyManager = mDirector.getHierarchyManager();
        auto const& input = acsr.getAttr<Track::AttrType::description>().input;
        auto const frameType = Track::Tools::getFrameType(input);
        if(!frameType.has_value())
        {
            return false;
        }
        return identifier.isEmpty() || hierarchyManager.isTrackValidFor(acsr.getAttr<Track::AttrType::identifier>(), frameType.value(), identifier);
    };

    askToModifyProcessors([this](bool result)
                          {
                              if(result)
                              {
                                  mDirector.startAction(true);
                              }
                              else
                              {
                                  updateInputTrack();
                              }
                              return result;
                          },
                          [=, this]()
                          {
                              auto trackAcsrs = Tools::getTrackAcsrs(mAccessor);
                              for(auto& trackAcsr : trackAcsrs)
                              {
                                  if(supportsInputTrack(trackAcsr.get()))
                                  {
                                      trackAcsr.get().setAttr<Track::AttrType::input>(identifier, NotificationType::synchronous);
                                  }
                              }
                              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's input track"));
                              updateInputTrack();
                          },
                          supportsInputTrack);
}

void Group::PropertyProcessorsSection::updateContent()
{
    updateWindowType();
    updateBlockSize();
    updateStepSize();
    updateInputTrack();
    updateParameters();
    updateState();
    resized();
}

void Group::PropertyProcessorsSection::updateWindowType()
{
    juce::StringArray trackNames;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    std::set<Plugin::WindowType> windowTypes;
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::supportsWindowType(trackAcsr.get()))
        {
            auto const windowType = trackAcsr.get().getAttr<Track::AttrType::state>().windowType;
            windowTypes.insert(windowType);
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyWindowType.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The window type of the FFT used by the tracks of the group."));
    mPropertyWindowType.setVisible(!windowTypes.empty());
    if(windowTypes.size() == 1_z)
    {
        mPropertyWindowType.entry.setSelectedId(static_cast<int>(*windowTypes.cbegin()) + 1, juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyWindowType.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
    auto const hasPlugin = std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                       {
                                           return Track::Tools::supportsWindowType(trackAcsr.get()) && Track::Tools::hasPluginKey(trackAcsr.get());
                                       });
    mPropertyWindowType.setEnabled(hasPlugin);
}

void Group::PropertyProcessorsSection::updateBlockSize()
{
    juce::StringArray trackNames;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    std::set<size_t> blockSizes;
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::supportsBlockSize(trackAcsr.get()))
        {
            auto const blockSize = trackAcsr.get().getAttr<Track::AttrType::state>().blockSize;
            blockSizes.insert(blockSize);
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyBlockSize.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The block size used by the tracks of the group."));
    mPropertyBlockSize.setVisible(!blockSizes.empty());
    if(blockSizes.size() == 1_z)
    {
        mPropertyBlockSize.entry.setText(juce::String(*blockSizes.cbegin()) + "samples", juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyBlockSize.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
    auto const hasPlugin = std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                       {
                                           return Track::Tools::supportsBlockSize(trackAcsr.get()) && Track::Tools::hasPluginKey(trackAcsr.get());
                                       });
    mPropertyBlockSize.setEnabled(hasPlugin);
    auto const isTimeDomainOnly = std::all_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                              {
                                                  return !Track::Tools::supportsBlockSize(trackAcsr.get()) || trackAcsr.get().template getAttr<Track::AttrType::description>().inputDomain == Plugin::InputDomain::TimeDomain;
                                              });
    mPropertyBlockSize.entry.setEditableText(isTimeDomainOnly);
}

void Group::PropertyProcessorsSection::updateStepSize()
{
    juce::StringArray trackNames;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    std::set<size_t> stepSizes;
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::supportsStepSize(trackAcsr.get()))
        {
            auto const stepSize = trackAcsr.get().getAttr<Track::AttrType::state>().stepSize;
            stepSizes.insert(stepSize);
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    auto const blockSize = std::accumulate(trackAcsrs.cbegin(), trackAcsrs.cend(), 16384_z, [](auto blkSize, auto const& trackAcsr)
                                           {
                                               if(Track::Tools::supportsStepSize(trackAcsr.get()))
                                               {
                                                   return std::min(blkSize, trackAcsr.get().template getAttr<Track::AttrType::state>().blockSize);
                                               }
                                               return blkSize;
                                           });
    mPropertyStepSize.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The step size used by the tracks of the group."));
    mPropertyStepSize.setVisible(!stepSizes.empty());
    if(stepSizes.size() == 1_z)
    {
        mPropertyStepSize.entry.clear(juce::NotificationType::dontSendNotification);
        for(int i = 1; static_cast<size_t>(i) <= blockSize; i *= 2)
        {
            mPropertyStepSize.entry.addItem(juce::String(i) + "samples", i);
        }
        mPropertyBlockSize.entry.setText(juce::String(*stepSizes.cbegin()) + "samples", juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyStepSize.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
    auto const hasPlugin = std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                       {
                                           return Track::Tools::supportsStepSize(trackAcsr.get()) && Track::Tools::hasPluginKey(trackAcsr.get());
                                       });
    mPropertyStepSize.setEnabled(hasPlugin);
    auto const isTimeDomainOnly = std::all_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                              {
                                                  return Track::Tools::supportsBlockSize(trackAcsr.get()) || trackAcsr.get().template getAttr<Track::AttrType::description>().inputDomain == Plugin::InputDomain::TimeDomain;
                                              });
    mPropertyBlockSize.entry.setEditableText(isTimeDomainOnly);
}

void Group::PropertyProcessorsSection::updateInputTrack()
{
    juce::StringArray trackNames;
    std::set<juce::String> inputs;
    std::set<juce::String> identifiers;
    std::map<juce::String, Track::HierarchyManager::TrackInfo> otherTracks;

    auto const& hierarchyManager = mDirector.getHierarchyManager();
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::supportsInputTrack(trackAcsr.get()))
        {
            auto const identifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
            identifiers.insert(identifier);
            inputs.insert(trackAcsr.get().getAttr<Track::AttrType::input>());
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
            auto const& input = trackAcsr.get().getAttr<Track::AttrType::description>().input;
            auto const frameType = Track::Tools::getFrameType(input);
            if(frameType.has_value())
            {
                auto const inputTracks = hierarchyManager.getAvailableTracksFor(identifier, frameType.value());
                for(auto const& inputTrack : inputTracks)
                {
                    otherTracks[inputTrack.identifier] = inputTrack;
                }
            }
        }
    }
    mPropertyInputTrack.setVisible(!inputs.empty());
    if(inputs.empty())
    {
        return;
    }

    mPropertyInputTrack.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The input tracks used by the tracks of the group."));
    mPropertyInputTrack.entry.clear(juce::NotificationType::dontSendNotification);
    mPropertyInputTrackList.clear();
    auto listId = 1;
    mPropertyInputTrack.entry.setEnabled(!otherTracks.empty());
    mPropertyInputTrack.entry.addItem(juce::translate("Undefined"), listId++);
    for(auto trackIt = otherTracks.crbegin(); trackIt != otherTracks.crend(); ++trackIt)
    {
        if(!trackIt->second.group.isEmpty() && !trackIt->second.group.isEmpty())
        {
            mPropertyInputTrackList[listId] = trackIt->second.identifier;
            mPropertyInputTrack.entry.addItem(trackIt->second.group + ": " + trackIt->second.name, listId++);
        }
    }

    if(inputs.size() == 1_z)
    {
        mPropertyInputTrack.entry.setTextWhenNothingSelected(juce::translate("Not found"));
        if(inputs.cbegin()->isEmpty())
        {
            mPropertyInputTrack.entry.setSelectedId(1, juce::NotificationType::dontSendNotification);
        }
        else
        {
            auto const it = std::find_if(mPropertyInputTrackList.cbegin(), mPropertyInputTrackList.cend(), [&](auto const& pait)
                                         {
                                             return pait.second == *inputs.cbegin();
                                         });
            if(it != mPropertyInputTrackList.cend())
            {
                mPropertyInputTrack.entry.setSelectedId(it->first, juce::NotificationType::dontSendNotification);
            }
        }
    }
    else
    {
        mPropertyInputTrack.entry.setTextWhenNothingSelected(juce::translate("Multiple Values"));
        mPropertyStepSize.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
}

void Group::PropertyProcessorsSection::updateParameters()
{
    juce::WeakReference<juce::Component> weakReference(this);
    auto changeValue = [=, this](auto const& parameter, auto value)
    {
        if(weakReference.get() == nullptr)
        {
            return;
        }
        applyParameterValue(parameter, value);
    };

    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    auto const hasPlugin = std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                       {
                                           return Track::Tools::hasPluginKey(trackAcsr.get());
                                       });
    mPropertyStepSize.setEnabled(hasPlugin);

    mParameterProperties.clear();
    std::set<size_t> stepSizes;
    for(auto const& trackAcsr : trackAcsrs)
    {
        auto const& description = trackAcsr.get().getAttr<Track::AttrType::description>();
        for(auto const& parameter : description.parameters)
        {
            if(mParameterProperties.count(parameter.identifier) == 0_z)
            {
                auto property = Plugin::Tools::createProperty(parameter, changeValue);
                anlWeakAssert(property != nullptr);
                if(property != nullptr)
                {
                    addAndMakeVisible(property.get());
                    property->setEnabled(hasPlugin);
                    mParameterProperties[parameter.identifier] = std::move(property);
                }
            }
        }
    }

    for(auto& parameter : mParameterProperties)
    {
        if(auto* tooltipClient = dynamic_cast<juce::SettableTooltipClient*>(parameter.second.get()))
        {
            juce::StringArray trackNames;
            for(auto& trackAcsr : trackAcsrs)
            {
                if(trackAcsr.get().getAttr<Track::AttrType::state>().parameters.count(parameter.first))
                {
                    trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
                }
            }
            tooltipClient->setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + tooltipClient->getTooltip());
        }
    }
}

void Group::PropertyProcessorsSection::updateState()
{
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    for(auto const& parameter : mParameterProperties)
    {
        std::set<float> values;
        for(auto& trackAcsr : trackAcsrs)
        {
            auto const& state = trackAcsr.get().getAttr<Track::AttrType::state>();
            auto const it = state.parameters.find(parameter.first);
            if(it != state.parameters.cend())
            {
                values.insert(it->second);
            }
        }
        if(auto* propertyList = dynamic_cast<PropertyList*>(parameter.second.get()))
        {
            if(values.empty())
            {
                propertyList->setEnabled(!values.empty());
                propertyList->entry.setText(juce::translate("No Value"), juce::NotificationType::dontSendNotification);
            }
            else if(values.size() == 1_z)
            {
                propertyList->entry.setSelectedItemIndex(static_cast<int>(std::floor(*values.cbegin())), juce::NotificationType::dontSendNotification);
            }
            else
            {
                propertyList->entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
            }
        }
        else if(auto* propertyNumber = dynamic_cast<PropertyNumber*>(parameter.second.get()))
        {
            if(values.empty())
            {
                propertyNumber->setEnabled(!values.empty());
                propertyNumber->entry.setText(juce::translate("No Value"), juce::NotificationType::dontSendNotification);
            }
            if(values.size() == 1_z)
            {
                propertyNumber->entry.setValue(static_cast<double>(*values.cbegin()), juce::NotificationType::dontSendNotification);
            }
            else
            {
                propertyNumber->entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
            }
        }
        else if(auto* propertyToggle = dynamic_cast<PropertyToggle*>(parameter.second.get()))
        {
            if(values.empty())
            {
                propertyToggle->entry.getProperties().remove("Multiple Values");
                propertyToggle->setEnabled(!values.empty());
            }
            else if(values.size() == 1_z)
            {
                propertyToggle->entry.getProperties().remove("Multiple Values");
                propertyToggle->entry.setToggleState(*values.cbegin() > 0.5f, juce::NotificationType::dontSendNotification);
            }
            else
            {
                propertyToggle->entry.getProperties().set("Multiple Values", {true});
                propertyToggle->entry.setToggleState(false, juce::NotificationType::dontSendNotification);
            }
        }
        else
        {
            anlWeakAssert(false && "property unsupported");
        }
    }
}

ANALYSE_FILE_END
