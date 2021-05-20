#include "AnlTrackProgressBar.h"
#include "AnlTrackTools.h"

ANALYSE_FILE_BEGIN

Track::ProgressBar::ProgressBar(Accessor& accessor, Mode mode)
: mAccessor(accessor)
, mMode(mode)
{
    addChildComponent(mProgressBar);

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
            case AttrType::processing:
            case AttrType::warnings:
            {
                auto const state = acsr.getAttr<AttrType::processing>();
                auto const warnings = acsr.getAttr<AttrType::warnings>();

                if(std::get<0>(state))
                {
                    if(mMode == Mode::analysis || mMode == Mode::both)
                    {
                        mProgressBar.setVisible(true);
                        mProgressValue = static_cast<double>(std::get<1>(state));
                        mProgressBar.setTextToDisplay("Analysing... ");
                    }
                    else
                    {
                        mProgressBar.setVisible(false);
                        mMessage = "Analysing... ";
                    }
                }
                else if(std::get<2>(state) && (mMode == Mode::rendering || mMode == Mode::both))
                {
                    mProgressBar.setVisible(true);
                    mProgressValue = static_cast<double>(std::get<3>(state));
                    mProgressBar.setTextToDisplay("Rendering... ");
                }
                else
                {
                    mProgressBar.setVisible(false);
                    mStateImage = IconManager::getIcon(warnings == WarningType::none ? IconManager::IconType::checked : IconManager::IconType::alert);
                    auto getMessage = [&, warnings]()
                    {
                        switch(warnings)
                        {
                            case WarningType::none:
                                break;
                            case WarningType::plugin:
                                return "Analysis failed: the plugin cannot be found or allocated!";
                            case WarningType::state:
                                return "Analysis failed: the step size or the block size might not be supported!";
                        }
                        switch(mMode)
                        {
                            case Mode::analysis:
                                return "Analysis successfully completed!";
                            case Mode::rendering:
                                return "Rendering successfully completed!";
                            case Mode::both:
                                return "Analysis and rendering successfully completed!";
                        }
                        return "Analysis and rendering successfully completed!";
                    };

                    mMessage = getMessage();
                }

                auto const tooltip = Tools::getStateTootip(acsr);
                mProgressBar.setTooltip(tooltip);
                setTooltip(tooltip);

                repaint();
            }
            break;
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::zoomLink:
            case AttrType::zoomAcsr:
            case AttrType::focused:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::ProgressBar::~ProgressBar()
{
    mAccessor.removeListener(mListener);
}

void Track::ProgressBar::resized()
{
    auto localBounds = getLocalBounds().reduced(2);
    auto const halfHeight = localBounds.getHeight() / 2;
    mProgressBar.setBounds(localBounds.removeFromTop(halfHeight));
}

void Track::ProgressBar::paint(juce::Graphics& g)
{
    if(!mProgressBar.isVisible())
    {
        auto localBounds = getLocalBounds().reduced(2);
        auto const halfHeight = localBounds.getHeight() / 2;
        g.setColour(findColour(juce::Label::ColourIds::textColourId));
        juce::RectanglePlacement const placement(juce::RectanglePlacement::Flags::stretchToFit);
        if(mStateImage.isValid())
        {
            auto const imageBounds = localBounds.removeFromLeft(halfHeight).withSizeKeepingCentre(halfHeight, halfHeight).toFloat();
            g.drawImageTransformed(mStateImage, placement.getTransformToFit(mStateImage.getBounds().toFloat(), imageBounds), true);
        }
        g.setFont(juce::Font(static_cast<float>(halfHeight)));
        g.drawFittedText(mMessage, localBounds.withTrimmedLeft(4), juce::Justification::left, 2);
    }
}

ANALYSE_FILE_END
