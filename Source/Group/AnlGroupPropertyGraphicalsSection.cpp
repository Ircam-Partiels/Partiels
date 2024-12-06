#include "AnlGroupPropertyGraphicalsSection.h"
#include "../Track/AnlTrackTools.h"
#include "AnlGroupTools.h"

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

Group::PropertyGraphicalsSection::PropertyGraphicalsSection(Director& director)
: mDirector(director)
, mPropertyColourMap(juce::translate("Color Map"), juce::translate("The color map used by the graphical renderers of the tracks of the group."), "", getColourMapNames(), [&](size_t index)
                     {
                         mDirector.startAction(true);
                         setColourMap(static_cast<Track::ColourMap>(index));
                         mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's color map"));
                     })
, mPropertyForegroundColour(juce::translate("Foreground Color"), juce::translate("The foreground color used by the graphical renderers of the tracks of the group."), juce::translate("Select the foreground color"), [&](juce::Colour const& colour)
                            {
                                if(!mPropertyForegroundColour.entry.isColourSelectorVisible())
                                {
                                    mDirector.startAction(true);
                                }
                                setForegroundColour(colour);
                                if(!mPropertyForegroundColour.entry.isColourSelectorVisible())
                                {
                                    mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's foreground color"));
                                }
                            },
                            [&]()
                            {
                                mDirector.startAction(true);
                            },
                            [&]()
                            {
                                mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's foreground color"));
                            })
, mPropertyDurationColour(juce::translate("Duration Color"), juce::translate("The duration color used by the graphical renderers of the tracks of the group."), juce::translate("Select the duration color"), [&](juce::Colour const& colour)
                          {
                              if(!mPropertyDurationColour.entry.isColourSelectorVisible())
                              {
                                  mDirector.startAction(true);
                              }
                              setDurationColour(colour);
                              if(!mPropertyDurationColour.entry.isColourSelectorVisible())
                              {
                                  mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's duration color"));
                              }
                          },
                          [&]()
                          {
                              mDirector.startAction(true);
                          },
                          [&]()
                          {
                              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's duration color"));
                          })
, mPropertyBackgroundColour(juce::translate("Background Color"), juce::translate("The background color used by the graphical renderers of the tracks of the group."), juce::translate("Select the background color"), [&](juce::Colour const& colour)
                            {
                                if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
                                {
                                    mDirector.startAction(true);
                                }
                                setBackgroundColour(colour);
                                if(!mPropertyBackgroundColour.entry.isColourSelectorVisible())
                                {
                                    mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's background color"));
                                }
                            },
                            [&]()
                            {
                                mDirector.startAction(true);
                            },
                            [&]()
                            {
                                mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's background color"));
                            })
, mPropertyTextColour(juce::translate("Text Color"), juce::translate("The text color used by the graphical renderers of the tracks of the group."), juce::translate("Select the text color"), [&](juce::Colour const& colour)
                      {
                          if(!mPropertyTextColour.entry.isColourSelectorVisible())
                          {
                              mDirector.startAction(true);
                          }
                          setTextColour(colour);
                          if(!mPropertyTextColour.entry.isColourSelectorVisible())
                          {
                              mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's text color"));
                          }
                      },
                      [&]()
                      {
                          mDirector.startAction(true);
                      },
                      [&]()
                      {
                          mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's text color"));
                      })
, mPropertyShadowColour(juce::translate("Shadow Color"), juce::translate("The shadow color used by the graphical renderers of the tracks of the group."), juce::translate("Select the shadow color"), [&](juce::Colour const& colour)
                        {
                            if(!mPropertyShadowColour.entry.isColourSelectorVisible())
                            {
                                mDirector.startAction(true);
                            }
                            setShadowColour(colour);
                            if(!mPropertyShadowColour.entry.isColourSelectorVisible())
                            {
                                mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's shadow color"));
                            }
                        },
                        [&]()
                        {
                            mDirector.startAction(true);
                        },
                        [&]()
                        {
                            mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's shadow color"));
                        })
, mPropertyFontName(juce::translate("Font Name"), juce::translate("The name of the font for the graphical renderers of the tracks of the group."), "", getFontNames(), [&]([[maybe_unused]] size_t index)
                    {
                        mDirector.startAction(true);
                        setFontName(mPropertyFontName.entry.getText());
                        mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's font name"));
                    })
