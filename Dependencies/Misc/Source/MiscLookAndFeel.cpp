#include "Misc.h"
#if defined(MISC_ZOOM_ENABLED)
#include "Zoom/MiscZoom.h"
#endif

#if defined(MISC_TRANSPORT_ENABLED)
#include "Transport/MiscTransport.h"
#endif

MISC_FILE_BEGIN

LookAndFeel::ColourChart::ColourChart(Mode mode)
{
    auto getColours = [&]() -> Container
    {
        switch(mode)
        {
            case Mode::night:
            {
                // clang-format off
                return
                {{
                      juce::Colour(0xff070f17)
                    , juce::Colour(0xff263553)
                    , juce::Colour(0xff415a77)
                    , juce::Colour(0xff778da9)
                    , juce::Colour(0xffe0e1dd)
                }};
                // clang-format on
            }
            break;
            case Mode::day:
            {
                // clang-format off
                return
                {{
                      juce::Colour(0xfff8f8f7)
                    , juce::Colour(0xffaeb5b4)
                    , juce::Colour(0xff7e7a7d)
                    , juce::Colour(0xff27232b)
                    , juce::Colour(0xff000000)
                }};
                // clang-format on
            }
            break;
            case Mode::grass:
            {
                // clang-format off
                return
                {{
                      juce::Colour(0xffcad2c5)
                    , juce::Colour(0xff84a98c)
                    , juce::Colour(0xff52796f)
                    , juce::Colour(0xff354f52)
                    , juce::Colour(0xff2f3e46)
                }};
                // clang-format on
            }
            break;
        }
        return {};
    };
    mColours = getColours();
}

LookAndFeel::ColourChart::ColourChart(Container colours)
: mColours(std::move(colours))
{
}

juce::Colour LookAndFeel::ColourChart::get(Type const& type) const
{
    return mColours[static_cast<size_t>(magic_enum::enum_integer(type))];
}

LookAndFeel::LookAndFeel()
{
    setColourChart({ColourChart::Mode::night});
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0f);
}

LookAndFeel::ColourChart LookAndFeel::getColourChart() const
{
    return mColourChart;
}

