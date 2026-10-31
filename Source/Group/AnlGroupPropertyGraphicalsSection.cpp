#include "AnlGroupPropertyGraphicalsSection.h"
#include "../Track/AnlTrackTools.h"
#include "../Track/AnlTrackTooltip.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

static juce::StringArray getColourMapNames()
{
    juce::StringArray names;
    for(auto const& name : magic_enum::enum_names<Track::ColourMap>())
    {
        names.add(juce::String(std::string(name)));
    }
    return names;
}

static juce::StringArray getFontSizes()
{
    juce::StringArray names;
    for(auto size = 8; size <= 20; size += 2)
    {
        names.add(juce::String(size));
    }
    return names;
}

static juce::StringArray getLabelJustifications()
{
    return {juce::translate("Top"), juce::translate("Centred"), juce::translate("Bottom")};
}

static juce::StringArray getMarkerStyles()
{
    return {juce::translate("Bar"), juce::translate("Underline")};
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
, mPropertyFontName(juce::translate("Font Name"), juce::translate("The name of the font for the graphical renderers of the tracks of the group."), "", juce::Font::findAllTypefaceNames(), [&]([[maybe_unused]] size_t index)
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
, mPropertyLineWidth(juce::translate("Line Width"), juce::translate("The line width for the graphical renderers of the tracks of the group."), "", {1.0, 100.0}, 0.5, [&](double value)
                     {
                         mDirector.startAction(true);
                         setLineWidth(static_cast<float>(value));
                         mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's line width"));
                     })
, mPropertyUnit(juce::translate("Unit"), juce::translate("The unit of the values for the graphical renderers of the tracks of the group."), [&](juce::String text)
                {
                    setUnit(text);
                })
, mPropertyLabelJustification(juce::translate("Label Justification"), juce::translate("The justification of the labels for the graphical renderers of the tracks of the group."), "", getLabelJustifications(), [&](size_t index)
                              {
                                  mDirector.startAction(true);
                                  setLabelJustification(magic_enum::enum_cast<Track::LabelLayout::Justification>(static_cast<int>(index)).value_or(Track::LabelLayout::Justification::top));
                                  mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's justification of the labels"));
                              })
, mPropertyLabelPosition(juce::translate("Label Position"), juce::translate("The position of the labels."), "", {-120.0, 120.0}, 0.1, [this](double position)
                         {
                             mDirector.startAction(true);
                             setLabelPosition(static_cast<float>(position));
                             mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's position of the labels"));
                         })
, mPropertyMarkerStyle(juce::translate("Marker Style"), juce::translate("The style of the markers for the graphical renderers of the tracks of the group."), "", getMarkerStyles(), [&](size_t index)
                       {
                           mDirector.startAction(true);
                           setMarkerStyle(magic_enum::enum_cast<Track::MarkerStyle>(static_cast<int>(index)).value_or(Track::MarkerStyle::bar));
                           mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group's style of the markers"));
                       })
, mPropertyValueRangeLogScale(juce::translate("Value Log. Scale"), juce::translate("Toggle the logarithmic scale of the zoom range of the tracks of the group."), [&](bool value)
                              {
                                  mDirector.startAction(true);
                                  setLogScale(value);
                                  mDirector.endAction(true, ActionState::newTransaction, juce::translate("Change group log scale"));
                              })
, mPropertyChannelLayout(juce::translate("Channel Layout"), juce::translate("The visibility of the channels of the group."), [this]()
                         {
                             showChannelLayout();
                         })
, mPropertyTrackVisibility(juce::translate("Track Visibility"), juce::translate("The visibility of the tracks of the group."), [this]()
                           {
                               showTrackVisibility();
                           })
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::name, Track::AttrType::graphicsSettings, Track::AttrType::description, Track::AttrType::channelsLayout, Track::AttrType::showInGroup, Track::AttrType::results, Track::AttrType::hasPluginColourMap, Track::AttrType::zoomLogScale})
, mThresholdsNotifier(mAccessor, [this]()
                      {
                          updateExtraThresholdStates();
                      },
                      {Track::AttrType::extraThresholds})
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
    addAndMakeVisible(mPropertyMarkerStyle);
    addAndMakeVisible(mPropertyValueRangeLogScale);
    addAndMakeVisible(mPropertyTrackVisibility);
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
    setBounds(mPropertyMarkerStyle);
    setBounds(mPropertyValueRangeLogScale);
    for(auto const& propertyExtraThreshold : mPropertyExtraThresholds)
    {
        setBounds(*propertyExtraThreshold.second.second.get());
    }
    setBounds(mPropertyTrackVisibility);
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();
        settings.colours.map = colourMap;
        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();
        settings.colours.foreground = colour;
        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();
        settings.colours.duration = colour;
        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();
        settings.colours.background = colour;
        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();
        settings.colours.text = colour;
        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();
        settings.colours.shadow = colour;
        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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

    auto const font = trackAcsrs.front().get().getAttr<Track::AttrType::graphicsSettings>().font;
    auto newFont = juce::FontOptions(name, font.getHeight(), juce::Font::FontStyleFlags::plain);
    if(juce::Font(newFont).getAvailableStyles().contains(font.getStyle()))
    {
        newFont = newFont.withStyle(font.getStyle());
    }
    for(auto& trackAcsr : trackAcsrs)
    {
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();

        settings.font = font;

        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto const font = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>().font;
        if(juce::Font(font).getAvailableStyles().contains(style))
        {
            auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();

            settings.font = font.withStyle(style);

            trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto const font = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>().font.withHeight(size);
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();

        settings.font = font;

        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();

        settings.lineWidth = size;

        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
                                                 auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();

                                                 settings.unit = juce::String();

                                                 trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
                                             }
                                         }
                                         else
                                         {
                                             for(auto& trackAcsr : trackAcsrs)
                                             {
                                                 auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();

                                                 settings.unit = std::optional<juce::String>{};

                                                 trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
            auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();

            settings.unit = unit;

            trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();
        settings.labelLayout.justification = justification;
        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();
        settings.labelLayout.position = position;
        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
    }
    updateLabel();
}