, mPropertyFontStyle(juce::translate("Font Style"), juce::translate("The style of the font for the graphical renderers of the tracks of the group."), "", {}, [&]([[maybe_unused]] size_t index)
                     {
                         mDirector.startAction(true);
                         setFontStyle(mPropertyFontStyle.entry.getText());
                         mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's font style"));
                     })
, mPropertyFontSize(juce::translate("Font Size"), juce::translate("The size of the font for the graphical renderers of the tracks of the group."), "", getFontSizes(), [&]([[maybe_unused]] size_t index)
                    {
                        mDirector.startAction(true);
                        setFontSize(mPropertyFontSize.entry.getText().getFloatValue());
                        mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's font size"));
                    })
, mPropertyLineWidth(juce::translate("Line Width"), juce::translate("The line width for the graphical renderers of the tracks of the group."), "", {1.0f, 100.0f}, 0.5f, [&](float value)
                     {
                         mDirector.startAction(true);
                         setLineWidth(value);
                         mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's line width"));
                     })
, mPropertyUnit(juce::translate("Unit"), juce::translate("The unit of the values for the graphical renderers of the tracks of the group."), [&](juce::String text)
                {
                    setUnit(text);
                })
, mPropertyLabelJustification(juce::translate("Label Justification"), juce::translate("The justification of the labels for the graphical renderers of the tracks of the group."), "", std::vector<std::string>{"Top", "Centred", "Bottom"}, [&](size_t index)
                              {
                                  mDirector.startAction(true);
                                  setLabelJustification(magic_enum::enum_cast<Track::LabelLayout::Justification>(static_cast<int>(index)).value_or(Track::LabelLayout::Justification::top));
                                  mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's justification of the labels"));
                              })
, mPropertyLabelPosition(juce::translate("Label Position"), juce::translate("The position of the labels."), "", {-120.0f, 120.0f}, 0.1f, [this](float position)
                         {
                             mDirector.startAction(true);
                             setLabelPosition(position);
                             mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's position of the labels"));
                         })
, mPropertyValueRangeLogScale(juce::translate("Value Log. Scale"), juce::translate("Toggle the logarithmic scale of the zoom range of the tracks of the group."), [&](bool value)
                              {
                                  mDirector.startAction(true);
                                  setLogScale(value);
                                  mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group log scale"));
                              })
, mPropertyChannelLayout(juce::translate("Channel Layout"), juce::translate("The visibility of the channels of the tracks of the group."), [this]()
                         {
                             showChannelLayout();
                         })
, mPropertyShowInGroup(juce::translate("Track Layout"), juce::translate("The visibility of the tracks in the group overlay view."), [this]()
                       {
                           showVisibilityInGroup();
                       })
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::name, Track::AttrType::colours, Track::AttrType::font, Track::AttrType::lineWidth, Track::AttrType::unit, Track::AttrType::description, Track::AttrType::channelsLayout, Track::AttrType::showInGroup, Track::AttrType::results, Track::AttrType::hasPluginColourMap, Track::AttrType::zoomLogScale})
{
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
    addAndMakeVisible(mPropertyUnit);
    addAndMakeVisible(mPropertyValueRangeLogScale);
    addAndMakeVisible(mPropertyShowInGroup);
    addAndMakeVisible(mPropertyChannelLayout);
}

void Group::PropertyGraphicalsSection::resized()
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
    setBounds(mPropertyValueRangeLogScale);
    setBounds(mPropertyShowInGroup);
    setBounds(mPropertyChannelLayout);
    setSize(getWidth(), bounds.getY());
}

void Group::PropertyGraphicalsSection::setColourMap(Track::ColourMap const& colourMap)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector || trackAcsr.get().template getAttr<Track::AttrType::hasPluginColourMap>();
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.map = colourMap;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setForegroundColour(juce::Colour const& colour)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.foreground = colour;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setDurationColour(juce::Colour const& colour)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::label;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.duration = colour;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setBackgroundColour(juce::Colour const& colour)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.background = colour;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setTextColour(juce::Colour const& colour)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.text = colour;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setShadowColour(juce::Colour const& colour)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
        colours.shadow = colour;
        trackAcsr.get().setAttr<Track::AttrType::colours>(colours, NotificationType::synchronous);
    }
}

