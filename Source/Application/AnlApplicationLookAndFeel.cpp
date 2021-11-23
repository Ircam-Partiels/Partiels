#include "AnlApplicationLookAndFeel.h"
#include "../Document/AnlDocumentSection.h"
#include "../Zoom/AnlZoomRuler.h"

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    setColourChart(ColourChart::Mode::night);
}

void Application::LookAndFeel::setColourChart(ColourChart const& colourChart)
{
    using Type = ColourChart::Type;
    setColour(Zoom::Ruler::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(Zoom::Ruler::ColourIds::gridColourId, colourChart.get(Type::text));
    setColour(Zoom::Ruler::ColourIds::anchorColourId, colourChart.get(Type::active));
    setColour(Zoom::Ruler::ColourIds::selectionColourId, colourChart.get(Type::active));

    setColour(Transport::LoopBar::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Transport::LoopBar::ColourIds::thumbCoulourId, colourChart.get(Type::inactive));

    setColour(Transport::PlayheadBar::ColourIds::startPlayheadColourId, colourChart.get(Type::inactive));
    setColour(Transport::PlayheadBar::ColourIds::runningPlayheadColourId, colourChart.get(Type::active));

    setColour(Track::Thumbnail::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(Track::Thumbnail::ColourIds::backgroundColourId, colourChart.get(Type::background));

    setColour(Track::Section::ColourIds::backgroundColourId, colourChart.get(Type::background));

    setColour(Group::Thumbnail::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(Group::Thumbnail::ColourIds::backgroundColourId, colourChart.get(Type::background));

    setColour(Group::Section::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(Group::Section::ColourIds::overflyColourId, colourChart.get(Type::inactive).withAlpha(0.4f));

    setColour(Document::Section::ColourIds::backgroundColourId, colourChart.get(Type::background));
    Misc::LookAndFeel::setColourChart(colourChart);
}

ANALYSE_FILE_END
