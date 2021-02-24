#include "AnlAnalyzerPropertyPanel.h"

ANALYSE_FILE_BEGIN

Analyzer::PropertyPanel::PropertyTextButton::PropertyTextButton(juce::String const& name, juce::String const& tooltip, std::function<void(void)> fn)
: PropertyComponent<juce::TextButton>(juce::translate(name), juce::translate(tooltip), nullptr)
{
    title.setVisible(false);
    entry.setButtonText(name);
    entry.setTooltip(tooltip);
    entry.onClick = [=]()
    {
        if(fn != nullptr)
        {
            fn();
        }
    };
}

void Analyzer::PropertyPanel::PropertyTextButton::resized()
{
    entry.setBounds(getLocalBounds().reduced(32, 2));
}

Analyzer::PropertyPanel::PropertyText::PropertyText(juce::String const& name, juce::String const& tooltip, std::function<void(juce::String)> fn)
: PropertyComponent<juce::Label>(juce::translate(name), juce::translate(tooltip))
{
    entry.setRepaintsOnMouseActivity(true);
    entry.setEditable(true);
    entry.setTooltip(tooltip);
    entry.setJustificationType(juce::Justification::right);
    entry.setMinimumHorizontalScale(1.0f);
    entry.setBorderSize({});
    entry.onEditorShow = [&]()
    {
        if(auto* editor = entry.getCurrentTextEditor())
        {
            auto const font = entry.getFont();
            editor->setFont(font);
            editor->setIndents(0, static_cast<int>(std::floor(font.getDescent())) - 1);
            editor->setBorder(entry.getBorderSize());
            editor->setJustification(entry.getJustificationType());
        }
    };
    entry.onTextChange = [&]()
    {
        if(fn != nullptr)
        {
            fn(entry.getText());
        }
    };
}

Analyzer::PropertyPanel::PropertyLabel::PropertyLabel(juce::String const& name, juce::String const& tooltip)
: PropertyText(name, tooltip, nullptr)
{
    entry.setEditable(false, false);
}

Analyzer::PropertyPanel::PropertyNumber::PropertyNumber(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, juce::Range<float> const& range, float interval, std::function<void(float)> fn)
: PropertyComponent<NumberField>(juce::translate(name), juce::translate(tooltip))
{
    entry.setRange({static_cast<double>(range.getStart()), static_cast<double>(range.getEnd())}, static_cast<double>(interval), juce::NotificationType::dontSendNotification);
    entry.setTooltip(juce::translate(tooltip));
    entry.setJustificationType(juce::Justification::centredRight);
    entry.setTextValueSuffix(suffix);
    entry.onValueChanged = [=](double value)
    {
        if(fn != nullptr)
        {
            fn(static_cast<float>(value));
        }
    };
}

Analyzer::PropertyPanel::PropertyList::PropertyList(juce::String const& name, juce::String const& tooltip, juce::String const& suffix, std::vector<std::string> const& values, std::function<void(size_t)> fn)
: PropertyComponent<juce::ComboBox>(juce::translate(name), juce::translate(tooltip))
{
    entry.setTooltip(tooltip);
    juce::StringArray items;
    for(auto const& value : values)
    {
        items.add(juce::String(value)+suffix);
    }
    entry.addItemList(items, 1);
    entry.setJustificationType(juce::Justification::centredRight);
    entry.onChange = [=, this]()
    {
        if(fn != nullptr)
        {
            fn(entry.getSelectedItemIndex() > 0 ? static_cast<size_t>(entry.getSelectedItemIndex()) : 0_z);
        }
    };
}

Analyzer::PropertyPanel::PropertyPanel(Accessor& accessor)
: mAccessor(accessor)

, mPropertyName("Name", "The name of the analyzer", [&](juce::String text)
{
    mAccessor.setAttr<AttrType::name>(text, NotificationType::synchronous);
})