void Group::PropertyGraphicalsSection::setFontName(juce::String const& name)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }

    auto const font = trackAcsrs.front().get().getAttr<Track::AttrType::font>();
    auto newFont = juce::Font(juce::FontOptions(name, font.getHeight(), juce::Font::FontStyleFlags::plain));
    if(newFont.getAvailableStyles().contains(font.getTypefaceStyle()))
    {
        newFont.setTypefaceStyle(font.getTypefaceStyle());
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        trackAcsr.get().setAttr<Track::AttrType::font>(font, NotificationType::synchronous);
    }
    updateFont();
}

void Group::PropertyGraphicalsSection::setFontStyle(juce::String const& style)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }

    for(auto& trackAcsr : trackAcsrs)
    {
        auto const font = trackAcsr.get().getAttr<Track::AttrType::font>();
        if(font.getAvailableStyles().contains(style))
        {
            trackAcsr.get().setAttr<Track::AttrType::font>(font.withTypefaceStyle(style), NotificationType::synchronous);
        }
    }
    updateFont();
}

void Group::PropertyGraphicalsSection::setFontSize(float size)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }

    for(auto& trackAcsr : trackAcsrs)
    {
        auto const font = trackAcsr.get().getAttr<Track::AttrType::font>().withHeight(size);
        trackAcsr.get().setAttr<Track::AttrType::font>(font, NotificationType::synchronous);
    }
    updateFont();
}

void Group::PropertyGraphicalsSection::setLineWidth(float size)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }

    for(auto& trackAcsr : trackAcsrs)
    {
        trackAcsr.get().setAttr<Track::AttrType::lineWidth>(size, NotificationType::synchronous);
    }
    updateLineWidth();
}

void Group::PropertyGraphicalsSection::setUnit(juce::String const& unit)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::label;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }

    auto const hasPluginUnit = std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                           {
                                               return !trackAcsr.get().template getAttr<Track::AttrType::description>().output.unit.empty();
                                           });

    if(unit.isEmpty() && hasPluginUnit)
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::QuestionIcon)
                                 .withTitle("Would you like to reset the unit to default?")
                                 .withMessage("The specified unit is empty. Would you like remove the unit or to reset the unit to default using the one provided by the plugin(s)?")
                                 .withButton(juce::translate("Remove the unit"))
                                 .withButton(juce::translate("Reset the unit to default"));
        juce::WeakReference<juce::Component> weakReference(this);
        juce::AlertWindow::showAsync(options, [=, this](int result)
                                     {
                                         mDirector.startAction(true);
                                         if(result == 1)
                                         {
                                             for(auto& trackAcsr : trackAcsrs)
                                             {
                                                 trackAcsr.get().setAttr<Track::AttrType::unit>(juce::String(), NotificationType::synchronous);
                                             }
                                         }
                                         else
                                         {
                                             for(auto& trackAcsr : trackAcsrs)
                                             {
                                                 trackAcsr.get().setAttr<Track::AttrType::unit>(std::optional<juce::String>{}, NotificationType::synchronous);
                                             }
                                         }
                                         updateUnit();
                                         mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change unit of the values name"));
                                     });
    }
    else
    {
        mDirector.startAction(true);
        for(auto& trackAcsr : trackAcsrs)
        {
            trackAcsr.get().setAttr<Track::AttrType::unit>(unit, NotificationType::synchronous);
        }
        updateUnit();
        mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change unit of the values name"));
    }
}

void Group::PropertyGraphicalsSection::setLabelJustification(Track::LabelLayout::Justification justification)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::label;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }

    for(auto& trackAcsr : trackAcsrs)
    {
        auto labelLayout = trackAcsr.get().getAttr<Track::AttrType::labelLayout>();
        labelLayout.justification = justification;
        trackAcsr.get().setAttr<Track::AttrType::labelLayout>(labelLayout, NotificationType::synchronous);
    }
    updateLabel();
}

void Group::PropertyGraphicalsSection::setLabelPosition(float position)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::label;
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }

    for(auto& trackAcsr : trackAcsrs)
    {
        auto labelLayout = trackAcsr.get().getAttr<Track::AttrType::labelLayout>();
        labelLayout.position = position;
        trackAcsr.get().setAttr<Track::AttrType::labelLayout>(labelLayout, NotificationType::synchronous);
    }
    updateLabel();
}

