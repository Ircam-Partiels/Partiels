#include "AnlGroupEditor.h"
#include "../Track/AnlTrackTools.h"
#include "../Track/AnlTrackTooltip.h"

ANALYSE_FILE_BEGIN

Group::Editor::Editor(Director& director, Zoom::Accessor& timeZoomAccessor, Transport::Accessor& transportAccessor, juce::ApplicationCommandManager& commandManager, juce::Component& content)
: mContent(content)
, mDirector(director)
, mAccessor(director.getAccessor())
, mTimeZoomAccessor(timeZoomAccessor)
, mTransportAccessor(transportAccessor)
, mCommandManager(commandManager)
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateContent();
                  },
                  {Track::AttrType::identifier, Track::AttrType::showInGroup})
{
    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::expanded:
            case AttrType::layout:
            case AttrType::tracks:
                break;
            case AttrType::referenceid:
            {
                updateContent();
                break;
            }
            case AttrType::focused:
            {
                colourChanged();
                break;
            }
            case AttrType::colour:
            {
                repaint();
                updateEditorNameAndColour();
                break;
            }
            case AttrType::name:
            {
                updateEditorNameAndColour();
                break;
            }
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Editor::~Editor()
{
    mAccessor.removeListener(mListener);
}

void Group::Editor::resized()
{
    if(mTrackEditor != nullptr)
    {
        mTrackEditor->setBounds(getLocalBounds());
    }
}

void Group::Editor::paint(juce::Graphics& g)
{
    g.fillAll(mAccessor.getAttr<AttrType::colour>());
}

void Group::Editor::updateContent()
{
    auto const trackAcsr = Tools::getReferenceTrackAcsr(mAccessor);
    if(!trackAcsr.has_value())
    {
        mTrackEditor.reset();
    }
    else if(mTrackIdentifier != trackAcsr.value().get().getAttr<Track::AttrType::identifier>())
    {
        mTrackIdentifier = trackAcsr.value().get().getAttr<Track::AttrType::identifier>();
        mTrackEditor = std::make_unique<Track::Editor>(
            mDirector.getTrackDirector(mTrackIdentifier), mTimeZoomAccessor, mTransportAccessor, mCommandManager, mContent, [this](juce::Point<int> const& pt)
            {
                return getBubbleTooltip(pt);
            },
            false);
        if(mTrackEditor != nullptr)
        {
            mTrackEditor->onMouseDown = [this](juce::MouseEvent const& event)
            {
                if(!event.mods.isPopupMenu())
                {
                    return;
                }
                showPopupMenu();
            };
        }
        addAndMakeVisible(mTrackEditor.get());
    }
    updateEditorNameAndColour();
    resized();
}

void Group::Editor::updateEditorNameAndColour()
{
    if(mTrackEditor != nullptr)
    {
        mTrackEditor->setSnapshotNameAndColour(mAccessor.getAttr<AttrType::name>(), mAccessor.getAttr<AttrType::colour>());
    }
}

void Group::Editor::showPopupMenu()
{
    juce::PopupMenu trackRefenceMenu;
    auto const referenceId = mAccessor.getAttr<AttrType::referenceid>();
    trackRefenceMenu.addItem(juce::translate("Front"), !referenceId.isEmpty(), referenceId.isEmpty(), [this]()
                             {
                                 mAccessor.setAttr<AttrType::referenceid>(juce::String(), NotificationType::synchronous);
                             });
    auto const layout = mAccessor.getAttr<AttrType::layout>();
    for(auto layoutIndex = 0_z; layoutIndex < layout.size(); ++layoutIndex)
    {
        auto const trackId = layout.at(layoutIndex);
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, trackId);
        if(trackAcsr.has_value())
        {
            auto const& trackName = trackAcsr->get().getAttr<Track::AttrType::name>();
            auto const itemLabel = trackName.isEmpty() ? juce::translate("Track IDX").replace("IDX", juce::String(layoutIndex + 1)) : trackName;
            trackRefenceMenu.addItem(itemLabel, trackId != referenceId, trackId == referenceId, [=, this]()
                                     {
                                         mAccessor.setAttr<AttrType::referenceid>(trackId, NotificationType::synchronous);
                                     });
        }
    }
    juce::PopupMenu menu;
    menu.addSubMenu(juce::translate("Track Reference"), trackRefenceMenu);
    menu.showMenuAsync(juce::PopupMenu::Options().withDeletionCheck(*this).withMousePosition());
}

juce::String Group::Editor::getBubbleTooltip(juce::Point<int> const& pt)
{
    if(!getLocalBounds().contains(pt))
    {
        return "";
    }
    juce::StringArray lines;
    auto const time = Zoom::Tools::getScaledValueFromWidth(mTimeZoomAccessor, *this, pt.x);
    lines.add(juce::translate("Time: TIME").replace("TIME", Format::secondsToString(time)));
    auto const referenceTrackAcsr = Tools::getReferenceTrackAcsr(mAccessor);
    if(referenceTrackAcsr.has_value() && Track::Tools::hasVerticalZoom(referenceTrackAcsr.value()))
    {
        lines.add(juce::translate("Mouse: VALUE").replace("VALUE", Track::Tools::getZoomTootip(referenceTrackAcsr.value().get(), *this, pt.y)));
    }
    auto const& layout = mAccessor.getAttr<AttrType::layout>();
    for(auto const& identifier : layout)
    {
        auto const trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
        if(trackAcsr.has_value() && trackAcsr->get().getAttr<Track::AttrType::showInGroup>())
        {
            lines.add(trackAcsr->get().getAttr<Track::AttrType::name>() + ":");
            for(auto const& line : Track::Tools::getValueTootip(trackAcsr->get(), mTimeZoomAccessor, *this, pt.y, time))
            {
                lines.addArray("\t" + line);
            }
        }
    }
    return lines.joinIntoString("\n");
}

ANALYSE_FILE_END