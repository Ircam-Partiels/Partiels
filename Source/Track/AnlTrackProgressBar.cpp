#include "AnlTrackProgressBar.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::ProgressBar::ProgressBar(Director& director, Mode mode)
: mDirector(director)
, mMode(mode)
{
    addChildComponent(mProgressBar);
    addAndMakeVisible(mStateIcon);
    mStateIcon.setWantsKeyboardFocus(false);
    addMouseListener(&mStateIcon, false);
    mStateIcon.onClick = [this]()
    {
        mDirector.askToResolveWarnings();
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
            case AttrType::file:
            case AttrType::processing:
            case AttrType::warnings:
            case AttrType::results:
            {
                auto const state = acsr.getAttr<AttrType::processing>();
                auto const warnings = acsr.getAttr<AttrType::warnings>();
                auto const isLoading = acsr.getAttr<AttrType::file>().file != juce::File{};
                if(std::get<0>(state))
                {
                    mMessage = isLoading ? "Loading... " : "Analysing... ";
                    mProgressBar.setVisible(mMode == Mode::analysis || mMode == Mode::both);
                    mProgressValue = static_cast<double>(std::get<1>(state));
                    mProgressBar.setTextToDisplay(mMessage);
                    mStateIcon.setVisible(false);
                }
                else if(std::get<2>(state) && (mMode == Mode::rendering || mMode == Mode::both))
                {
                    mProgressBar.setVisible(true);
                    mProgressValue = static_cast<double>(std::get<3>(state));
                    mProgressBar.setTextToDisplay("Rendering... ");
                    mStateIcon.setVisible(false);
                }
                else
                {
                    mProgressBar.setVisible(false);
                    mStateIcon.setVisible(true);
                    mStateIcon.setTypes(warnings == WarningType::none ? Icon::Type::verified : Icon::Type::alert);
                    mStateIcon.setEnabled(warnings != WarningType::none);
                    auto getMessage = [&, warnings]()
                    {
                        switch(warnings)
                        {
                            case WarningType::none:
                                break;
                            case WarningType::library:
                                return juce::translate("The library cannot be found or loaded!");
                            case WarningType::plugin:
                                return juce::translate("The plugin cannot be allocated!");
                            case WarningType::state:
                                return juce::translate("The parameters are invalid!");
                            case WarningType::file:
                                return juce::translate("The file cannot be parsed!");
                        }
                        switch(mMode)
                        {
                            case Mode::analysis:
                                return isLoading ? juce::translate("Loading completed successfully!") : juce::translate("Analysis completed successfully!");
                            case Mode::rendering:
                                return juce::translate("Rendering completed successfully!");
                            case Mode::both:
                                return isLoading ? juce::translate("Loading and rendering completed successfully!") : juce::translate("Analysis and rendering completed successfully!");
                        }
                        return juce::translate("Loading and rendering completed successfully!");
                    };
                    mMessage = getMessage();
                }

                auto const tooltip = Tools::getStateTootip(acsr);
                mProgressBar.setTooltip(tooltip);
                mStateIcon.setTooltip(tooltip);
                setTooltip(tooltip);

                repaint();
            }
            break;
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::graphics:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::channelsLayout:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::focused:
            case AttrType::grid:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::ProgressBar::~ProgressBar()
{
    removeMouseListener(&mStateIcon);
    mAccessor.removeListener(mListener);
}

void Track::ProgressBar::resized()
{
    auto localBounds = getLocalBounds().reduced(2);
    auto const halfHeight = localBounds.getHeight() / 2;
    if(mProgressBar.isVisible())
    {
        mProgressBar.setBounds(localBounds.removeFromTop(halfHeight));
    }
    mStateIcon.setBounds(localBounds.removeFromLeft(halfHeight).withSizeKeepingCentre(halfHeight, halfHeight));
}

void Track::ProgressBar::paint(juce::Graphics& g)
{
    if(!mProgressBar.isVisible())
    {
        auto localBounds = getLocalBounds().reduced(2);
        auto const halfHeight = localBounds.getHeight() / 2;
        g.setColour(findColour(juce::Label::ColourIds::textColourId));
        g.setFont(juce::Font(static_cast<float>(halfHeight)));
        g.drawFittedText(mMessage, localBounds.withTrimmedLeft(4 + (mStateIcon.isVisible() ? halfHeight : 0)), juce::Justification::left, 2);
    }
}

ANALYSE_FILE_END