void Group::PropertyGraphicalsSection::setLogScale(bool state)
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return !Track::Tools::hasVerticalZoomInHertz(trackAcsr.get());
                                                });
    if(trackAcsrs.empty())
    {
        return;
    }

    for(auto& trackAcsr : trackAcsrs)
    {
        trackAcsr.get().setAttr<Track::AttrType::zoomLogScale>(state, NotificationType::synchronous);
    }
    updateLogScale();
}

void Group::PropertyGraphicalsSection::showChannelLayout()
{
    juce::PopupMenu menu;
    Tools::fillMenuForChannelVisibility(mAccessor, menu, nullptr, [this]()
                                        {
                                            showChannelLayout();
                                        });
    if(!std::exchange(mChannelLayoutActionStarted, true))
    {
        mDirector.startAction(true);
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertyChannelLayout.entry).withDeletionCheck(*this), [=, this](int menuResult)
                       {
                           if(menuResult == 0 && std::exchange(mChannelLayoutActionStarted, false))
                           {
                               mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change The visibility of the channels of the group"));
                           }
                       });
}

void Group::PropertyGraphicalsSection::showVisibilityInGroup()
{
    juce::PopupMenu menu;
    Tools::fillMenuForTrackVisibility(mAccessor, menu, nullptr, [this]()
                                      {
                                          showVisibilityInGroup();
                                      });
    if(!std::exchange(mShowInGroupActionStarted, true))
    {
        mDirector.startAction(true);
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertyShowInGroup.entry).withDeletionCheck(*this), [=, this](int menuResult)
                       {
                           if(menuResult == 0 && std::exchange(mShowInGroupActionStarted, false))
                           {
                               mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change the visibility of the tracks of the group"));
                           }
                       });
}

void Group::PropertyGraphicalsSection::updateContent()
{
    updateColourMap();
    updateColours();
    updateFont();
    updateLineWidth();
    updateUnit();
    updateLabel();
    updateLogScale();
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    auto const numChannels = std::accumulate(trackAcsrs.cbegin(), trackAcsrs.cend(), 0_z, [](auto n, auto const& trackAcsr)
                                             {
                                                 return std::max(n, trackAcsr.get().template getAttr<Track::AttrType::channelsLayout>().size());
                                             });
    mPropertyChannelLayout.setVisible(numChannels > 1_z);
    resized();
}

