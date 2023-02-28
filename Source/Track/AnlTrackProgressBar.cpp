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
        if(mDirector.isFileModified())
        {
            mDirector.askToRemoveFile();
        }
        else
        {
            mDirector.askToResolveWarnings();
        }
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
                auto const includeAnalysis = mMode == Mode::analysis || mMode == Mode::both;
                auto const includeRendering = mMode == Mode::rendering || mMode == Mode::both;
                mMessage = Tools::getStateTootip(acsr, includeAnalysis, includeRendering);
                if(std::get<0>(state))
                {
                    mProgressBar.setVisible(includeAnalysis);
                    mProgressValue = static_cast<double>(std::get<1>(state));
                    mStateIcon.setVisible(false);
                }
                else if(std::get<2>(state) && includeRendering)
                {
                    mProgressBar.setVisible(true);
                    mProgressValue = static_cast<double>(std::get<3>(state));
                    mStateIcon.setVisible(false);
                }
                else if(warnings != WarningType::none)
                {
                    mProgressBar.setVisible(false);
                    mStateIcon.setVisible(true);
                    mStateIcon.setTypes(Icon::Type::alert);
                    mStateIcon.setEnabled(true);
                }
                else if(mDirector.isFileModified())
                {
                    mProgressBar.setVisible(false);
                    mStateIcon.setVisible(true);
                    mStateIcon.setTypes(Icon::Type::alert);
                    mStateIcon.setEnabled(true);
                    mMessage = juce::translate("Analysis results have been edited!");
                }
                else
                {
                    mProgressBar.setVisible(false);
                    mStateIcon.setVisible(true);
                    mStateIcon.setTypes(warnings != WarningType::none ? Icon::Type::alert : Icon::Type::verified);
                    mStateIcon.setEnabled(warnings != WarningType::none);
                }
                mProgressBar.setTextToDisplay(mMessage);
                auto const tooltip = acsr.getAttr<AttrType::name>() + ": " + mMessage;
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
            case AttrType::font:
            case AttrType::unit:
            case AttrType::channelsLayout:
            case AttrType::showInGroup:
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
    auto const localBounds = getLocalBounds().reduced(2);
    auto const halfHeight = localBounds.getHeight() / 2;
    mProgressBar.setBounds(localBounds.withHeight(halfHeight));
    mStateIcon.setBounds(localBounds.withWidth(halfHeight).withSizeKeepingCentre(halfHeight, halfHeight));
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
