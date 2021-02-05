#include "AnlApplicationLookAndFeel.h"
#include "../Layout/AnlLayout.h"
#include "../Zoom/AnlZoomRuler.h"
#include "../Document/AnlDocumentSection.h"

ANALYSE_FILE_BEGIN

Application::LookAndFeel::LookAndFeel()
{
    JUCE_COMPILER_WARNING("Use a colour scheme")
    auto const backgroundColour = juce::Colours::grey.darker();
    auto const rulerColour = juce::Colours::grey;
    auto const textColour = juce::Colours::white;
    auto const thumbColour = juce::Colours::white;
    
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0f);
    setColour(ColouredPanel::ColourIds::backgroundColourId, backgroundColour);
    setColour(FloatingWindow::ColourIds::backgroundColourId, backgroundColour.darker());

    setColour(ConcertinaPanel::ColourIds::headerBackgroundColourId, backgroundColour);
    setColour(ConcertinaPanel::ColourIds::headerTitleColourId, textColour);
    setColour(ConcertinaPanel::ColourIds::headerButtonColourId, textColour);
    setColour(ConcertinaPanel::ColourIds::separatorColourId, juce::Colours::transparentBlack);

    setColour(Layout::StrechableContainer::Section::resizerActiveColourId, rulerColour.brighter());
    setColour(Layout::StrechableContainer::Section::resizerInactiveColourId, backgroundColour.darker());
    
    setColour(Zoom::Ruler::backgroundColourId, backgroundColour.darker());
    setColour(Zoom::Ruler::tickColourId, textColour);
    setColour(Zoom::Ruler::textColourId, textColour);
    setColour(Zoom::Ruler::anchorColourId, thumbColour);
    setColour(Zoom::Ruler::selectionColourId, thumbColour);
    
    setColour(Analyzer::Thumbnail::backgroundColourId, backgroundColour.darker());
    setColour(Analyzer::Thumbnail::textColourId, textColour);
    setColour(Analyzer::Section::sectionColourId, juce::Colours::black);
    
    setColour(Document::Section::backgroundColourId, backgroundColour);
    
    setColour(Document::Playhead::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Document::Playhead::playheadColourId, thumbColour);
    
    auto& colourScheme = getCurrentColourScheme();
    colourScheme.setUIColour(ColourScheme::UIColour::windowBackground, backgroundColour.darker());
    colourScheme.setUIColour(ColourScheme::UIColour::widgetBackground, backgroundColour.darker());
    
    // juce::ResizableWindow
    setColour(juce::ResizableWindow::ColourIds::backgroundColourId, backgroundColour.darker());
    
    // juce::AlertWindow
    setColour(juce::AlertWindow::ColourIds::backgroundColourId, backgroundColour.darker());
    
    // juce::TextButton
    setColour(juce::TextButton::ColourIds::buttonColourId, backgroundColour);
    setColour(juce::TextButton::ColourIds::buttonOnColourId, backgroundColour.brighter());
    
    // juce::Slider
    setColour(juce::Slider::ColourIds::backgroundColourId, backgroundColour);
    setColour(juce::Slider::ColourIds::thumbColourId, thumbColour);
    
    // juce::ComboBox
    setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::arrowColourId, textColour);
    
    // juce::ScrollBar
    setColour(juce::ScrollBar::ColourIds::backgroundColourId, backgroundColour.darker());
    setColour(juce::ScrollBar::ColourIds::thumbColourId, thumbColour);
    
    // juce::ListBox
    setColour(juce::ListBox::ColourIds::backgroundColourId, backgroundColour);
    setColour(juce::ListBox::ColourIds::outlineColourId, backgroundColour.darker());
    setColour(juce::ListBox::ColourIds::textColourId, textColour);

    // juce::TableHeaderComponent
    setColour(juce::TableHeaderComponent::ColourIds::textColourId, textColour);
    setColour(juce::TableHeaderComponent::ColourIds::backgroundColourId, backgroundColour);
    setColour(juce::TableHeaderComponent::ColourIds::outlineColourId, backgroundColour.darker());
    setColour(juce::TableHeaderComponent::ColourIds::highlightColourId, backgroundColour.brighter());
    
    // juce::TextEditor
    setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::textColourId, textColour);
    setColour(juce::TextEditor::ColourIds::highlightColourId, textColour.withAlpha(0.2f));
    setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::shadowColourId, juce::Colours::transparentBlack);
    
    // juce::CaretComponent
    setColour(juce::CaretComponent::ColourIds::caretColourId, textColour);
}

