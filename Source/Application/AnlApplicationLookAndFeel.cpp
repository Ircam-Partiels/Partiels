#include "AnlApplicationLookAndFeel.h"
#include "../Layout/AnlLayout.h"
#include "../Zoom/AnlZoomRuler.h"
#include "../Document/AnlDocumentSection.h"

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0f);
    setColour(Tools::ColouredPanel::ColourIds::backgroundColourId, juce::Colours::black);
    
    auto const grey = juce::Colours::grey;
    setColour(Layout::PropertySection::ColourIds::separatorColourId, juce::Colours::transparentBlack);
    setColour(Layout::StrechableContainer::Section::resizerActiveColourId, grey.brighter());
    setColour(Layout::StrechableContainer::Section::resizerInactiveColourId, grey.darker());
    
    setColour(Zoom::Ruler::backgroundColourId, grey);
    setColour(Zoom::Ruler::tickColourId, juce::Colours::black);
    setColour(Zoom::Ruler::textColourId, juce::Colours::white);
    setColour(Zoom::Ruler::anchorColourId, juce::Colours::red);
    setColour(Zoom::Ruler::selectionColourId, juce::Colours::blue);
    
    setColour(Analyzer::Thumbnail::backgroundColourId, grey);
    setColour(Document::Section::backgroundColourId, juce::Colours::black);
    
    // juce::ComboBox::LookAndFeelMethods
    setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::arrowColourId, juce::Colours::white);
}

bool Application::LookAndFeel::areScrollbarButtonsVisible()
{
    return false;
}

void Application::LookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, const bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH);
    juce::Rectangle<int> bounds(0, 0, width, height);
    g.setColour(box.findColour(juce::ComboBox::ColourIds::backgroundColourId, true));
    g.fillRect(bounds);
    g.setColour(box.findColour(box.hasKeyboardFocus(true) ? juce::ComboBox::ColourIds::focusedOutlineColourId :juce::ComboBox::ColourIds::outlineColourId, true));
    g.drawRect(bounds.reduced(1));
    
    g.setColour(box.findColour(juce::ComboBox::ColourIds::arrowColourId).withAlpha(box.isEnabled() ? 1.0f : 0.2f));
    auto const arrawBounds = bounds.removeFromRight(height / 2).withSizeKeepingCentre(height / 2, height / 2).reduced(2).toFloat();
    juce::Path p;
    p.addTriangle(arrawBounds.getTopLeft(), arrawBounds.getTopRight(), arrawBounds.getBottomLeft().withX(arrawBounds.getCentreX()));
    g.fillPath(p);
}

ANALYSE_FILE_END

