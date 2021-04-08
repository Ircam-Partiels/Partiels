#include "AnlDocumentGroupButtonState.h"
#include "AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

Document::GroupStateButton::GroupStateButton(Accessor& accessor)
: mAccessor(accessor)
{
    addAndMakeVisible(mProcessingButton);
    auto updateLayout = [&]()
    {
        auto const& layout = mAccessor.getAttr<AttrType::layout>();
        auto it = mTrackAccessors.begin();
        while(it != mTrackAccessors.end())
        {
            if(!std::binary_search(layout.cbegin(), layout.cend(), it->first) || !Tools::hasTrack(mAccessor, it->first))
            {
                it->second.get().removeListener(mTrackListener);
                it = mTrackAccessors.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for(auto const& identifier : layout)
        {
            auto acsrIt = mTrackAccessors.find(identifier);
            if(acsrIt == mTrackAccessors.cend())
            {
                auto trackAcsr = Tools::getTrack(mAccessor, identifier);
                if(trackAcsr)
                {
                    trackAcsr->get().addListener(mTrackListener, NotificationType::synchronous);
                    mTrackAccessors.emplace(identifier, *trackAcsr);
                }
            }
        }
    };
    
    mListener.onAttrChanged = [=](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::file:
            case AttrType::layoutHorizontal:
            case AttrType::layoutVertical:
            case AttrType::expanded:
                break;
            case AttrType::layout:
            {
                updateLayout();
            }
                break;
        }
    };
    
    mListener.onAccessorInserted = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
            case AcsrType::tracks:
            {
                updateLayout();
            }
                break;
        }
    };
    
    mListener.onAccessorErased = [=](Accessor const& acsr, AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, index);
        switch(type)
        {
            case AcsrType::timeZoom:
            case AcsrType::transport:
                break;
            case AcsrType::tracks:
            {
                updateLayout();
            }
                break;
        }
    };
    
    mTrackListener.onAttrChanged = [&](Track::Accessor const& acsr, Track::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Track::AttrType::name:
            case Track::AttrType::processing:
            case Track::AttrType::warnings:
            {
                auto const valid = std::all_of(mTrackAccessors.cbegin(), mTrackAccessors.cend(), [](auto const& pair)
                {
                    return pair.second.get().template getAttr<Track::AttrType::warnings>() == Track::WarningType::none;
                });
                
                auto const processing = std::any_of(mTrackAccessors.cbegin(), mTrackAccessors.cend(), [](auto const& pair)
                {
                    return std::get<0>(pair.second.get().template getAttr<Track::AttrType::processing>());
                });
                auto const rendering = std::any_of(mTrackAccessors.cbegin(), mTrackAccessors.cend(), [](auto const& pair)
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
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::GroupStateButton::~GroupStateButton()
{
    mAccessor.removeListener(mListener);
    for(auto& trackAcsrs : mTrackAccessors)
    {
        trackAcsrs.second.get().removeListener(mTrackListener);
    }
}

void Document::GroupStateButton::resized()
{
    mProcessingButton.setBounds(getLocalBounds());
}

ANALYSE_FILE_END
