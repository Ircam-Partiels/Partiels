#include "AnlApplicationGraphicPreset.h"
#include "AnlApplicationInstance.h"

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

static std::vector<std::string> getLabelJustifications()
{
    return {juce::translate("Top").toStdString(), juce::translate("Centred").toStdString(), juce::translate("Bottom").toStdString()};
}

Application::GraphicPresetContent::GraphicPresetContent()
: mPropertyColourMap(juce::translate("Color Map"), juce::translate("The color map of the graphical renderer."), "", getColourMapNames(), [&](size_t index)
                     {
                         setColourMap(static_cast<Track::ColourMap>(index));
                     })
, mPropertyForegroundColour(juce::translate("Foreground Color"), juce::translate("The foreground color of the graphical renderer."), juce::translate("Select the foreground color"), [&](juce::Colour const& colour)
                            {
                                setForegroundColour(colour);
                            },
                            nullptr, nullptr)
, mPropertyDurationColour(juce::translate("Duration Color"), juce::translate("The duration color of the graphical renderer."), juce::translate("Select the duration color"), [&](juce::Colour const& colour)
                          {
                              setDurationColour(colour);
                          },
                          nullptr, nullptr)
, mPropertyBackgroundColour(juce::translate("Background Color"), juce::translate("The background color of the graphical renderer."), juce::translate("Select the background color"), [&](juce::Colour const& colour)
                            {
                                setBackgroundColour(colour);
                            },
                            nullptr, nullptr)
, mPropertyTextColour(juce::translate("Text Color"), juce::translate("The text color of the graphical renderer."), juce::translate("Select the text color"), [&](juce::Colour const& colour)
                      {
                          setTextColour(colour);
                      },
                      nullptr, nullptr)
, mPropertyShadowColour(juce::translate("Shadow Color"), juce::translate("The shadow color of the graphical renderer."), juce::translate("Select the shadow color"), [&](juce::Colour const& colour)
                        {
                            setShadowColour(colour);
                        },
                        nullptr, nullptr)
, mPropertyFontName(juce::translate("Font Name"), juce::translate("The name of the font for the graphical renderer."), "", getFontNames(), [&]([[maybe_unused]] size_t index)
                    {
                        setFontName(mPropertyFontName.entry.getText());
                    })
, mPropertyFontStyle(juce::translate("Font Style"), juce::translate("The style of the font for the graphical renderer."), "", {}, [&]([[maybe_unused]] size_t index)
                     {
                         setFontStyle(mPropertyFontStyle.entry.getText());
                     })
, mPropertyFontSize(juce::translate("Font Size"), juce::translate("The size of the font for the graphical renderer."), "", getFontSizes(), [&]([[maybe_unused]] size_t index)
                    {
                        setFontSize(mPropertyFontSize.entry.getText().getFloatValue());
                    })
, mPropertyLineWidth(juce::translate("Line Width"), juce::translate("The line width for the graphical renderer."), "", {1.0f, 100.0f}, 0.5f, [&](float value)
                     {
                         setLineWidth(value);
                     })
, mPropertyLabelJustification(juce::translate("Label Justification"), juce::translate("The justification of the labels."), "", getLabelJustifications(), [&](size_t index)
                              {
                                  auto fallbackJustification = Instance::get().getApplicationAccessor().getAttr<AttrType::globalGraphicPreset>().labelLayout.justification;
                                  setLabelJustification(magic_enum::enum_cast<Track::LabelLayout::Justification>(static_cast<int>(index)).value_or(fallbackJustification));
                              })
, mPropertyLabelPosition(juce::translate("Label Position"), juce::translate("The position of the labels."), "", {-120.0f, 120.0f}, 0.1f, [this](float position)
                         {
                             setLabelPosition(position);
                         })
{
    addAndMakeVisible(mPropertyColourMap);
    addAndMakeVisible(mPropertyForegroundColour);
    addAndMakeVisible(mPropertyDurationColour);
    addAndMakeVisible(mPropertyBackgroundColour);
    addAndMakeVisible(mPropertyTextColour);
    addAndMakeVisible(mPropertyShadowColour);
    addAndMakeVisible(mPropertyFontName);
    addAndMakeVisible(mPropertyFontStyle);
    addAndMakeVisible(mPropertyFontSize);
    addAndMakeVisible(mPropertyLineWidth);
    addAndMakeVisible(mPropertyLabelJustification);
    addAndMakeVisible(mPropertyLabelPosition);

    setSize(300, 200);

    mListener.onAttrChanged = [this](Accessor const&, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::globalGraphicPreset:
            {
                updateFromGlobalPreset();
            }
            break;
            case AttrType::desktopGlobalScaleFactor:
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::defaultTemplateFile:
            case AttrType::currentTranslationFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
            case AttrType::silentFileManagement:
            case AttrType::routingMatrix:
            case AttrType::autoUpdate:
            case AttrType::lastVersion:
            case AttrType::timeZoomAnchorOnPlayhead:
                break;
        }
    };

    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    updateFromGlobalPreset();
}