void LookAndFeel::setColourChart(ColourChart const& colourChart)
{
    mColourChart = colourChart;
    using Type = ColourChart::Type;
    setColour(ColourButton::ColourIds::borderOffColourId, colourChart.get(Type::inactive));
    setColour(ColourButton::ColourIds::borderOnColourId, colourChart.get(Type::active));

    setColour(ColouredPanel::ColourIds::backgroundColourId, colourChart.get(Type::border));

    setColour(FloatingWindow::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(FloatingWindow::ColourIds::hightlightColourId, juce::Colours::white);

    setColour(ConcertinaTable::ColourIds::headerBackgroundColourId, juce::Colours::transparentBlack);
    setColour(ConcertinaTable::ColourIds::headerBorderColourId, colourChart.get(Type::border));
    setColour(ConcertinaTable::ColourIds::headerTitleColourId, colourChart.get(Type::text));
    setColour(ConcertinaTable::ColourIds::headerButtonColourId, colourChart.get(Type::text));

    setColour(Decorator::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(Decorator::ColourIds::normalBorderColourId, colourChart.get(Type::border));
    setColour(Decorator::ColourIds::highlightedBorderColourId, colourChart.get(Type::inactive));

    setColour(Icon::ColourIds::normalColourId, colourChart.get(Type::inactive));
    setColour(Icon::ColourIds::overColourId, colourChart.get(Type::active));
    setColour(Icon::ColourIds::downColourId, colourChart.get(Type::active));

    setColour(SpinningIcon::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(SpinningIcon::ColourIds::foregroundColourId, colourChart.get(Type::inactive));

#if defined(MISC_ZOOM_ENABLED)
    setColour(Zoom::Ruler::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(Zoom::Ruler::ColourIds::gridColourId, colourChart.get(Type::text));
    setColour(Zoom::Ruler::ColourIds::anchorColourId, colourChart.get(Type::active));
    setColour(Zoom::Ruler::ColourIds::selectionColourId, colourChart.get(Type::active));
    setColour(Zoom::SelectionBar::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Zoom::SelectionBar::ColourIds::thumbCoulourId, colourChart.get(Type::inactive));
#endif

#if defined(MISC_TRANSPORT_ENABLED)
    setColour(Transport::LoopBar::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Transport::LoopBar::ColourIds::thumbCoulourId, colourChart.get(Type::inactive));
    setColour(Transport::SelectionBar::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(Transport::SelectionBar::ColourIds::thumbCoulourId, colourChart.get(Type::inactive).withAlpha(0.4f));
    setColour(Transport::PlayheadBar::ColourIds::startPlayheadColourId, colourChart.get(Type::inactive));
    setColour(Transport::PlayheadBar::ColourIds::runningPlayheadColourId, colourChart.get(Type::active));
#endif

    auto& colourScheme = getCurrentColourScheme();
    colourScheme.setUIColour(ColourScheme::UIColour::windowBackground, colourChart.get(Type::background));
    colourScheme.setUIColour(ColourScheme::UIColour::widgetBackground, colourChart.get(Type::background));
    colourScheme.setUIColour(ColourScheme::UIColour::menuBackground, colourChart.get(Type::background));
    colourScheme.setUIColour(ColourScheme::UIColour::outline, colourChart.get(Type::border));
    colourScheme.setUIColour(ColourScheme::UIColour::defaultText, colourChart.get(Type::text));
    colourScheme.setUIColour(ColourScheme::UIColour::defaultFill, colourChart.get(Type::inactive));
    colourScheme.setUIColour(ColourScheme::UIColour::highlightedText, colourChart.get(Type::text));
    colourScheme.setUIColour(ColourScheme::UIColour::highlightedText, colourChart.get(Type::active));
    colourScheme.setUIColour(ColourScheme::UIColour::menuText, colourChart.get(Type::text));
    setColourScheme(colourScheme);

    // juce::ResizableWindow
    setColour(juce::ResizableWindow::ColourIds::backgroundColourId, colourChart.get(Type::background));

    // juce::AlertWindow
    setColour(juce::AlertWindow::ColourIds::backgroundColourId, colourChart.get(Type::background));

    // juce::Label
    setColour(juce::Label::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(juce::Label::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::ColourIds::backgroundWhenEditingColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::ColourIds::textWhenEditingColourId, colourChart.get(Type::text));
    setColour(juce::Label::ColourIds::outlineWhenEditingColourId, juce::Colours::transparentBlack);

    // juce::TextButton
    setColour(juce::TextButton::ColourIds::buttonColourId, colourChart.get(Type::border));
    setColour(juce::TextButton::ColourIds::buttonOnColourId, colourChart.get(Type::inactive));
    setColour(juce::TextButton::ColourIds::textColourOffId, colourChart.get(Type::text));
    setColour(juce::TextButton::ColourIds::textColourOnId, colourChart.get(Type::text));

    // juce::Slider
    setColour(juce::Slider::ColourIds::backgroundColourId, colourChart.get(Type::border));
    setColour(juce::Slider::ColourIds::trackColourId, colourChart.get(Type::inactive));
    setColour(juce::Slider::ColourIds::thumbColourId, colourChart.get(Type::active));
    setColour(juce::Slider::ColourIds::rotarySliderFillColourId, colourChart.get(Type::active));
    setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, colourChart.get(Type::inactive));

    // juce::ComboBox
    setColour(juce::ComboBox::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::ColourIds::arrowColourId, colourChart.get(Type::text));

    // juce::ScrollBar
    setColour(juce::ScrollBar::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(juce::ScrollBar::ColourIds::thumbColourId, colourChart.get(Type::inactive));

    // juce::ListBox
    setColour(juce::ListBox::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(juce::ListBox::ColourIds::outlineColourId, colourChart.get(Type::border));
    setColour(juce::ListBox::ColourIds::textColourId, colourChart.get(Type::text));

    // juce::TableHeaderComponent
    setColour(juce::TableHeaderComponent::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(juce::TableHeaderComponent::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(juce::TableHeaderComponent::ColourIds::outlineColourId, colourChart.get(Type::border));
    setColour(juce::TableHeaderComponent::ColourIds::highlightColourId, colourChart.get(Type::active));

    // juce::TextEditor
    setColour(juce::TextEditor::ColourIds::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(juce::TextEditor::ColourIds::highlightColourId, colourChart.get(Type::text).withAlpha(0.2f));
    setColour(juce::TextEditor::ColourIds::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::ColourIds::shadowColourId, juce::Colours::transparentBlack);

    // juce::CaretComponent
    setColour(juce::CaretComponent::ColourIds::caretColourId, colourChart.get(Type::text));

    // juce::PopupMenu
    setColour(juce::PopupMenu::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(juce::PopupMenu::ColourIds::textColourId, colourChart.get(Type::text));
    setColour(juce::PopupMenu::ColourIds::headerTextColourId, colourChart.get(Type::text));
    setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, colourChart.get(Type::text).withAlpha(0.2f));
    setColour(juce::PopupMenu::ColourIds::highlightedTextColourId, colourChart.get(Type::text));

    // juce::TooltipWindow
    setColour(juce::TooltipWindow::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(juce::TooltipWindow::ColourIds::outlineColourId, colourChart.get(Type::border));
    setColour(juce::TooltipWindow::ColourIds::textColourId, colourChart.get(Type::text));

    // juce::ProgressBar
    setColour(juce::ProgressBar::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(juce::ProgressBar::ColourIds::foregroundColourId, colourChart.get(Type::active));

    // juce::HyperlinkButton
    setColour(juce::HyperlinkButton::ColourIds::textColourId, juce::Colour(0xFF16A4DB));

    // juce::ColourSelector
    setColour(juce::ColourSelector::ColourIds::backgroundColourId, colourChart.get(Type::background));
    setColour(juce::ColourSelector::ColourIds::labelTextColourId, colourChart.get(Type::text));

    // juce::TabbedButtonBar
    setColour(juce::TabbedButtonBar::ColourIds::tabOutlineColourId, colourChart.get(Type::border));
    setColour(juce::TabbedButtonBar::ColourIds::tabTextColourId, colourChart.get(Type::inactive));
    setColour(juce::TabbedButtonBar::ColourIds::frontOutlineColourId, colourChart.get(Type::active));
    setColour(juce::TabbedButtonBar::ColourIds::frontTextColourId, colourChart.get(Type::text));
}

juce::ScaledImage LookAndFeel::createDragImage(juce::Component& parent) const
{
    auto constexpr scaleFactor = 2.0;
    auto image = parent.createComponentSnapshot(parent.getLocalBounds(), true, static_cast<float>(scaleFactor)).convertedToFormat(juce::Image::ARGB);
    image.multiplyAllAlphas(0.6f);

    auto const lastMouseDown = parent.getMouseXYRelative();
    auto const relPosition = parent.getLocalPoint(nullptr, lastMouseDown).toDouble();
    auto const clippedPosition = (image.getBounds().toDouble() / scaleFactor).getConstrainedPoint(relPosition);

    juce::Image fade(juce::Image::SingleChannel, image.getWidth(), image.getHeight(), true);
    juce::Graphics fadeContext(fade);

    juce::ColourGradient gradient;
    gradient.isRadial = true;
    gradient.point1 = clippedPosition.toFloat() * scaleFactor;
    gradient.point2 = gradient.point1 + juce::Point<float>(0.0f, scaleFactor * 400.0f);
    gradient.addColour(0.0, juce::Colours::white);
    gradient.addColour(0.375, juce::Colours::white);
    gradient.addColour(1.0, juce::Colours::transparentWhite);

    fadeContext.setGradientFill(gradient);
    fadeContext.fillAll();

    juce::Image composite(juce::Image::ARGB, image.getWidth(), image.getHeight(), true);
    juce::Graphics compositeContext(composite);

    compositeContext.reduceClipRegion(fade, {});
    compositeContext.drawImageAt(image, 0, 0);

    return {composite, scaleFactor};
}

int LookAndFeel::getHeaderHeight(ConcertinaTable const& panel) const
{
    juce::ignoreUnused(panel);
    return 22;
}

juce::Font LookAndFeel::getHeaderFont(ConcertinaTable const& panel, int headerHeight) const
{
    juce::ignoreUnused(panel);
    return juce::Font(juce::FontOptions(static_cast<float>(headerHeight - 4)));
}

void LookAndFeel::drawHeaderBackground(juce::Graphics& g, ConcertinaTable const& panel, juce::Rectangle<int> area, bool isMouseDown, bool isMouseOver) const
{
    auto constexpr corner = 2.0f;
    auto const contrast = isMouseDown || isMouseOver ? 0.1f : 0.0f;
    g.setColour(panel.findColour(ConcertinaTable::headerBackgroundColourId).brighter(contrast));
    g.fillRoundedRectangle(area.toFloat(), corner);
    g.setColour(panel.findColour(ConcertinaTable::headerBorderColourId).brighter(contrast));
    g.drawRoundedRectangle(area.reduced(1).toFloat(), corner, 1.0f);
}

void LookAndFeel::drawHeaderButton(juce::Graphics& g, ConcertinaTable const& panel, juce::Rectangle<int> area, float sizeRatio, bool isMouseDown, bool isMouseOver) const
{
    auto const contrast = isMouseDown || isMouseOver ? 0.1f : 0.0f;
    auto const bounds = area.reduced(5).toFloat();
    juce::Path path;
    path.addTriangle(bounds.getTopLeft(), bounds.getTopRight(), {bounds.getCentreX(), bounds.getBottom() - 2});
    auto const rotation = static_cast<float>(1.0 - sizeRatio) * juce::MathConstants<float>::pi / 2.0f;
    path.applyTransform(juce::AffineTransform::rotation(rotation, bounds.getCentre().getX(), bounds.getCentre().getY()));
    path.closeSubPath();

    auto const alpha = panel.isEnabled() ? 1.0f : 0.5f;
    g.setColour(panel.findColour(ConcertinaTable::headerButtonColourId).brighter(contrast).withAlpha(alpha));
    g.fillPath(path);
}

void LookAndFeel::drawHeaderTitle(juce::Graphics& g, ConcertinaTable const& panel, juce::Rectangle<int> area, juce::Font font, bool isMouseDown, bool isMouseOver) const
{
    auto const contrast = isMouseDown || isMouseOver ? 0.1f : 0.0f;
    g.setFont(font);
    auto const alpha = panel.isEnabled() ? 1.0f : 0.5f;
    g.setColour(panel.findColour(ConcertinaTable::headerTitleColourId).brighter(contrast).withAlpha(alpha));
    g.drawFittedText(panel.getTitle(), area.reduced(1).withTrimmedLeft(3), juce::Justification::centredLeft, 1, 1.0f);
}

juce::Typeface::Ptr LookAndFeel::getTypefaceForFont(juce::Font const& font)
{
    if(font.getTypefaceName() == juce::Font::getDefaultSansSerifFontName())
    {
        return mFontManager.getDefaultSansSerifTypefaceWithStyle(font.getTypefaceStyle());
    }
    return juce::LookAndFeel::getTypefaceForFont(font);
}

int LookAndFeel::getSliderThumbRadius(juce::Slider& slider)
{
    if(slider.isRotary())
    {
        return std::clamp(std::min(slider.getWidth(), slider.getHeight()) / 6, 4, 6);
    }
    return std::min(9, static_cast<int>(static_cast<float>(slider.isHorizontal() ? slider.getHeight() : slider.getWidth()) * 0.5f));
}

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float const startAngle, float const endAngle, juce::Slider& slider)
{
    auto const thumbWidth = static_cast<float>(getSliderThumbRadius(slider));
    auto const lineWidth = thumbWidth / 2.0f;
    auto const bounds = juce::Rectangle<int>(x, y, width, height).reduced(1).toFloat();

    auto const radius = std::min(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto const toAngle = startAngle + sliderPos * (endAngle - startAngle);
    auto const arcRadius = radius - lineWidth * 0.5f;

    juce::Path outline;
    outline.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.f, startAngle, endAngle, true);
    g.setColour(slider.findColour(juce::Slider::ColourIds::rotarySliderOutlineColourId));
    g.strokePath(outline, juce::PathStrokeType(lineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if(slider.isEnabled())
    {
        juce::Path path;
        path.addCentredArc(bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius, 0.f, startAngle, toAngle, true);
        g.setColour(slider.findColour(juce::Slider::ColourIds::rotarySliderFillColourId));
        g.strokePath(path, juce::PathStrokeType(lineWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    static auto constexpr halfPi = juce::MathConstants<float>::halfPi;
    juce::Point<float> const thumbPoint(bounds.getCentreX() + arcRadius * std::cos(toAngle - halfPi), bounds.getCentreY() + arcRadius * std::sin(toAngle - halfPi));

    g.setColour(slider.findColour(juce::Slider::ColourIds::thumbColourId));
    g.fillEllipse(juce::Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
}

bool LookAndFeel::areScrollbarButtonsVisible()
{
    return false;
}

void LookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused(isMouseDown);
    g.fillAll(scrollbar.findColour(juce::ScrollBar::ColourIds::backgroundColourId));

    auto const thumbBounds = isScrollbarVertical ? juce::Rectangle<int>{x, thumbStartPosition, width, thumbSize} : juce::Rectangle<int>{thumbStartPosition, y, thumbSize, height};

    auto const colour = scrollbar.findColour(juce::ScrollBar::ColourIds::thumbColourId);
    g.setColour(isMouseOver ? colour.brighter(0.25f) : colour);
    g.fillRoundedRectangle(thumbBounds.reduced(1).toFloat(), 2.0f);
}

void LookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, const bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    juce::ignoreUnused(isButtonDown, buttonX, buttonY, buttonW, buttonH);
    juce::Rectangle<int> bounds(0, 0, width, height);
    g.setColour(box.findColour(juce::ComboBox::ColourIds::backgroundColourId, true));
    g.fillRect(bounds);
    g.setColour(box.findColour(box.hasKeyboardFocus(true) ? juce::ComboBox::ColourIds::focusedOutlineColourId : juce::ComboBox::ColourIds::outlineColourId, true));
    g.drawRect(bounds.reduced(1));

    g.setColour(box.findColour(juce::ComboBox::ColourIds::arrowColourId).withAlpha(box.isEnabled() ? 1.0f : 0.2f));
    auto const arrawBounds = bounds.removeFromRight(height / 2).withSizeKeepingCentre(height / 2, height / 2).reduced(2).toFloat();
    juce::Path p;
    p.addTriangle(arrawBounds.getTopLeft(), arrawBounds.getTopRight(), arrawBounds.getBottomLeft().withX(arrawBounds.getCentreX()));
    g.fillPath(p);
}

void LookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    auto const height = box.getHeight();
    label.setBounds(1, 1, box.getWidth() - 2 - height / 2, height - 2);
    label.setFont(getComboBoxFont(box));
}

juce::Label* LookAndFeel::createComboBoxTextBox(juce::ComboBox& box)
{
    auto const& properties = box.getProperties();
    if(properties.getWithDefault("isNumber", false))
    {
        auto label = std::make_unique<NumberField::Label>();
        if(label != nullptr)
        {
            label->loadProperties(properties, juce::NotificationType::dontSendNotification);
        }
        return label.release();
    }
    return juce::LookAndFeel_V4::createComboBoxTextBox(box);
}

int LookAndFeel::getAlertWindowButtonHeight()
{
    return 36;
}

void LookAndFeel::drawTableHeaderBackground(juce::Graphics& g, juce::TableHeaderComponent& header)
{
    auto const backgroundColour = header.findColour(juce::TableHeaderComponent::backgroundColourId);
    auto const outlineColour = header.findColour(juce::TableHeaderComponent::outlineColourId);

    for(int index = header.getNumColumns(true); --index >= 0;)
    {
        auto const columnBounds = header.getColumnPosition(index).reduced(1).toFloat();
        g.setColour(backgroundColour);
        g.fillRoundedRectangle(columnBounds, 2.0f);
        g.setColour(outlineColour);
        g.drawRoundedRectangle(columnBounds, 2.0f, 1.0f);
    }
}

void LookAndFeel::drawTableHeaderColumn(juce::Graphics& g, juce::TableHeaderComponent& header, juce::String const& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags)
{
    juce::ignoreUnused(columnId);
    using ColumnPropertyFlags = juce::TableHeaderComponent::ColumnPropertyFlags;
    if((isMouseDown || isMouseOver) && (columnFlags & ColumnPropertyFlags::sortable))
    {
        auto const columnBounds = g.getClipBounds().reduced(2).toFloat();
        g.setColour(header.findColour(juce::TableHeaderComponent::highlightColourId).withMultipliedAlpha(isMouseOver ? 0.625f : 1.0f));
        g.fillRoundedRectangle(columnBounds, 2.0f);
    }

    g.setColour(header.findColour(juce::TableHeaderComponent::textColourId));
    auto area = juce::Rectangle<int>(width, height).reduced(4, 0);
    if((columnFlags & (ColumnPropertyFlags::sortedForwards | ColumnPropertyFlags::sortedBackwards)) != 0)
    {
        juce::Path sortArrow;
        sortArrow.addTriangle(0.0f, 0.0f, 0.5f, (columnFlags & ColumnPropertyFlags::sortedForwards) != 0 ? -0.8f : 0.8f, 1.0f, 0.0f);
        g.fillPath(sortArrow, sortArrow.getTransformToScaleToFit(area.removeFromRight(height / 2).reduced(2).toFloat(), true));
    }

    g.setFont(juce::Font(juce::FontOptions(static_cast<float>(height) * 0.5f, juce::Font::bold)));
    g.drawText(columnName, area, juce::Justification::centredLeft, false);
}

void LookAndFeel::positionDocumentWindowButtons(juce::DocumentWindow& window, int titleBarX, int titleBarY, int titleBarW, int titleBarH, juce::Button* minimiseButton, juce::Button* maximiseButton, juce::Button* closeButton, bool positionTitleBarButtonsOnLeft)
{
    juce::ignoreUnused(window);
    auto constexpr buttonOffset = 4;
    auto const buttonHeight = titleBarH - 2 * buttonOffset;
    auto const buttonWidth = static_cast<int>(std::floor(buttonHeight * 1.2));
    auto bounds = juce::Rectangle<int>(titleBarX + buttonOffset, titleBarY + buttonOffset, titleBarW - 2 * buttonOffset, buttonHeight);

    auto placeButton = [&](juce::Button* button)
    {
        if(button != nullptr)
        {
            button->setBounds(positionTitleBarButtonsOnLeft ? bounds.removeFromLeft(buttonWidth) : bounds.removeFromRight(buttonWidth));
        }
    };
    placeButton(closeButton);
    placeButton(positionTitleBarButtonsOnLeft ? maximiseButton : minimiseButton);
    placeButton(positionTitleBarButtonsOnLeft ? minimiseButton : maximiseButton);
}

void LookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    if(!label.isBeingEdited())
    {
        auto const isEditable = (label.isEnabled() && label.isEditable());
        auto const alpha = label.isEnabled() ? 1.0f : 0.5f;
        auto const contrast = (isEditable && label.isMouseOverOrDragging()) ? 0.25f : 0.0f;
        auto const textColour = label.findColour(juce::Label::textColourId);
        g.setColour(isEditable && label.isMouseOver() ? textColour.contrasting(contrast) : textColour.withMultipliedAlpha(alpha));

        auto const font = getLabelFont(label);
        g.setFont(font);

        auto const caretWidth = label.isEditable() ? 3 : 0;
        auto const textArea = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds()).withTrimmedRight(caretWidth);

        auto const numLines = std::max(1, static_cast<int>(std::floor(static_cast<float>(textArea.getHeight()) / (font.getHeight() + 2.0f))));
        g.drawFittedText(label.getText(), textArea, label.getJustificationType(), numLines, label.getMinimumHorizontalScale());

        g.setColour(label.findColour(juce::Label::outlineColourId).withMultipliedAlpha(alpha));
    }
    else if(label.isEnabled())
    {
        g.setColour(label.findColour(juce::Label::outlineColourId));
    }

    g.drawRect(label.getLocalBounds());
}

void LookAndFeel::drawProgressBar(juce::Graphics& g, juce::ProgressBar& progressBar, int width, int height, double progress, juce::String const& textToShow)
{
    juce::ignoreUnused(width, height);

    auto const barBounds = progressBar.getLocalBounds().toFloat();
    auto const cornerSize = static_cast<float>(barBounds.getHeight()) * 0.5f;

    auto const backgroundColour = progressBar.findColour(juce::ProgressBar::ColourIds::backgroundColourId);
    auto const foregroundColour = progressBar.findColour(juce::ProgressBar::ColourIds::foregroundColourId);

    g.setColour(backgroundColour);
    g.fillRoundedRectangle(barBounds, cornerSize);

    if(progress > 0.0)
    {
        g.setColour(foregroundColour);
        auto const progressWidth = barBounds.getWidth() * static_cast<float>(std::min(progress, 1.0));
        g.fillRoundedRectangle(barBounds.withWidth(progressWidth), cornerSize);
    }

    if(!textToShow.isEmpty())
    {
        g.setColour(juce::Colour::contrasting(backgroundColour, foregroundColour));
        g.setFont(barBounds.getHeight() * 0.6f);
        auto getText = [&]()
        {
            if(juce::CharacterFunctions::isDigit(textToShow[0]))
            {
                return textToShow;
            }
            return textToShow + ' ' + juce::String(static_cast<int>(std::round(progress * 100.0))) + '%';
        };
        g.drawText(getText(), 0, 0, width, height, juce::Justification::centred, false);
    }
}

bool LookAndFeel::isProgressBarOpaque(juce::ProgressBar& progressBar)
{
    juce::ignoreUnused(progressBar);
    return false;
}

void LookAndFeel::drawTooltip(juce::Graphics& g, juce::String const& text, int width, int height)
{
    juce::Rectangle<float> const bounds(static_cast<float>(width), static_cast<float>(height));
    auto constexpr cornerSize = 5.0f;

    g.setColour(findColour(juce::TooltipWindow::ColourIds::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerSize);

    g.setColour(findColour(juce::TooltipWindow::ColourIds::outlineColourId));
    g.drawRoundedRectangle(bounds.reduced(0.5f, 0.5f), cornerSize, 1.0f);

    juce::AttributedString attributedString;
    attributedString.setJustification(juce::Justification::centredLeft);
    attributedString.append(text, juce::Font(juce::FontOptions(13.0f)), findColour(juce::TooltipWindow::ColourIds::textColourId));

    juce::TextLayout tl;
    tl.createLayout(attributedString, static_cast<float>(std::min(width, 500 - 8)));
    tl.draw(g, bounds.reduced(4.0f, 4.0f));
}

juce::Rectangle<int> LookAndFeel::getTooltipBounds(juce::String const& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea)
{
    juce::AttributedString s;
    s.setJustification(juce::Justification::centredLeft);
    s.append(tipText, juce::Font(juce::FontOptions(13.0f)), juce::Colours::black);

    juce::TextLayout tl;
    tl.createLayout(s, static_cast<float>(std::min(parentArea.getWidth() - 8, 500)));
    auto const w = static_cast<int>(tl.getWidth()) + 8;
    auto const h = static_cast<int>(tl.getHeight()) + 8;
    auto const x = screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 8) : screenPos.x + 8;
    auto const y = screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 8) : screenPos.y + 9;
    return juce::Rectangle<int>(x, y, w, h).constrainedWithin(parentArea);
}

void LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, juce::Colour const& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto constexpr cornerSize = 2.0f;
    auto const bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);

    auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f).withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);

    if(shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
    {
        baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
    }

    g.setColour(baseColour);

    auto const flatOnLeft = button.isConnectedOnLeft();
    auto const flatOnRight = button.isConnectedOnRight();
    auto const flatOnTop = button.isConnectedOnTop();
    auto const flatOnBottom = button.isConnectedOnBottom();

    if(flatOnLeft || flatOnRight || flatOnTop || flatOnBottom)
    {
        juce::Path path;
        path.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                 bounds.getWidth(), bounds.getHeight(),
                                 cornerSize, cornerSize,
                                 !(flatOnLeft || flatOnTop),
                                 !(flatOnRight || flatOnTop),
                                 !(flatOnLeft || flatOnBottom),
                                 !(flatOnRight || flatOnBottom));

        g.fillPath(path);

        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.strokePath(path, juce::PathStrokeType(1.0f));
    }
    else
    {
        g.fillRoundedRectangle(bounds, cornerSize);

        g.setColour(button.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto const width = static_cast<float>(button.getWidth());
    auto const height = static_cast<float>(button.getHeight());

    drawTickBox(g, button, 0.0f, 0.0f, width, height, button.getToggleState(), button.isEnabled(), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    g.setColour(button.findColour(juce::ToggleButton::textColourId));
    g.setFont(std::min(15.0f, height * 0.75f));

    if(!button.isEnabled())
    {
        g.setOpacity(0.5f);
    }

    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(2, 0), juce::Justification::centredLeft, 10);
}

void LookAndFeel::drawTickBox(juce::Graphics& g, juce::Component& button, float x, float y, float w, float h, bool ticked, bool isEnabled, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto const hasMultipleValues = button.getProperties().contains("Multiple Values");

    auto borderColour = button.findColour(juce::ToggleButton::tickDisabledColourId);
    if(shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
    {
        borderColour = borderColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
    }

    juce::Rectangle<float> const tickBounds(x + 1.0f, y + 1.0f, w - 2.0f, h - 2.0f);
    g.setColour(borderColour);
    g.drawRoundedRectangle(tickBounds, 4.0f, 1.0f);

    ticked = hasMultipleValues || ticked;
    if(ticked)
    {
        auto tickColour = !isEnabled || hasMultipleValues ? borderColour : button.findColour(juce::ToggleButton::tickColourId);
        if(shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        {
            tickColour = tickColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
        }

        g.setColour(tickColour);
        auto const tick = getTickShape(0.75f);
        g.fillPath(tick, tick.getTransformToScaleToFit(tickBounds.reduced(3.0f), false));
    }
}

juce::Font LookAndFeel::getTextButtonFont(juce::TextButton& textButtton, int buttonHeight)
{
    auto const fontDescription = textButtton.getProperties().getWithDefault("Font", {});
    if(!fontDescription.isString())
    {
        return {juce::FontOptions(std::min(16.0f, std::max(static_cast<float>(buttonHeight) * 0.6f, 12.0f)))};
    }
    return juce::Font::fromString(fontDescription);
}

int LookAndFeel::getCallOutBoxBorderSize(juce::CallOutBox const& box)
{
    juce::ignoreUnused(box);
    return 2;
}

float LookAndFeel::getCallOutBoxCornerSize(juce::CallOutBox const& box)
{
    juce::ignoreUnused(box);
    return 2.0f;
}

void LookAndFeel::drawCallOutBoxBackground(juce::CallOutBox& box, juce::Graphics& g, juce::Path const& path, juce::Image& cachedImage)
{
    juce::ignoreUnused(path, cachedImage);
    auto& colourScheme = getCurrentColourScheme();
    auto const bounds = box.getLocalBounds().reduced(getCallOutBoxBorderSize(box));
    juce::DropShadow(juce::Colours::black.withAlpha(0.7f), 8, {0, 2}).drawForRectangle(g, bounds);
    g.setColour(colourScheme.getUIColour(ColourScheme::UIColour::windowBackground));
    g.fillRoundedRectangle(bounds.toFloat(), getCallOutBoxCornerSize(box));
    g.setColour(colourScheme.getUIColour(ColourScheme::UIColour::outline));
    g.drawRoundedRectangle(bounds.toFloat(), getCallOutBoxCornerSize(box), 1.0f);
}

juce::Font LookAndFeel::getTabButtonFont(juce::TabBarButton& button, float height)
{
    juce::ignoreUnused(button);
    return {juce::FontOptions(height * 0.8f)};
}

int LookAndFeel::getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth)
{
    auto const font = getTabButtonFont(button, static_cast<float>(button.getTabbedButtonBar().getHeight()));
    auto const extraSpace = std::max(getTabButtonOverlap(tabDepth), 0) * 2;
    auto width = juce::GlyphArrangement::getStringWidthInt(font, button.getButtonText().trim()) + extraSpace;

    if(auto* extraComponent = button.getExtraComponent())
    {
        width += button.getTabbedButtonBar().isVertical() ? extraComponent->getHeight() : extraComponent->getWidth();
    }

    return std::clamp(width, tabDepth * 2, tabDepth * 8);
}

MISC_FILE_END