, mPropertyWindowType("Window Type", "The window type of the FFT.", "", std::vector<std::string>{"Rectangular", "Triangular", "Hamming", "Hanning", "Blackman", "Nuttall", "BlackmanHarris"}, [=](size_t index)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    state.windowType = static_cast<Plugin::WindowType>(index);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
, mPropertyWindowSize("Window Size", "The window size of the FFT.", "samples", std::vector<std::string>{"8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096"}, [&](size_t index)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    auto const overlapping = state.blockSize / state.stepSize;
    state.blockSize = static_cast<size_t>(std::pow(2.0, static_cast<int>(index) + 3));
    state.stepSize = state.blockSize / overlapping;
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
, mPropertyWindowOverlapping("Window Overlapping", "The window overlapping of the FFT.", "x", std::vector<std::string>{}, [&](size_t index)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    state.stepSize = state.blockSize / std::max(static_cast<size_t>(std::pow(2.0, static_cast<int>(index))), 1_z);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
, mPropertyBlockSize("Block Size", "The block size used by the analyzer. [1:65536]", "samples", {1.0f, 65536.0f}, 1.0f, [&](float value)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    state.blockSize = static_cast<size_t>(value);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
, mPropertyStepSize("Step Size", "The step size used by the analyzer. [1:65536]", "samples", {1.0f, 65536.0f}, 1.0f, [&](float value)
{
    auto state = mAccessor.getAttr<AttrType::state>();
    state.stepSize = static_cast<size_t>(value);
    mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
})
, mPropertyResetProcessor("Reset", "Reset processor to the default state", [&]()
{
    mAccessor.setAttr<AttrType::state>(mAccessor.getAttr<AttrType::description>().defaultState, NotificationType::synchronous);
})
      
, mPropertyColourMap("Colour Map", "The colour map of the graphical renderer.", "", std::vector<std::string>{"Parula", "Heat", "Jet", "Turbo", "Hot", "Gray", "Magma", "Inferno", "Plasma", "Viridis", "Cividis", "Github"}, [&](size_t index)
{
    auto colours = mAccessor.getAttr<AttrType::colours>();
    colours.map = static_cast<ColourMap>(index);
    mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
})
, mPropertyForegroundColour("Foreground Color", "The foreground current color of the graphical renderer.", [&]()
{
    ColourSelector colourSelector;
    colourSelector.setSize(400, 300);
    colourSelector.setCurrentColour(mAccessor.getAttr<AttrType::colours>().foreground, juce::NotificationType::dontSendNotification);
    colourSelector.onColourChanged = [&](juce::Colour const& colour)
    {
        auto colours = mAccessor.getAttr<AttrType::colours>();
        colours.foreground = colour;
        mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
    };
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = juce::translate("Select the foreground color");
    options.content.setNonOwned(&colourSelector);
    options.componentToCentreAround = this;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.runModal();
})

, mPropertyBackgroundColour("Background Color", "The background current color of the graphical renderer.", [&]()
{
    ColourSelector colourSelector;
    colourSelector.setSize(400, 300);
    colourSelector.setCurrentColour(mAccessor.getAttr<AttrType::colours>().background, juce::NotificationType::dontSendNotification);
    colourSelector.onColourChanged = [&](juce::Colour const& colour)
    {
        auto colours = mAccessor.getAttr<AttrType::colours>();
        colours.background = colour;
        mAccessor.setAttr<AttrType::colours>(colours, NotificationType::synchronous);
    };
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = juce::translate("Select the background color");
    options.content.setNonOwned(&colourSelector);
    options.componentToCentreAround = this;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.runModal();
})

, mPropertyValueRangeMin("Value Range Min.", "The minimum value of the output.", "", {Zoom::lowest(), Zoom::max()}, 0.0, [&](float value)
{
    auto& zoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>().withStart(static_cast<double>(value));
    zoomAcsr.setAttr<Zoom::AttrType::globalRange>(range, NotificationType::synchronous);
})
, mPropertyValueRangeMax("Value Range Max.", "The maximum value of the output.", "", {Zoom::lowest(), Zoom::max()}, 0.0, [&](float value)
{
    auto& zoomAcsr = mAccessor.getAccessor<AcsrType::valueZoom>(0);
    auto const range = zoomAcsr.getAttr<Zoom::AttrType::globalRange>().withEnd(static_cast<double>(value));
    zoomAcsr.setAttr<Zoom::AttrType::globalRange>(range, NotificationType::synchronous);
})
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
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
                    auto const setValue = [=, this](float value)
                    {
                        auto state = mAccessor.getAttr<AttrType::state>();
                        anlWeakAssert(value >= parameter.minValue && value <= parameter.maxValue);
                        state.parameters[parameter.identifier] = std::min(std::max(value, parameter.minValue), parameter.maxValue);
                        mAccessor.setAttr<AttrType::state>(state, NotificationType::synchronous);
                    };
                    
                    if(parameter.valueNames.empty())
                    {
                        auto const description = juce::String(parameter.description) + " [" + juce::String(parameter.minValue, 2) + ":" + juce::String(parameter.maxValue, 2) + (!parameter.isQuantized ? "" : ("-" + juce::String(parameter.quantizeStep, 2))) + "]";
                        return std::make_unique<PropertyNumber>(parameter.name, description, parameter.unit, juce::Range<float>{parameter.minValue, parameter.maxValue}, parameter.isQuantized ? parameter.quantizeStep : 0.0f, [=](float value)
                        {
                            setValue(value);
                        });
                    }
                    return std::make_unique<PropertyList>(parameter.name, parameter.description, parameter.unit, parameter.valueNames, [=](size_t index)
                    {
                        setValue(static_cast<float>(index));
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
                    mParameterProperties[parameter.identifier] = createProperty(parameter);
                }
                for(auto const& property : mParameterProperties)
                {
                    components.push_back(*property.second.get());
                }
                components.push_back(mPropertyResetProcessor);
                mProcessorSection.setComponents(components);
                components.clear();
                
                // Graphical Part
                auto const output = description.output;
                mPropertyValueRangeMin.entry.setTextValueSuffix(output.unit);
                mPropertyValueRangeMax.entry.setTextValueSuffix(output.unit);

                // Plugin Information Part
                mPropertyPluginName.entry.setText(description.name, silent);
                mPropertyPluginFeature.entry.setText(description.output.name, silent);
                mPropertyPluginMaker.entry.setText(description.maker, silent);
                mPropertyPluginVersion.entry.setText(juce::String(description.version), silent);
                mPropertyPluginCategory.entry.setText(description.category.isEmpty() ? "-" : description.category, silent);
                mPropertyPluginDetails.setText(description.details + " - " + description.output.description, silent);
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
                    anlStrongAssert(it != mParameterProperties.end());
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
                        else
                        {
                            anlStrongAssert(false && "property unsupported");
                        }
                    }
                }
            }
                break;
            case AttrType::results:
            {
                auto const description = mAccessor.getAttr<AttrType::description>();
                auto const& results = mAccessor.getAttr<AttrType::results>();
                if(results.empty())
                {
                    break;
                }
                auto const output = description.output;
                auto const numDimensions = output.hasFixedBinCount ? output.binCount : results.front().values.size();
                auto getPropertyColour = [&]() -> juce::Component&
                {
                    if(numDimensions > 1)
                    {
                        return mPropertyColourMap;
                    }
                    return mPropertyForegroundColour;
                };
                
                mGraphicalSection.setComponents(
                {
                      getPropertyColour()
                    , mPropertyBackgroundColour
                    , mPropertyValueRangeMin
                    , mPropertyValueRangeMax
                });
            }
                break;
            case AttrType::warnings:
            case AttrType::time:
            case AttrType::processing:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::propertyState:
                break;
        }
    };
    
    mValueZoomListener.onAttrChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            {
                auto const range = acsr.getAttr<Zoom::AttrType::globalRange>();
                mPropertyValueRangeMin.entry.setValue(range.getStart(), juce::NotificationType::dontSendNotification);
                mPropertyValueRangeMax.entry.setValue(range.getEnd(), juce::NotificationType::dontSendNotification);
            }
                break;
            case Zoom::AttrType::minimumLength:
                break;
            case Zoom::AttrType::visibleRange:
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
                
            }
                break;
            case Zoom::AttrType::minimumLength:
                break;
            case Zoom::AttrType::visibleRange:
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
    
    mPropertyPluginDetails.setTooltip(juce::translate("The details of the plugin"));
    mPropertyPluginDetails.setSize(sInnerWidth, 48);
    mPropertyPluginDetails.setJustification(juce::Justification::horizontallyJustified);
    mPropertyPluginDetails.setMultiLine(true);
    mPropertyPluginDetails.setReadOnly(true);
    mPropertyPluginDetails.setScrollbarsShown(true);
    mPluginSection.setComponents(
    {
          mPropertyPluginName
        , mPropertyPluginFeature
        , mPropertyPluginMaker
        , mPropertyPluginVersion
        , mPropertyPluginCategory
        , mPropertyPluginDetails
    });
    
    addAndMakeVisible(mPropertyName);
    addAndMakeVisible(mProcessorSection);
    addAndMakeVisible(mGraphicalSection);
    addAndMakeVisible(mPluginSection);
    
    setSize(sInnerWidth, 400);
    mViewport.setSize(sInnerWidth + mViewport.getVerticalScrollBar().getWidth() + 2, 400);
    mViewport.setScrollBarsShown(true, false, true, false);
    mViewport.setViewedComponent(this, false);
    mFloatingWindow.setContentNonOwned(&mViewport, true);
    mFloatingWindow.setResizable(true, false);
    mBoundsConstrainer.setMinimumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMaximumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMinimumHeight(120);
    mBoundsConstrainer.setMinimumOnscreenAmounts(0, 40, 40, 40);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);
    mFloatingWindow.onChanged = [&]()
    {
        mAccessor.setAttr<AttrType::propertyState>(mFloatingWindow.getWindowStateAsString(), NotificationType::synchronous);
    };
    
    mAccessor.getAccessor<AcsrType::valueZoom>(0).addListener(mValueZoomListener, NotificationType::synchronous);
    mAccessor.getAccessor<AcsrType::binZoom>(0).addListener(mBinZoomListener, NotificationType::synchronous);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Analyzer::PropertyPanel::~PropertyPanel()
{
    mBoundsListener.detachFrom(mProcessorSection);
    mBoundsListener.detachFrom(mGraphicalSection);
    mBoundsListener.detachFrom(mPluginSection);
    mAccessor.removeListener(mListener);
    mAccessor.getAccessor<AcsrType::binZoom>(0).removeListener(mBinZoomListener);
    mAccessor.getAccessor<AcsrType::valueZoom>(0).removeListener(mValueZoomListener);
}

