#include "AnlApplicationAbout.h"
#include "AnlApplicationInstance.h"
#include <IconsData.h>
#include <MiscData.h>

ANALYSE_FILE_BEGIN

Application::About::About()
: mImage(juce::Drawable::createFromImageData(IconsData::icon_png, IconsData::icon_pngSize))
{
    addAndMakeVisible(mPartielsLink);
    mPartielsLink.setBounds(2, 2, 44, 20);
    addAndMakeVisible(mIrcamLink);
    mIrcamLink.setBounds(78, 130, 44, 20);
    addAndMakeVisible(mVampLink);
    mVampLink.setBounds(152, 162, 42, 20);
    setSize(400, 400);
}

void Application::About::paint(juce::Graphics& g)
{
    if(mImage != nullptr)
    {
        mImage->drawWithin(g, {150.0f, 140.0f, 240.0f, 280.0f}, juce::RectanglePlacement::onlyReduceInSize | juce::RectanglePlacement::centred, 1.0f);
    }
    
    g.setColour(findColour(juce::Label::ColourIds::textColourId, true));
    g.setFont(16.0f);
    auto const font = g.getCurrentFont();
    mPartielsLink.setFont(font, false);
    mIrcamLink.setFont(font, false);
    mVampLink.setFont(font, false);
    juce::String const text(MiscData::About_txt, MiscData::About_txtSize);
    g.drawFittedText(text, getLocalBounds().reduced(4), juce::Justification::left, 40);
}

void Application::About::show()
{
    if(mFloatingWindow.getContentComponent() == nullptr)
    {
        auto const& desktop = juce::Desktop::getInstance();
        auto const mousePosition = desktop.getMainMouseSource().getScreenPosition().toInt();
        auto const* display = desktop.getDisplays().getDisplayForPoint(mousePosition);
        anlStrongAssert(display != nullptr);
        if(display != nullptr)
        {
            auto const bounds = display->userArea.withSizeKeepingCentre(mFloatingWindow.getWidth(), mFloatingWindow.getHeight());
            mFloatingWindow.setBounds(bounds);
        }
        mFloatingWindow.setContentNonOwned(this, true);
    }

    mFloatingWindow.setVisible(true);
    mFloatingWindow.toFront(false);
}

ANALYSE_FILE_END