int Application::LookAndFeel::getSeparatorHeight(ConcertinaPanel const& panel) const
{
    juce::ignoreUnused(panel);
    return 2;
}

int Application::LookAndFeel::getHeaderHeight(ConcertinaPanel const& panel) const
{
    juce::ignoreUnused(panel);
    return 20;
}

juce::Font Application::LookAndFeel::getHeaderFont(ConcertinaPanel const& panel, int headerHeight) const
{
     juce::ignoreUnused(panel);
    return juce::Font(static_cast<float>(headerHeight - 2));
}

void Application::LookAndFeel::drawHeaderBackground(juce::Graphics& g, ConcertinaPanel const& panel, juce::Rectangle<int> area, bool isMouseDown, bool isMouseOver) const
{
    auto const contrast = isMouseDown || isMouseOver ? 0.1f : 0.0f;
    g.setColour(panel.findColour(ConcertinaPanel::headerBackgroundColourId).darker(contrast));
    g.fillRect(area);
}

void Application::LookAndFeel::drawHeaderButton(juce::Graphics& g, ConcertinaPanel const& panel, juce::Rectangle<int> area, float sizeRatio, bool isMouseDown, bool isMouseOver) const
{
    juce::ignoreUnused(isMouseDown, isMouseOver);
    auto const bounds = area.reduced(4).toFloat();
    juce::Path path;
    path.addTriangle(bounds.getTopLeft(), bounds.getTopRight(), {bounds.getCentreX(), bounds.getBottom() - 2});
    auto const rotation = static_cast<float>(1.0 - sizeRatio) * juce::MathConstants<float>::pi / 2.0f;
    path.applyTransform(juce::AffineTransform::rotation(rotation, bounds.getCentre().getX(), bounds.getCentre().getY()));
    path.closeSubPath();
    
    g.setColour(panel.findColour(ConcertinaPanel::headerButtonColourId, true));
    g.fillPath(path);
}

void Application::LookAndFeel::drawHeaderTitle(juce::Graphics& g, ConcertinaPanel const& panel, juce::Rectangle<int> area, juce::Font font, bool isMouseDown, bool isMouseOver) const
{
    juce::ignoreUnused(isMouseDown, isMouseOver);
    g.setFont(font);
    g.setColour(panel.findColour(ConcertinaPanel::headerTitleColourId, true));
    g.drawFittedText(panel.getTitle(), area.withTrimmedLeft(5), juce::Justification::centredLeft, 1, 1.0f);
}

bool Application::LookAndFeel::areScrollbarButtonsVisible()
{
    return false;
}

void Application::LookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused(isMouseDown);
    g.fillAll(scrollbar.findColour(juce::ScrollBar::ColourIds::backgroundColourId));
    
    auto const thumbBounds = isScrollbarVertical ?  juce::Rectangle<int>{x, thumbStartPosition, width, thumbSize} : juce::Rectangle<int>{thumbStartPosition, y, thumbSize, height};
    
    auto const colour = scrollbar.findColour(juce::ScrollBar::ColourIds::thumbColourId);
    g.setColour(isMouseOver ? colour.brighter (0.25f) :colour);
    g.fillRoundedRectangle(thumbBounds.reduced(1).toFloat(), 4.0f);
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

