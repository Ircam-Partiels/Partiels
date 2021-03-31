#include "AnlTrackProgressBar.h"

ANALYSE_FILE_BEGIN

Track::ProgressBar::ProgressBar(Accessor& accessor)
: mAccessor(accessor)
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
                
                auto const isProcessing = std::get<0>(state) || std::get<2>(state);
                if(isProcessing)
                {
                    mProgressValue = static_cast<double>(std::get<0>(state) ? std::get<1>(state) : std::get<3>(state));
                    mProgressBar.setTextToDisplay(std::get<0>(state) ? "Analysing... " : "Rendering... ");
                }
                else
                {
                    mStateImage = IconManager::getIcon(warnings == WarningType::none ? IconManager::IconType::checked : IconManager::IconType::alert);
                    auto getMessage = [warnings]()
                    {
                        switch(warnings)
                        {
                            case WarningType::none:
                                return "Analysis and rendering successfully completed!";
                            case WarningType::plugin:
                                return "Analysis failed: the plugin cannot be found or allocated!";
                            case WarningType::state:
                                return "Analysis failed: the step size or the block size might not be supported!";
                        }
                        return "Analysis and rendering successfully completed!";
                    };
                    mMessage = getMessage();
                }
                mProgressBar.setVisible(isProcessing);
                
                auto getTooltip = [&, state, warnings]() -> juce::String
                {
                    if(std::get<0>(state))
                    {
                        return "analysing... (" + juce::String(static_cast<int>(std::round(std::get<1>(state) * 100.f))) + "%)";
                    }
                    else if(std::get<2>(state))
                    {
                        return "rendering... (" + juce::String(static_cast<int>(std::round(std::get<3>(state) * 100.f))) + "%)";
                    }
                    switch(warnings)
                    {
                        case WarningType::none:
                            return "analysis and rendering successfully completed!";
                        case WarningType::plugin:
                            return "analysis failed: the plugin cannot be found or allocated!";
                        case WarningType::state:
                            return "analysis failed: the step size or the block size might not be supported!";
                    }
                    return "analysis and rendering successfully completed!";
                };
                auto const tooltip = acsr.getAttr<AttrType::name>() + ": " + juce::translate(getTooltip());
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
            case AttrType::time:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::propertyState:
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
