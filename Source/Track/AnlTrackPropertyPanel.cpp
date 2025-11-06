#include "AnlTrackPropertyPanel.h"
#include "AnlTrackExporter.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::PropertyPanel::WindowContainer::WindowContainer(PropertyPanel& propertyPanel)
: FloatingWindowContainer(juce::translate("Properties"), mViewport)
, mPropertyPanel(propertyPanel)
{
    mViewport.setSize(mPropertyPanel.getWidth() + mViewport.getVerticalScrollBar().getWidth() + 2, 400);
    mViewport.setScrollBarsShown(true, false, true, false);
    mViewport.setViewedComponent(&mPropertyPanel, false);
    mFloatingWindow.setResizable(true, false);
    mBoundsConstrainer.setMinimumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMaximumWidth(mViewport.getWidth() + mFloatingWindow.getBorderThickness().getTopAndBottom());
    mBoundsConstrainer.setMinimumHeight(120);
    mFloatingWindow.setConstrainer(&mBoundsConstrainer);
}

Track::PropertyPanel::PropertyPanel(Director& director, PresetList::Accessor& presetListAcsr)
: mDirector(director)
, mPropertyName("Name", "The name of the track", [&](juce::String text)
                {
                    mDirector.startAction();
                    mAccessor.setAttr<AttrType::name>(text, NotificationType::synchronous);
                    mDirector.endAction(ActionState::newTransaction, juce::translate("Change track name"));
                })
, mPropertyProcessorSection(mDirector, presetListAcsr)
, mPropertyGraphicalSection(mDirector, presetListAcsr)
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
            {
                mPropertyName.entry.setText(acsr.getAttr<AttrType::name>(), juce::NotificationType::dontSendNotification);
                parentHierarchyChanged();
            }
            break;
            case AttrType::key:
            case AttrType::input:
            case AttrType::description:
            case AttrType::file:
            case AttrType::results:
            case AttrType::edit:
            case AttrType::state:
            case AttrType::graphics:
            case AttrType::processing:
            case AttrType::warnings:
            case AttrType::graphicsSettings:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
            case AttrType::oscIdentifier:
            case AttrType::sendViaOsc:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::focused:
            case AttrType::grid:
            case AttrType::zoomValueMode:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::extraThresholds:
            case AttrType::hasPluginColourMap:
            case AttrType::sampleRate:
            case AttrType::zoomLogScale:
                break;
        }
    };

    mComponentListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        mProcessorSection.setEnabled(mPropertyProcessorSection.getHeight() > 0);
        mGraphicalSection.setEnabled(mPropertyGraphicalSection.getHeight() > 0);
        mOscSection.setEnabled(mPropertyOscSection.getHeight() > 0);
        mPluginSection.setEnabled(mPropertyPluginSection.getHeight() > 0);
        resized();
    };
    mComponentListener.attachTo(mProcessorSection);
    mComponentListener.attachTo(mGraphicalSection);
    mComponentListener.attachTo(mOscSection);
    mComponentListener.attachTo(mPluginSection);

    mProcessorSection.setComponents({mPropertyProcessorSection});
    mGraphicalSection.setComponents({mPropertyGraphicalSection});
    mOscSection.setComponents({mPropertyOscSection});
    mPluginSection.setComponents({mPropertyPluginSection});

    addAndMakeVisible(mPropertyName);
    addAndMakeVisible(mProcessorSection);
    addAndMakeVisible(mGraphicalSection);
    addAndMakeVisible(mOscSection);
    addAndMakeVisible(mPluginSection);

    setSize(300, 400);

    // This avoids to move the focus to the first component
    // (the name property) when recreating properties.
    setWantsKeyboardFocus(true);
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::PropertyPanel::~PropertyPanel()
{
    mComponentListener.detachFrom(mProcessorSection);
    mComponentListener.detachFrom(mGraphicalSection);
    mComponentListener.detachFrom(mOscSection);
    mComponentListener.detachFrom(mPluginSection);
    mAccessor.removeListener(mListener);
}

void Track::PropertyPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyName);
    setBounds(mProcessorSection);
    setBounds(mGraphicalSection);
    setBounds(mOscSection);
    setBounds(mPluginSection);
    setSize(getWidth(), std::max(bounds.getY(), 120) + 2);
}

void Track::PropertyPanel::parentHierarchyChanged()
{
    if(auto* window = findParentComponentOfClass<juce::DialogWindow>())
    {
        window->setName(juce::translate("ANLNAME PROPERTIES").replace("ANLNAME", mAccessor.getAttr<AttrType::name>().toUpperCase()));
    }
}

ANALYSE_FILE_END
