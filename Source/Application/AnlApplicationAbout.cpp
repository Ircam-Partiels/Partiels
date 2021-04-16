#include "AnlApplicationAbout.h"
#include "AnlApplicationInstance.h"
#include <IconsData.h>
#include <MiscData.h>

ANALYSE_FILE_BEGIN

Application::About::About()
: FloatingWindowContainer("About Partiels - v" + juce::String(ProjectInfo::versionString), *this)
, mImage(juce::Drawable::createFromImageData(IconsData::icon_png, IconsData::icon_pngSize))
{
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
    juce::String const text(MiscData::About_txt, MiscData::About_txtSize);
    g.drawFittedText(text, getLocalBounds().reduced(4), juce::Justification::left, 40);
}

ANALYSE_FILE_END