void Analyzer::PropertyPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mPropertyName.setBounds(bounds.removeFromTop(mPropertyName.getHeight()));
    mProcessorSection.setBounds(bounds.removeFromTop(mProcessorSection.getHeight()));
    mGraphicalSection.setBounds(bounds.removeFromTop(mGraphicalSection.getHeight()));
    mPluginSection.setBounds(bounds.removeFromTop(mPluginSection.getHeight()));
    setSize(sInnerWidth, std::max(bounds.getY(), 120) + 2);
}

void Analyzer::PropertyPanel::show()
{
    auto const& displayInfo = mAccessor.getAttr<AttrType::propertyState>();
    if(displayInfo.isEmpty())
    {
        auto const& desktop = juce::Desktop::getInstance();
        auto const mousePosition = desktop.getMainMouseSource().getScreenPosition().toInt();
        auto const* display = desktop.getDisplays().getDisplayForPoint(mousePosition);
        anlStrongAssert(display != nullptr);
        if(display != nullptr)
        {
            auto const bounds = display->userArea.withSizeKeepingCentre(mFloatingWindow.getWidth(), mFloatingWindow.getHeight());
            mFloatingWindow.setBounds(bounds);
        }
    }
    else
    {
        mFloatingWindow.restoreWindowStateFromString(displayInfo);
    }
    mFloatingWindow.setVisible(true);
}

ANALYSE_FILE_END
