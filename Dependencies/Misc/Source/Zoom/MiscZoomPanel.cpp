#include "MiscZoomPanel.h"

MISC_FILE_BEGIN

Zoom::Panel::Panel(Zoom::Accessor& accessor, juce::String const& name, juce::String const& unit)
: mAccessor(accessor)
, mName("Name", name)
, mPropertyStart(juce::translate("Low"), juce::translate("The low value of the RANGENAME range").replace("RANGENAME", name.toLowerCase()), unit, {0.0f, 1.0f}, 0.0f, [&](float value)
                 {
                     auto const range = mAccessor.getAttr<Zoom::AttrType::visibleRange>().withStart(static_cast<double>(value));
                     mAccessor.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
                 })
, mPropertyEnd(juce::translate("High"), juce::translate("The high value of the visible range").replace("RANGENAME", name.toLowerCase()), unit, {0.0f, 1.0f}, 0.0f, [&](float value)
               {
                   auto const range = mAccessor.getAttr<Zoom::AttrType::visibleRange>().withEnd(static_cast<double>(value));
                   mAccessor.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
               })
{
    setWantsKeyboardFocus(true);
    addChildComponent(mName);
    mName.setVisible(mName.getText().isNotEmpty());
    addAndMakeVisible(mPropertyStart);
    addAndMakeVisible(mPropertyEnd);
    mListener.onAttrChanged = [this](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::globalRange:
            {
                auto const range = acsr.getAttr<Zoom::AttrType::globalRange>();
                mPropertyStart.entry.setRange(range, 0.0, juce::NotificationType::dontSendNotification);
                mPropertyEnd.entry.setRange(range, 0.0, juce::NotificationType::dontSendNotification);
            }
            break;
            case Zoom::AttrType::visibleRange:
            {
                auto const range = acsr.getAttr<Zoom::AttrType::visibleRange>();
                mPropertyStart.entry.setValue(range.getStart(), juce::NotificationType::dontSendNotification);
                mPropertyEnd.entry.setValue(range.getEnd(), juce::NotificationType::dontSendNotification);
            }
            break;
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(180, 74);
}

Zoom::Panel::~Panel()
{
    mAccessor.removeListener(mListener);
}

void Zoom::Panel::resized()
{
    auto bounds = getLocalBounds();
    if(mName.isVisible())
    {
        mName.setBounds(bounds.removeFromTop(24));
    }
    mPropertyEnd.setBounds(bounds.removeFromTop(mPropertyEnd.getHeight()));
    mPropertyStart.setBounds(bounds.removeFromTop(mPropertyStart.getHeight()));
    setSize(getWidth(), bounds.getY() + 2);
}

Zoom::TimePanel::TimePanel(Accessor& accessor, juce::String const& name)
: mAccessor(accessor)
, mName("Name", name)
, mPropertyStart(juce::translate("Start"), juce::translate("The start of the RANGENAME range").replace("RANGENAME", name.toLowerCase()), [&](double time)
                 {
                     auto const range = mAccessor.getAttr<AttrType::visibleRange>().withStart(time);
                     mAccessor.setAttr<AttrType::visibleRange>(range, NotificationType::synchronous);
                 })
, mPropertyEnd(juce::translate("End"), juce::translate("The end of the RANGENAME range").replace("RANGENAME", name.toLowerCase()), [&](double time)
               {
                   auto const range = mAccessor.getAttr<AttrType::visibleRange>().withEnd(time);
                   mAccessor.setAttr<AttrType::visibleRange>(range, NotificationType::synchronous);
               })
{
    setWantsKeyboardFocus(true);
    addChildComponent(mName);
    mName.setVisible(mName.getText().isNotEmpty());
    addAndMakeVisible(mPropertyStart);
    addAndMakeVisible(mPropertyEnd);
    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::globalRange:
            {
                auto const range = acsr.getAttr<AttrType::globalRange>();
                mPropertyStart.entry.setRange(range, juce::NotificationType::dontSendNotification);
                mPropertyEnd.entry.setRange(range, juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::visibleRange:
            {
                auto const range = acsr.getAttr<AttrType::visibleRange>();
                mPropertyStart.entry.setTime(range.getStart(), juce::NotificationType::dontSendNotification);
                mPropertyEnd.entry.setTime(range.getEnd(), juce::NotificationType::dontSendNotification);
            }
            break;
            case AttrType::minimumLength:
            case AttrType::anchor:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(180, 74);
}

Zoom::TimePanel::~TimePanel()
{
    mAccessor.removeListener(mListener);
}

void Zoom::TimePanel::resized()
{
    auto bounds = getLocalBounds();
    if(mName.isVisible())
    {
        mName.setBounds(bounds.removeFromTop(24));
    }
    mPropertyStart.setBounds(bounds.removeFromTop(mPropertyStart.getHeight()));
    mPropertyEnd.setBounds(bounds.removeFromTop(mPropertyEnd.getHeight()));
    setSize(getWidth(), bounds.getY() + 2);
}

MISC_FILE_END
