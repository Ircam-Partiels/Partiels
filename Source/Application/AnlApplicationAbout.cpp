#include "AnlApplicationAbout.h"
#include "AnlApplicationInstance.h"
#include <ResourceData.h>

ANALYSE_FILE_BEGIN

Application::About::WindowContainer::WindowContainer(About& about)
: FloatingWindowContainer(juce::translate("About Partiels - v") + juce::String(ProjectInfo::versionString), about)
, mAbout(about)
, mTooltip(&mAbout)
{
}

Application::About::About()
: mImage(juce::Drawable::createFromImageData(ResourceData::icon_png, ResourceData::icon_pngSize))
{
    setSize(400, 400);
}

void Application::About::paint(juce::Graphics& g)
{
    if(mImage != nullptr)
    {
        mImage->drawWithin(g, {240.0f, 240.0f, 160.0f, 160.0f}, juce::RectanglePlacement::onlyReduceInSize | juce::RectanglePlacement::centred, 1.0f);
    }

    g.setColour(findColour(juce::Label::ColourIds::textColourId, true));
    g.setFont(16.0f);
    juce::String const text(ResourceData::About_txt, ResourceData::About_txtSize);
    g.drawFittedText(text, getLocalBounds().reduced(4), juce::Justification::left, 40);
}

ANALYSE_FILE_END