void Group::PropertyGraphicalsSection::setMarkerStyle(Track::MarkerStyle markerStyle)
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
        auto settings = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>();
        settings.markerStyle = markerStyle;
        trackAcsr.get().setAttr<Track::AttrType::graphicsSettings>(settings, NotificationType::synchronous);
    }
    updateMarkerStyle();
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

void Group::PropertyGraphicalsSection::showTrackVisibility()
{
    juce::PopupMenu menu;
    Tools::fillMenuForTrackVisibility(mAccessor, menu, nullptr, [this]()
                                      {
                                          showTrackVisibility();
                                      });
    if(!std::exchange(mTrackVisibilityActionStarted, true))
    {
        mDirector.startAction(true);
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mPropertyTrackVisibility.entry).withDeletionCheck(*this), [=, this](int menuResult)
                       {
                           if(menuResult == 0 && std::exchange(mTrackVisibilityActionStarted, false))
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
    updateMarkerStyle();
    updateLogScale();
    updateExtraThresholds();
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
            auto const colourMap = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>().colours.map;
            colourMaps.insert(colourMap);
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyColourMap.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The color map used by the graphical renderers of the tracks of the group."));
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
            auto const& colours = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>().colours;
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
    mPropertyForegroundColour.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The foreground color used by the graphical renderers of the tracks of the group."));
    mPropertyForegroundColour.setVisible(!foregroundColours.empty());
    if(foregroundColours.size() == 1_z)
    {
        mPropertyForegroundColour.entry.setCurrentColour(juce::Colour(*foregroundColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyForegroundColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyDurationColour.setTooltip(juce::translate("Track(s): ") + labelTrackNames.joinIntoString(", ") + " - " + juce::translate("The duration color used by the graphical renderers of the tracks of the group."));
    mPropertyDurationColour.setVisible(!durationColours.empty());
    if(durationColours.size() == 1_z)
    {
        mPropertyDurationColour.entry.setCurrentColour(juce::Colour(*durationColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyDurationColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyBackgroundColour.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The background color used by the graphical renderers of the tracks of the group."));
    mPropertyBackgroundColour.setVisible(!backgrounColours.empty());
    if(backgrounColours.size() == 1_z)
    {
        mPropertyBackgroundColour.entry.setCurrentColour(juce::Colour(*backgrounColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyBackgroundColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyTextColour.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The text color used by the graphical renderers of the tracks of the group."));
    mPropertyTextColour.setVisible(!textColours.empty());
    if(textColours.size() == 1_z)
    {
        mPropertyTextColour.entry.setCurrentColour(juce::Colour(*textColours.cbegin()), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyTextColour.entry.setCurrentColour(juce::Colours::transparentBlack, juce::NotificationType::dontSendNotification);
    }

    mPropertyShadowColour.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The shadow color used by the graphical renderers of the tracks of the group."));
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
            auto const font = trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>().font;
            if(!currentFont.has_value())
            {
                currentFont = font;
            }
            fontNames.insert(font.getName());
            fontStyles.insert(font.getStyle());
            fontSizes.insert(font.getHeight());
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }

    mPropertyFontName.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The font name used by the graphical renderers of the tracks of the group."));
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

    mPropertyFontStyle.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The font style used by the graphical renderers of the tracks of the group."));
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

    mPropertyFontSize.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The font size used by the graphical renderers of the tracks of the group."));
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
            lineWidth.insert(trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>().lineWidth);
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }

    mPropertyLineWidth.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The line width used by the graphical renderers of the tracks of the group."));
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

    mPropertyUnit.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The unit used by the graphical renderers of the tracks of the group."));
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
            labelLayouts.insert(trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>().labelLayout);
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyLabelJustification.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The justification of the labels for the graphical renderers of the tracks of the group."));
    mPropertyLabelPosition.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The position of the labels for the graphical renderers of the tracks of the group."));
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

void Group::PropertyGraphicalsSection::updateMarkerStyle()
{
    juce::StringArray trackNames;
    std::set<Track::MarkerStyle> markerStyles;
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    for(auto const& trackAcsr : trackAcsrs)
    {
        if(Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::label)
        {
            markerStyles.insert(trackAcsr.get().getAttr<Track::AttrType::graphicsSettings>().markerStyle);
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
    }
    mPropertyMarkerStyle.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The style of the markers for the graphical renderers of the tracks of the group."));
    mPropertyMarkerStyle.setVisible(!markerStyles.empty());
    if(markerStyles.size() == 1_z)
    {
        auto const index = magic_enum::enum_index(*markerStyles.cbegin()).value_or(mPropertyMarkerStyle.entry.getSelectedItemIndex());
        mPropertyMarkerStyle.entry.setSelectedItemIndex(static_cast<int>(index), juce::NotificationType::dontSendNotification);
    }
    else
    {
        mPropertyMarkerStyle.entry.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
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
    mPropertyValueRangeLogScale.setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + juce::translate("The logarithmic scale of the zoom range of the tracks of the group."));
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

static std::optional<size_t> getExtraOutputIndex(Track::Accessor const& trackAcsr, juce::String const& outputIdentifier)
{
    auto const& extraOutputs = trackAcsr.getAttr<Track::AttrType::description>().extraOutputs;
    auto const it = std::find_if(extraOutputs.cbegin(), extraOutputs.cend(), [&](auto const& extraOutput)
                                 {
                                     return extraOutput.identifier == outputIdentifier;
                                 });
    if(it == extraOutputs.cend())
    {
        return {};
    }
    return static_cast<size_t>(std::distance(extraOutputs.cbegin(), it));
}

void Group::PropertyGraphicalsSection::updateExtraThresholds()
{
    std::map<juce::String, std::vector<std::reference_wrapper<Track::Accessor>>> extraOutputsToTracks;
    for(auto const& trackAcsr : Tools::getTrackAcsrs(mAccessor))
    {
        if(Track::Tools::getFrameType(trackAcsr.get()) != Track::FrameType::vector)
        {
            for(auto const& extraOutput : trackAcsr.get().getAttr<Track::AttrType::description>().extraOutputs)
            {
                extraOutputsToTracks[extraOutput.identifier].push_back(trackAcsr);
            }
        }
    }

    // The properties are only recreated if their description changed to preserve the keyboard focus
    decltype(mPropertyExtraThresholds) properties;
    for(auto const& extraOutputsToTrack : extraOutputsToTracks)
    {
        auto const& outputIdentifier = extraOutputsToTrack.first;
        auto const& referenceTrackAcsr = extraOutputsToTrack.second.front().get();
        auto const outputIndex = getExtraOutputIndex(referenceTrackAcsr, outputIdentifier);
        MiscWeakAssert(outputIndex.has_value());
        if(!outputIndex.has_value())
        {
            continue;
        }

        auto const& output = referenceTrackAcsr.getAttr<Track::AttrType::description>().extraOutputs.at(outputIndex.value());
        auto& property = properties[outputIdentifier];
        auto it = mPropertyExtraThresholds.find(outputIdentifier);
        if(it != mPropertyExtraThresholds.end() && it->second.first == output)
        {
            property = std::move(it->second);
        }
        else
        {
            auto const vname = output.name.empty() ? juce::translate("Extra OUTIDX").replace("OUTIDX", juce::String(outputIndex.value()))
                                                   : juce::String(output.name);
            auto const name = juce::translate("OUTNAME Threshold").replace("OUTNAME", vname);
            auto const step = output.isQuantized ? output.quantizeStep : 0.0f;

            juce::WeakReference<juce::Component> weakReference(this);
            auto const startChange = [=, this]()
            {
                if(weakReference.get() == nullptr)
                {
                    return;
                }
                mDirector.startAction(true);
            };
            auto const endChange = [=, this]()
            {
                if(weakReference.get() == nullptr)
                {
                    return;
                }
                mDirector.endAction(true, ActionState::newTransaction, juce::translate("Modify the OUTNAME threshold").replace("OUTNAME", vname));
            };
            auto const applyChange = [=, this](float newValue)
            {
                if(weakReference.get() == nullptr)
                {
                    return;
                }
                auto const propertyIt = mPropertyExtraThresholds.find(outputIdentifier);
                MiscWeakAssert(propertyIt != mPropertyExtraThresholds.end());
                if(propertyIt == mPropertyExtraThresholds.end())
                {
                    return;
                }
                auto const& slider = propertyIt->second.second->entry;
                auto const value = static_cast<float>(std::clamp(static_cast<double>(newValue), slider.getMinimum(), slider.getMaximum()));
                for(auto const& trackAcsr : Tools::getTrackAcsrs(mAccessor))
                {
                    if(Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector)
                    {
                        continue;
                    }
                    auto const index = getExtraOutputIndex(trackAcsr.get(), outputIdentifier);
                    if(!index.has_value())
                    {
                        continue;
                    }
                    auto thresholds = trackAcsr.get().getAttr<Track::AttrType::extraThresholds>();
                    thresholds.resize(trackAcsr.get().getAttr<Track::AttrType::description>().extraOutputs.size());
                    thresholds[index.value()] = value;
                    trackAcsr.get().setAttr<Track::AttrType::extraThresholds>(thresholds, NotificationType::synchronous);
                }
            };
            // The range and the tooltip are updated below
            property = std::make_pair(output, std::make_unique<PropertySlider>(name, "", output.unit, juce::Range<float>{0.0f, 1.0f}, step, startChange, applyChange, endChange, true));
            addAndMakeVisible(property.second.get());
        }

        auto const srange = Track::Tools::getExtraRange(referenceTrackAcsr, outputIndex.value());
        auto const start = srange.has_value() ? static_cast<float>(srange.value().getStart()) : 0.0f;
        auto const end = srange.has_value() ? static_cast<float>(srange.value().getEnd()) : 0.0f;
        auto const step = static_cast<double>(output.isQuantized ? output.quantizeStep : 0.0f);
        juce::Range<double> const range{static_cast<double>(start), static_cast<double>(std::max(end, start + std::numeric_limits<float>::epsilon() * 100.0f))};
        property.second->entry.setRange(range, step);
        property.second->numberField.setRange(range, step, juce::NotificationType::dontSendNotification);
        property.second->setEnabled(srange.has_value() && !srange.value().isEmpty());

        juce::StringArray trackNames;
        for(auto const& trackAcsr : extraOutputsToTrack.second)
        {
            trackNames.add(trackAcsr.get().getAttr<Track::AttrType::name>());
        }
        property.second->setTooltip(juce::translate("Track(s): ") + trackNames.joinIntoString(", ") + " - " + Track::Tools::getExtraTooltip(referenceTrackAcsr, outputIndex.value()));
    }
    mPropertyExtraThresholds = std::move(properties);
    updateExtraThresholdStates();
}

void Group::PropertyGraphicalsSection::updateExtraThresholdStates()
{
    auto const trackAcsrs = copy_with_erased_if(Tools::getTrackAcsrs(mAccessor), [](auto const& trackAcsr)
                                                {
                                                    return Track::Tools::getFrameType(trackAcsr.get()) == Track::FrameType::vector;
                                                });

    for(auto const& propertyExtraThreshold : mPropertyExtraThresholds)
    {
        auto const& outputIdentifier = propertyExtraThreshold.first;
        auto const& property = propertyExtraThreshold.second.second;
        MiscWeakAssert(property != nullptr);
        if(property != nullptr)
        {
            std::set<float> thresholdValues;
            for(auto const& trackAcsr : trackAcsrs)
            {
                auto const index = getExtraOutputIndex(trackAcsr.get(), outputIdentifier);
                if(index.has_value())
                {
                    auto const& thresholds = trackAcsr.get().getAttr<Track::AttrType::extraThresholds>();
                    if(index.value() < thresholds.size() && thresholds.at(index.value()).has_value())
                    {
                        thresholdValues.insert(thresholds.at(index.value()).value());
                    }
                }
            }

            if(!thresholdValues.empty())
            {
                property->entry.setValue(static_cast<double>(*thresholdValues.cbegin()), juce::NotificationType::dontSendNotification);
                property->numberField.setValue(static_cast<double>(*thresholdValues.cbegin()), juce::NotificationType::dontSendNotification);
                if(thresholdValues.size() > 1)
                {
                    property->numberField.setText(juce::translate("Multiple Values"), juce::NotificationType::dontSendNotification);
                }
            }
            else
            {
                auto const defaultValue = property->entry.getDoubleClickReturnValue();
                property->entry.setValue(defaultValue, juce::NotificationType::dontSendNotification);
                property->numberField.setValue(defaultValue, juce::NotificationType::dontSendNotification);
            }
        }
    }
}

ANALYSE_FILE_END