void Group::PropertyGraphicalsSection::updateColourMap()
{
    juce::StringArray trackNames;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    std::set<Track::ColourMap> colourMaps;
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector && !trackAcsr.get().getAttr<Track::AttrType::hasPluginColourMap>())
        {
            auto const colourMap = trackAcsr.get().getAttr<Track::AttrType::colours>().map;
            colourMaps.insert(colourMap);
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyColourMap.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The color map used by the graphical renderers of the tracks of the group."));
    mPropertyColourMap.setVisible(!colourMaps.empty());
    if(colourMaps.size() == 1_z)
    {
        mPropertyColourMap.entry.setSelectedId(static_cast<int>(*colourMaps.cbegin()) + 1, juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyColourMap.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
}

void Group::PropertyGraphicalsSection::updateColours()
{
    juce::StringArray trackNames;
    juce::StringArray labelTrackNames;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    std::set<juce::uint32> foregroundColours;
    std::set<juce::uint32> durationColours;
    std::set<juce::uint32> backgrounColours;
    std::set<juce::uint32> textColours;
    std::set<juce::uint32> shadowColours;
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
        {
            auto const& colours = trackAcsr.get().getAttr<Track::AttrType::colours>();
            foregroundColours.insert(colours.foreground.getARGB());
            backgrounColours.insert(colours.background.getARGB());
            textColours.insert(colours.text.getARGB());
            shadowColours.insert(colours.shadow.getARGB());
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
            if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
            {
                durationColours.insert(colours.duration.getARGB());
                labelTrackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
            }
        }
    }
    mPropertyForegroundColour.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The foreground color used by the graphical renderers of the tracks of the group."));
    mPropertyForegroundColour.setVisible(!foregroundColours.empty());
    if(foregroundColours.size() == 1_z)
    {
        mPropertyForegroundColour.entry.setCurrentColour(juce::Colour(*foregroundColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyForegroundColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyDurationColour.setTooltip("Track(s): " + labelTrackNames.joinIntoString(", ") + " - " + juce::translate("The duration color used by the graphical renderers of the tracks of the group."));
    mPropertyDurationColour.setVisible(!durationColours.empty());
    if(durationColours.size() == 1_z)
    {
        mPropertyDurationColour.entry.setCurrentColour(juce::Colour(*durationColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyDurationColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyBackgroundColour.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The background color used by the graphical renderers of the tracks of the group."));
    mPropertyBackgroundColour.setVisible(!backgrounColours.empty());
    if(backgrounColours.size() == 1_z)
    {
        mPropertyBackgroundColour.entry.setCurrentColour(juce::Colour(*backgrounColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyBackgroundColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyTextColour.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The text color used by the graphical renderers of the tracks of the group."));
    mPropertyTextColour.setVisible(!textColours.empty());
    if(textColours.size() == 1_z)
    {
        mPropertyTextColour.entry.setCurrentColour(juce::Colour(*textColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyTextColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyShadowColour.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The shadow color used by the graphical renderers of the tracks of the group."));
    mPropertyShadowColour.setVisible(!shadowColours.empty());
    if(shadowColours.size() == 1_z)
    {
        mPropertyShadowColour.entry.setCurrentColour(juce::Colour(*shadowColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyShadowColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }
}

void Group::PropertyGraphicalsSection::updateFont()
{
    juce::StringArray trackNames;
    std::set<juce::String> fontNames;
    std::set<juce::String> fontStyles;
    std::set<float> fontSizes;
    std::optional<juce::Font> currentFont;
    for(auto const& trackAcsr : Tools::getTrackAcsrs(mAccessor))
    {
        if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
        {
            auto const font = trackAcsr.get().getAttr<Track::AttrType::font>();
            if(!currentFont.has_value())
            {
                currentFont = font;
            }
            fontNames.insert(font.getTypefaceName());
            fontStyles.insert(font.getTypefaceStyle());
            fontSizes.insert(font.getHeight());
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }

    mPropertyFontName.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The font name used by the graphical renderers of the tracks of the group."));
    mPropertyFontName.setVisible(currentFont.has_value());
    mPropertyFontName.entry.setEnabled(mPropertyFontName.entry.getNumItems() > 1);
    if(fontNames.size() == 1_z && currentFont.has_value())
    {
        mPropertyFontName.entry.setText(currentFont.value().getTypefaceName(), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyFontName.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }

    mPropertyFontStyle.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The font style used by the graphical renderers of the tracks of the group."));
    mPropertyFontStyle.setVisible(currentFont.has_value());
    mPropertyFontStyle.entry.clear(juce::NotificationType::dontSendNotification);
    for(auto name : fontNames)
    {
        mPropertyFontStyle.entry.addItemList(juce::Font::findAllTypefaceStyles(name), mPropertyFontStyle.entry.getNumItems() + 1);
    }
    mPropertyFontStyle.entry.setEnabled(mPropertyFontStyle.entry.getNumItems() > 1);
    if(fontStyles.size() == 1_z && currentFont.has_value())
    {
        mPropertyFontStyle.entry.setText(currentFont.value().getTypefaceStyle(), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyFontStyle.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }

    mPropertyFontSize.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The font size used by the graphical renderers of the tracks of the group."));
    mPropertyFontSize.setVisible(currentFont.has_value());
    if(fontSizes.size() == 1_z && currentFont.has_value())
    {
        auto const size = currentFont.value().getHeight();
        auto const roundedSize = std::round(size);
        if(std::abs(size - roundedSize) < 0.1f)
        {
            mPropertyFontSize.entry.setText(juce::String(static_cast<int>(roundedSize)), juce::NotificationType::dontSendNotification);
        }
        else
        {
            mPropertyFontSize.entry.setText(juce::String(size, 1), juce::NotificationType::dontSendNotification);
        }
    }
    else
    {
        mPropertyFontSize.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
    resized();
}

void Group::PropertyGraphicalsSection::updateLineWidth()
{
    juce::StringArray trackNames;
    std::set<float> lineWidth;
    for(auto const& trackAcsr : Tools::getTrackAcsrs(mAccessor))
    {
        if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
        {
            lineWidth.insert(trackAcsr.get().getAttr<Track::AttrType::lineWidth>());
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }

    mPropertyLineWidth.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The line width used by the graphical renderers of the tracks of the group."));
    mPropertyLineWidth.setVisible(!lineWidth.empty());
    if(lineWidth.size() == 1_z)
    {
        mPropertyLineWidth.entry.setValue(static_cast<double>(*lineWidth.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else if(lineWidth.size() > 1_z)
    {
        mPropertyLineWidth.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
    resized();
}

void Group::PropertyGraphicalsSection::updateUnit()
{
    juce::StringArray trackNames;
    std::set<juce::String> units;
    for(auto const& trackAcsr : Tools::getTrackAcsrs(mAccessor))
    {
        if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::label)
        {
            units.insert(Track::Tools::getUnit(trackAcsr.get()));
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }

    mPropertyUnit.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The unit used by the graphical renderers of the tracks of the group."));
    mPropertyUnit.setVisible(!units.empty());
    if(units.size() == 1_z)
    {
        mPropertyUnit.entry.setText(*units.cbegin(), juce::NotificationType::dontSendNotification);
        mPropertyUnit.entry.onEditorShow = nullptr;
    }
    else if(units.size() > 1_z)
    {
        mPropertyUnit.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
        juce::WeakReference<juce::Component> weakReference(this);
        mPropertyUnit.entry.onEditorShow = [=, this, first = *units.cbegin()]()
        {
            if(weakReference.get() == nullptr)
            {
                return;
            }
            if(auto* editor = mPropertyUnit.entry.getCurrentTextEditor())
            {
                auto const font = mPropertyUnit.entry.getFont();
                editor->setFont(font);
                editor->setIndents(0, static_cast<int>(std::floor(font.getDescent())) - 1);
                editor->setBorder(mPropertyUnit.entry.getBorderSize());
                editor->setJustification(mPropertyUnit.entry.getJustificationType());
                editor->setText(first, false);
                editor->selectAll();
            }
        };
    }
    resized();
}

void Group::PropertyGraphicalsSection::updateLabel()
{
    juce::StringArray trackNames;
    std::set<Track::LabelLayout> labelLayouts;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::label)
        {
            labelLayouts.insert(trackAcsr.get().getAttr<Track::AttrType::labelLayout>());
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyLabelJustification.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The justification of the labels for the graphical renderers of the tracks of the group."));
    mPropertyLabelPosition.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The position of the labels for the graphical renderers of the tracks of the group."));
    mPropertyLabelJustification.setVisible(!labelLayouts.empty());
    mPropertyLabelPosition.setVisible(!labelLayouts.empty());
    if(labelLayouts.size() == 1_z)
    {
        auto const& labelLayout = *labelLayouts.cbegin();
        auto const index = magic_enum::enum_index(labelLayout.justification).value_or(mPropertyLabelJustification.entry.getSelectedItemIndex());
        mPropertyLabelJustification.entry.setSelectedItemIndex(static_cast<int>(index), juce::NotificationType::dontSendNotification);
        mPropertyLabelPosition.entry.setValue(static_cast<double>(labelLayout.position), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyLabelJustification.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
        mPropertyLabelPosition.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
    }
}

void Group::PropertyGraphicalsSection::updateLogScale()
{
    juce::StringArray trackNames;
    std::set<bool> logScales;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::hasVerticalZoomInHertz(trackAcsr.get()))
        {
            logScales.insert(trackAcsr.get().getAttr<Track::AttrType::zoomLogScale>());
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyValueRangeLogScale.setTooltip("Track(s): " + trackNames.joinIntoString(", ") + " - " + juce::translate("The logarithmic scale of the zoom range of the tracks of the group."));
    mPropertyValueRangeLogScale.setVisible(!logScales.empty());
    if(logScales.size() == 1_z)
    {
        mPropertyValueRangeLogScale.entry.setToggleState(*logScales.cbegin(), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyValueRangeLogScale.entry.setToggleState(false, juce::NotificationType::dontSendNotification);
    }
    mPropertyValueRangeLogScale.entry.setAlpha(logScales.size() > 1_z ? 0.5f : 1.0f);
}

ANALYSE_FILE_END
