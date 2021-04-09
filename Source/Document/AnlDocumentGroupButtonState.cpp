#include "AnlDocumentGroupButtonState.h"

ANALYSE_FILE_BEGIN

Document::GroupStateButton::GroupStateButton(Accessor& accessor)
: GroupContainer<std::reference_wrapper<Track::Accessor>>(accessor)
{
    addAndMakeVisible(mProcessingButton);
    
    mTrackListener.onAttrChanged = [&](Track::Accessor const& acsr, Track::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Track::AttrType::name:
            case Track::AttrType::processing:
            case Track::AttrType::warnings:
            {
                auto const& group = getGroupContent();
                auto const valid = std::all_of(group.cbegin(), group.cend(), [](auto const& pair)
                {
                    return pair.second.get().template getAttr<Track::AttrType::warnings>() == Track::WarningType::none;
                });
                
                auto const processing = std::any_of(group.cbegin(), group.cend(), [](auto const& pair)
                {
                    return std::get<0>(pair.second.get().template getAttr<Track::AttrType::processing>());
                });
                auto const rendering = std::any_of(group.cbegin(), group.cend(), [](auto const& pair)
                {
                    return std::get<2>(pair.second.get().template getAttr<Track::AttrType::processing>());
                });
                if(processing && rendering)
                {
                    setTooltip("Overview: processing and rendering...");
                    mProcessingButton.setTooltip("Overview: processing and rendering...");
                }
                else if(processing)
                {
                    setTooltip("Overview: processing...");
                    mProcessingButton.setTooltip("Overview: processing...");
                }
                else if(rendering)
                {
                    setTooltip("Overview: rendering...");
                    mProcessingButton.setTooltip("Overview: rendering...");
                }
                else if(valid)
                {
                    setTooltip("Overview: analyses and renderings successfully completed!");
                    mProcessingButton.setTooltip("Overview: analyses and renderings successfully completed!");
                }
                else
                {
                    setTooltip("Overview: analyses and renderings finished with errors!");
                    mProcessingButton.setTooltip("Overview: analyses and renderings finished with errors!");
                }
                
                mProcessingButton.setActive(processing || rendering);
                mProcessingButton.setInactiveImage(IconManager::getIcon(valid ? IconManager::IconType::checked : IconManager::IconType::alert));
            }
                break;
            case Track::AttrType::key:
            case Track::AttrType::description:
            case Track::AttrType::state:
            case Track::AttrType::results:
            case Track::AttrType::graphics:
            case Track::AttrType::time:
            case Track::AttrType::identifier:
            case Track::AttrType::height:
            case Track::AttrType::colours:
            case Track::AttrType::propertyState:
            case Track::AttrType::zoomLink:
                break;
        }
    };
}

Document::GroupStateButton::~GroupStateButton()
{
    for(auto& trackAcsr : getGroupContent())
    {
        trackAcsr.second.get().removeListener(mTrackListener);
    }
}

void Document::GroupStateButton::resized()
{
    mProcessingButton.setBounds(getLocalBounds());
}

void Document::GroupStateButton::removeFromGroup(std::reference_wrapper<Track::Accessor>& value)
{
    value.get().removeListener(mTrackListener);
}

std::reference_wrapper<Track::Accessor> Document::GroupStateButton::createForGroup(Track::Accessor& trackAccessor)
{
    trackAccessor.addListener(mTrackListener, NotificationType::synchronous);
    return trackAccessor;
}

ANALYSE_FILE_END
