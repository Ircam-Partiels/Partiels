#include "AnlApplicationAbout.h"
#include "AnlApplicationInstance.h"
#include <ResourceData.h>

ANALYSE_FILE_BEGIN

Application::AboutContent::AboutContent()
: mImage(juce::ImageCache::getFromMemory(ResourceData::Ircamlogo_png, ResourceData::Ircamlogo_pngSize))
{
    mImage.onClick = []()
    {
        juce::URL("https://www.ircam.fr/").launchInDefaultBrowser();
    };
    addAndMakeVisible(mImage);
    setSize(400, 340);
}

void Application::AboutContent::paint(juce::Graphics& g)
{
    g.setColour(findColour(juce::Label::ColourIds::textColourId, true));
    juce::String const text(ResourceData::About_txt, ResourceData::About_txtSize);
    g.drawFittedText(text, getLocalBounds().reduced(4), juce::Justification::left, 40);
}

void Application::AboutContent::resized()
{
    auto constexpr imageWidth = 3072.0;
    auto constexpr imageHeight = 1181.0;
    auto constexpr imageOffet = 20;
    auto constexpr newWidth = 220;
    auto const newHeight = static_cast<int>(std::round(imageHeight * static_cast<double>(newWidth) / imageWidth));
    mImage.setBounds(getLocalBounds().removeFromBottom(newHeight + imageOffet).withSizeKeepingCentre(newWidth, newHeight));
}

Application::AboutPanel::AboutPanel()
: HideablePanelTyped<AboutContent>(juce::translate("About ") + Instance::get().getApplicationName() + " - v" + Instance::get().getApplicationVersion() + juce::String(" (BUILD_ID)").replace("BUILD_ID", PARTIELS_BUILD_ID))
{
}

ANALYSE_FILE_END