void Application::LookAndFeel::drawAlertBox(juce::Graphics& g, juce::AlertWindow& alert, juce::Rectangle<int> const& textArea, juce::TextLayout& textLayout)
{
    auto constexpr cornerSize = 4.0f;
    
    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::outlineColourId));
    g.drawRoundedRectangle(alert.getLocalBounds().toFloat(), cornerSize, 2.0f);
    
    auto const bounds = alert.getLocalBounds().reduced(1);
    g.reduceClipRegion(bounds);
    
    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::backgroundColourId));
    g.fillRoundedRectangle(bounds.toFloat(), cornerSize);

    auto getIcon = [&]()
    {
        switch (alert.getAlertType())
        {
            case juce::AlertWindow::QuestionIcon:
                return juce::ImageCache::getFromMemory(BinaryData::question_png, BinaryData::question_pngSize);
            case juce::AlertWindow::WarningIcon:
                return juce::ImageCache::getFromMemory(BinaryData::alert_png, BinaryData::alert_pngSize);
            case juce::AlertWindow::InfoIcon:
                return juce::ImageCache::getFromMemory(BinaryData::information_png, BinaryData::information_pngSize);
            case juce::AlertWindow::NoIcon:
                return juce::Image();
        }
    };
    
    g.drawImage(getIcon(), {-48.0f, -20.0f, 180.0f, 180.0f});
    
    g.setColour(alert.findColour(juce::AlertWindow::ColourIds::textColourId));
    textLayout.draw(g, textArea.withX(84).withTop(12).toFloat());
}

int Application::LookAndFeel::getAlertWindowButtonHeight()
{
    return 36;
}

void Application::LookAndFeel::drawTableHeaderColumn(juce::Graphics& g, juce::TableHeaderComponent& header, juce::String const& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags)
{
    juce::ignoreUnused(columnId);
    if(isMouseDown)
    {
        g.fillAll(header.findColour(juce::TableHeaderComponent::highlightColourId));
    }
    else if(isMouseOver)
    {
        g.fillAll(header.findColour(juce::TableHeaderComponent::highlightColourId).withMultipliedAlpha (0.625f));
    }
    
    g.setColour(header.findColour(juce::TableHeaderComponent::textColourId));
    auto area = juce::Rectangle<int>(width, height).reduced(4, 0);
    using ColumnPropertyFlags = juce::TableHeaderComponent::ColumnPropertyFlags;
    if ((columnFlags & (ColumnPropertyFlags::sortedForwards | ColumnPropertyFlags::sortedBackwards)) != 0)
    {
        juce::Path sortArrow;
        sortArrow.addTriangle(0.0f, 0.0f, 0.5f, (columnFlags & ColumnPropertyFlags::sortedForwards) != 0 ? -0.8f : 0.8f, 1.0f, 0.0f);
        g.fillPath(sortArrow, sortArrow.getTransformToScaleToFit(area.removeFromRight(height / 2).reduced(2).toFloat(), true));
    }

    g.setFont(juce::Font(static_cast<float>(height) * 0.5f, juce::Font::bold));
    g.drawFittedText(columnName, area, juce::Justification::centredLeft, 1);
}

juce::Button* Application::LookAndFeel::createDocumentWindowButton(int buttonType)
{
    using TitleBarButtons = juce::DocumentWindow::TitleBarButtons;
    if(buttonType == TitleBarButtons::closeButton)
    {
        auto button = std::make_unique<juce::ImageButton>();
        anlWeakAssert(button != nullptr);
        if(button != nullptr)
        {
            JUCE_COMPILER_WARNING("use a global approach");
            auto const image = juce::ImageCache::getFromMemory(BinaryData::annuler_png, BinaryData::annuler_pngSize);
            button->setImages(true, true, true, image, 1.0f, juce::Colours::grey, image, 0.8f, juce::Colours::grey.brighter(), image, 0.8f, juce::Colours::grey.brighter());
        }
        return button.release();
    }
    return LookAndFeel_V4::createDocumentWindowButton(buttonType);
}

ANALYSE_FILE_END

