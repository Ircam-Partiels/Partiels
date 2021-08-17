#include "AnlGroupButtonState.h"

ANALYSE_FILE_BEGIN

Group::StateButton::StateButton(Accessor& accessor)
: mAccessor(accessor)
, mLayoutNotifier(accessor, [this]()
                  {
                      updateContent();
                  })
{
    addAndMakeVisible(mProcessingButton);

    mTrackListener.onAttrChanged = [this](Track::Accessor const& acsr, Track::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Track::AttrType::processing:
            case Track::AttrType::warnings:
            case Track::AttrType::channelsLayout:
            {
                updateTooltip();
            }
            break;
            case Track::AttrType::name:
            case Track::AttrType::key:
            case Track::AttrType::file:
            case Track::AttrType::description:
            case Track::AttrType::state:
            case Track::AttrType::results:
            case Track::AttrType::graphics:
            case Track::AttrType::identifier:
            case Track::AttrType::height:
            case Track::AttrType::colours:
            case Track::AttrType::zoomLink:
            case Track::AttrType::zoomAcsr:
            case Track::AttrType::focused:
            case Track::AttrType::grid:
                break;
        }
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::name:
            {
                updateTooltip();
            }
            break;
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::focused:
            case AttrType::layout:
            case AttrType::tracks:
                break;
        }
    };

    mProcessingButton.setActive(false);
    mProcessingButton.setInactiveImage(IconManager::getIcon(IconManager::IconType::verified));
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::StateButton::~StateButton()
{
    mAccessor.removeListener(mListener);
    for(auto& trackAcsr : mTrackAccessors.getContents())
    {
        trackAcsr.second.get().removeListener(mTrackListener);
    }
}

void Group::StateButton::resized()
{
    mProcessingButton.setBounds(getLocalBounds());
}

void Group::StateButton::updateContent()
{
    mTrackAccessors.updateContents(
        mAccessor,
        [this](Track::Accessor& trackAccessor)
        {
            trackAccessor.addListener(mTrackListener, NotificationType::synchronous);
            return std::ref(trackAccessor);
        },
        [this](std::reference_wrapper<Track::Accessor>& content)
        {
            content.get().removeListener(mTrackListener);
        });
    updateTooltip();
}

void Group::StateButton::updateTooltip()
{
    auto const& name = mAccessor.getAttr<AttrType::name>();
    auto const& contents = mTrackAccessors.getContents();
    auto const valid = std::all_of(contents.cbegin(), contents.cend(), [](auto const& pair)
                                   {
                                       return pair.second.get().template getAttr<Track::AttrType::warnings>() == Track::WarningType::none;
                                   });

    auto const processing = std::any_of(contents.cbegin(), contents.cend(), [](auto const& pair)
                                        {
                                            return std::get<0>(pair.second.get().template getAttr<Track::AttrType::processing>());
                                        });
    auto const rendering = std::any_of(contents.cbegin(), contents.cend(), [](auto const& pair)
                                       {
                                           return std::get<2>(pair.second.get().template getAttr<Track::AttrType::processing>());
                                       });
    if(processing && rendering)
    {
        setTooltip(name + ": processing and rendering...");
        mProcessingButton.setTooltip(name + ": processing and rendering...");
    }
    else if(processing)
    {
        setTooltip(name + ": processing...");
        mProcessingButton.setTooltip(name + ": processing...");
    }
    else if(rendering)
    {
        setTooltip(name + ": rendering...");
        mProcessingButton.setTooltip(name + ": rendering...");
    }
    else if(valid)
    {
        setTooltip(name + ": analyses and renderings successfully completed!");
        mProcessingButton.setTooltip(name + ": analyses and renderings successfully completed!");
    }
    else
    {
        setTooltip(name + ": analyses and renderings finished with errors!");
        mProcessingButton.setTooltip(name + ":analyses and renderings finished with errors!");
    }

    mProcessingButton.setActive(processing || rendering);
    mProcessingButton.setInactiveImage(IconManager::getIcon(valid ? IconManager::IconType::verified : IconManager::IconType::alert));
}

ANALYSE_FILE_END
