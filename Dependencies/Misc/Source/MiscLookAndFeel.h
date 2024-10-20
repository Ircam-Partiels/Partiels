#include "MiscConcertinaTable.h"
#include "MiscFontManager.h"

MISC_FILE_BEGIN

class LookAndFeel
: public juce::LookAndFeel_V4
, public ConcertinaTable::LookAndFeelMethods
{
public:
    class ColourChart
    {
    public:
        // clang-format off
        enum class Type
        {
              background = 0
            , border
            , inactive
            , active
            , text
        };
        
        enum class Mode
        {
              night
            , day
            , grass
        };
        // clang-format on

        using Container = std::array<juce::Colour, magic_enum::enum_count<Type>()>;

        ColourChart(Container colours);
        ColourChart(Mode mode);
        ColourChart(ColourChart const&) = default;
        ColourChart& operator=(ColourChart const&) = default;

        ~ColourChart() = default;
        juce::Colour get(Type const& type) const;

    private:
        Container mColours;
    };

    LookAndFeel();
    ~LookAndFeel() override = default;

    virtual void setColourChart(ColourChart const& colourChart);
    ColourChart getColourChart() const;

    virtual juce::ScaledImage createDragImage(juce::Component& parent) const;

    // ConcertinaTable::LookAndFeelMethods
    juce::Font getHeaderFont(ConcertinaTable const& panel, int headerHeight) const override;
    int getHeaderHeight(ConcertinaTable const& panel) const override;
    void drawHeaderBackground(juce::Graphics& g, ConcertinaTable const& panel, juce::Rectangle<int> area, bool isMouseDown, bool isMouseOver) const override;
    void drawHeaderButton(juce::Graphics& g, ConcertinaTable const& panel, juce::Rectangle<int> area, float sizeRatio, bool isMouseDown, bool isMouseOver) const override;
    void drawHeaderTitle(juce::Graphics& g, ConcertinaTable const& panel, juce::Rectangle<int> area, juce::Font font, bool isMouseDown, bool isMouseOver) const override;

    // juce::LookAndFeel
    juce::Typeface::Ptr getTypefaceForFont(juce::Font const& font) override;

    // juce::Slider::LookAndFeelMethods
    int getSliderThumbRadius(juce::Slider& slider) override;
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float startAngle, float const endAngle, juce::Slider& slider) override;

    // juce::ScrollBar::LookAndFeelMethods
    bool areScrollbarButtonsVisible() override;
    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

    // juce::ComboBox::LookAndFeelMethods
    void drawComboBox(juce::Graphics& g, int width, int height, const bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;
    juce::Label* createComboBoxTextBox(juce::ComboBox& box) override;

    // juce::AlertWindow::LookAndFeelMethods
    int getAlertWindowButtonHeight() override;

    // juce::TableHeaderComponent::LookAndFeelMethods
    void drawTableHeaderBackground(juce::Graphics& g, juce::TableHeaderComponent& header) override;
    void drawTableHeaderColumn(juce::Graphics& g, juce::TableHeaderComponent& header, juce::String const& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags) override;

    // juce::DocumentWindow::LookAndFeelMethods
    void positionDocumentWindowButtons(juce::DocumentWindow& window, int titleBarX, int titleBarY, int titleBarW, int titleBarH, juce::Button* minimiseButton, juce::Button* maximiseButton, juce::Button* closeButton, bool positionTitleBarButtonsOnLeft) override;

    // juce::Label::LookAndFeelMethods
    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    // juce::ProgressBar::LookAndFeelMethods
    void drawProgressBar(juce::Graphics& g, juce::ProgressBar& progressBar, int width, int height, double progress, juce::String const& textToShow) override;
    bool isProgressBarOpaque(juce::ProgressBar& progressBar) override;

    // juce::TooltipWindow::LookAndFeelMethods
    void drawTooltip(juce::Graphics& g, juce::String const& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds(juce::String const& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;

    // juce::Button::LookAndFeelMethods
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, juce::Colour const& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    void drawTickBox(juce::Graphics& g, juce::Component& button, float x, float y, float w, float h, bool ticked, bool isEnabled, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    juce::Font getTextButtonFont(juce::TextButton& textButtton, int buttonHeight) override;

    // juce::CallOutBox::LookAndFeelMethods
    void drawCallOutBoxBackground(juce::CallOutBox& box, juce::Graphics& g, juce::Path const& path, juce::Image& cachedImage) override;
    int getCallOutBoxBorderSize(juce::CallOutBox const& box) override;
    float getCallOutBoxCornerSize(juce::CallOutBox const& box) override;

    // juce::TabbedButtonBar::LookAndFeelMethods
    juce::Font getTabButtonFont(juce::TabBarButton& button, float height) override;
    int getTabButtonBestWidth(juce::TabBarButton& button, int tabDepth) override;

private:
    FontManager mFontManager;
    ColourChart mColourChart{ColourChart::Mode::night};
};

MISC_FILE_END