Application::GraphicPresetContent::~GraphicPresetContent()
{
    Instance::get().getApplicationAccessor().removeListener(mListener);
}

void Application::GraphicPresetContent::resized()
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
    setBounds(mPropertyBackgroundColour);
    setBounds(mPropertyTextColour);
    setBounds(mPropertyShadowColour);
    setBounds(mPropertyFontName);
    setBounds(mPropertyFontStyle);
    setBounds(mPropertyFontSize);
    setBounds(mPropertyLineWidth);
    setBounds(mPropertyLabelJustification);
    setBounds(mPropertyLabelPosition);

    auto const height = bounds.getY();
    setSize(getWidth(), height);
}

void Application::GraphicPresetContent::updateFromGlobalPreset()
{
    auto const& preset = Instance::get().getApplicationAccessor().getAttr<AttrType::globalGraphicPreset>();

    // Update colour map
    mPropertyColourMap.entry.setSelectedId(static_cast<int>(preset.colours.map) + 1, juce::NotificationType::dontSendNotification);

    // Update colours
    mPropertyForegroundColour.entry.setCurrentColour(preset.colours.foreground, juce::NotificationType::dontSendNotification);
    mPropertyDurationColour.entry.setCurrentColour(preset.colours.duration, juce::NotificationType::dontSendNotification);
    mPropertyBackgroundColour.entry.setCurrentColour(preset.colours.background, juce::NotificationType::dontSendNotification);
    mPropertyTextColour.entry.setCurrentColour(preset.colours.text, juce::NotificationType::dontSendNotification);
    mPropertyShadowColour.entry.setCurrentColour(preset.colours.shadow, juce::NotificationType::dontSendNotification);

    // Update font
    auto const font = juce::Font(preset.font);
    mPropertyFontName.entry.setText(font.getTypefaceName(), juce::NotificationType::dontSendNotification);

    // Update font style
    auto const styles = font.getAvailableStyles();
    mPropertyFontStyle.entry.clear(juce::NotificationType::dontSendNotification);
    for(auto i = 0; i < styles.size(); ++i)
    {
        mPropertyFontStyle.entry.addItem(styles[i], i + 1);
    }
    mPropertyFontStyle.entry.setText(font.getTypefaceStyle(), juce::NotificationType::dontSendNotification);

    // Update font size
    mPropertyFontSize.entry.setText(juce::String(static_cast<int>(font.getHeight())), juce::NotificationType::dontSendNotification);

    // Update line width
    mPropertyLineWidth.entry.setValue(static_cast<double>(preset.lineWidth), juce::NotificationType::dontSendNotification);

    // Update label justification
    mPropertyLabelJustification.entry.setSelectedId(static_cast<int>(preset.labelLayout.justification) + 1, juce::NotificationType::dontSendNotification);

    // Update label position
    mPropertyLabelPosition.entry.setValue(static_cast<double>(preset.labelLayout.position), juce::NotificationType::dontSendNotification);
}

void Application::GraphicPresetContent::setColourMap(Track::ColourMap const& colourMap)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.colours.map = colourMap;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setForegroundColour(juce::Colour const& colour)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.colours.foreground = colour;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setDurationColour(juce::Colour const& colour)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.colours.duration = colour;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setBackgroundColour(juce::Colour const& colour)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.colours.background = colour;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setTextColour(juce::Colour const& colour)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.colours.text = colour;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setShadowColour(juce::Colour const& colour)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.colours.shadow = colour;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setFontName(juce::String const& name)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    auto newFont = juce::FontOptions(name, preset.font.getHeight(), juce::Font::FontStyleFlags::plain);
    if(juce::Font(newFont).getAvailableStyles().contains(preset.font.getStyle()))
    {
        newFont = newFont.withStyle(preset.font.getStyle());
    }
    preset.font = newFont;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setFontStyle(juce::String const& style)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.font = preset.font.withStyle(style);
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setFontSize(float size)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.font = preset.font.withHeight(size);
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setLineWidth(float width)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.lineWidth = width;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setLabelJustification(Track::LabelLayout::Justification justification)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.labelLayout.justification = justification;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

void Application::GraphicPresetContent::setLabelPosition(float position)
{
    auto& accessor = Instance::get().getApplicationAccessor();
    auto preset = accessor.getAttr<AttrType::globalGraphicPreset>();
    preset.labelLayout.position = position;
    accessor.setAttr<AttrType::globalGraphicPreset>(preset, NotificationType::synchronous);
}

Application::GraphicPresetPanel::GraphicPresetPanel()
: HideablePanelTyped<GraphicPresetContent>(juce::translate("Graphic Preset"))
{
}

ANALYSE_FILE_END
