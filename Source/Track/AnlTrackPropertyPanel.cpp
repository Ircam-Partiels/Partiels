#include "AnlTrackPropertyPanel.h"
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
{
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
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
            case AttrType::state:
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

    mComponentListener.onComponentResized = [&](juce::Component& component)
    {
        juce::ignoreUnused(component);
        resized();
    };
    mComponentListener.attachTo(mProcessorSection);
    mComponentListener.attachTo(mGraphicalSection);
    mComponentListener.attachTo(mPluginSection);

    mProcessorSection.setComponents({mPropertyProcessorSection});
    mGraphicalSection.setComponents({mPropertyGraphicalSection});
    mPluginSection.setComponents({mPropertyPluginSection});

    addAndMakeVisible(mPropertyName);
    addAndMakeVisible(mProcessorSection);
    addAndMakeVisible(mGraphicalSection);
    addAndMakeVisible(mPluginSection);

    setSize(300, 400);
    mViewport.setSize(getWidth() + mViewport.getVerticalScrollBar().getWidth() + 2, 400);
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
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::PropertyPanel::~PropertyPanel()
{
    mComponentListener.detachFrom(mProcessorSection);
    mComponentListener.detachFrom(mGraphicalSection);
    mComponentListener.detachFrom(mPluginSection);
    mAccessor.removeListener(mListener);
}

void Track::PropertyPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyName);
    setBounds(mProcessorSection);
    setBounds(mGraphicalSection);
    setBounds(mPluginSection);
    setSize(getWidth(), std::max(bounds.getY(), 120) + 2);
}

ANALYSE_FILE_END
