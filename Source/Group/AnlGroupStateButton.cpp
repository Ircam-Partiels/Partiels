#include "AnlGroupStateButton.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Group::StateButton::StateButton(Director& director)
: mDirector(director)
, mStateIcon(juce::ImageCache::getFromMemory(AnlIconsData::alert_png, AnlIconsData::alert_pngSize))
, mLayoutNotifier(mAccessor, [this]()
                  {
                      updateTooltip();
                  },
                  {Track::AttrType::processing, Track::AttrType::warnings, Track::AttrType::channelsLayout})
{
    addAndMakeVisible(mProcessingButton);
    mStateIcon.setWantsKeyboardFocus(false);
    addAndMakeVisible(mStateIcon);

    mStateIcon.onClick = [this]()
    {
        auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
        auto const numTrackWarnings = std::count_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                                    {
                                                        return trackAcsr.get().template getAttr<Track::AttrType::warnings>() != Track::WarningType::none;
                                                    });
        if(numTrackWarnings == 1)
        {
            auto it = std::find_if(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                   {
                                       return trackAcsr.get().template getAttr<Track::AttrType::warnings>() != Track::WarningType::none;
                                   });
            MiscWeakAssert(it != trackAcsrs.cend());
            if(it != trackAcsrs.cend())
            {
                mDirector.getTrackDirector(it->get().getAttr<Track::AttrType::identifier>()).askToResolveWarnings();
            }
        }
        else if(numTrackWarnings > 1)
        {
            juce::PopupMenu menu;
            juce::WeakReference<juce::Component> weakReference(this);
            for(auto const& trackAcsr : trackAcsrs)
            {
                auto const identifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
                if(trackAcsr.get().getAttr<Track::AttrType::warnings>() != Track::WarningType::none)
                {
                    auto const name = trackAcsr.get().getAttr<Track::AttrType::name>();
                    menu.addItem(juce::translate("Resolve: ") + name, [=, this]()
                                 {
                                     if(weakReference.get() == nullptr)
                                     {
                                         return;
                                     }
                                     if(!Tools::hasTrackAcsr(mAccessor, identifier))
                                     {
                                         return;
                                     }
                                     mDirector.getTrackDirector(identifier).askToResolveWarnings();
                                 });
                }
            }
            menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(*this));
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
            case AttrType::zoomid:
                break;
        }
    };

    mProcessingButton.setActive(false);
    mProcessingButton.setInactiveImage(juce::ImageCache::getFromMemory(AnlIconsData::checked_png, AnlIconsData::checked_pngSize));
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::StateButton::~StateButton()
{
    mAccessor.removeListener(mListener);
}

void Group::StateButton::resized()
{
    mProcessingButton.setBounds(getLocalBounds());
    mStateIcon.setBounds(getLocalBounds());
}

void Group::StateButton::updateTooltip()
{
    auto const& name = mAccessor.getAttr<AttrType::name>();
    auto const trackAcsrs = Tools::getTrackAcsrs(mAccessor);
    auto const valid = std::all_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                   {
                                       return trackAcsr.get().template getAttr<Track::AttrType::warnings>() == Track::WarningType::none;
                                   });

    auto const processing = std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                        {
                                            return std::get<0>(trackAcsr.get().template getAttr<Track::AttrType::processing>());
                                        });
    auto const rendering = std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [](auto const& trackAcsr)
                                       {
                                           return std::get<2>(trackAcsr.get().template getAttr<Track::AttrType::processing>());
                                       });

    auto getTooltip = [&]()
    {
        if(processing && rendering)
        {
            return juce::translate("Processing and rendering...");
        }
        else if(processing)
        {
            return juce::translate("Processing...");
        }
        else if(rendering)
        {
            return juce::translate("Rendering...");
        }
        else if(valid)
        {
            return juce::translate("Analyses and renderings completed successfully!");
        }
        else
        {
            return juce::translate("Analyses and renderings completed with errors!");
        }
    };

    auto const tooltip = name + ": " + getTooltip();
    setTooltip(tooltip);
    mProcessingButton.setTooltip(tooltip);
    mStateIcon.setTooltip(tooltip);

    auto const changed = mHasWarning != !valid || (processing || rendering) != mProcessingButton.isActive();
    mHasWarning = !valid;
    mProcessingButton.setActive(processing || rendering);
    auto const checkedImage = juce::ImageCache::getFromMemory(AnlIconsData::checked_png, AnlIconsData::checked_pngSize);
    auto const alertImage = juce::ImageCache::getFromMemory(AnlIconsData::alert_png, AnlIconsData::alert_pngSize);
    mProcessingButton.setInactiveImage(valid ? checkedImage : alertImage);
    mStateIcon.setImages(!mHasWarning ? checkedImage : alertImage);
    mStateIcon.setEnabled(mHasWarning);
    mStateIcon.setVisible(!isProcessingOrRendering());
    if(changed && onStateChanged != nullptr)
    {
        onStateChanged();
    }
}

bool Group::StateButton::isProcessingOrRendering() const
{
    return mProcessingButton.isActive();
}

bool Group::StateButton::hasWarning() const
{
    return mHasWarning;
}

ANALYSE_FILE_END
