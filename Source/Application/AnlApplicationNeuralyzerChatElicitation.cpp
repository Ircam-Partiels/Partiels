#include "AnlApplicationNeuralyzerChatElicitation.h"

ANALYSE_FILE_BEGIN

// A button that wraps its text on several lines (the height of the button depends on the length of the text)
class Application::Neuralyzer::ChatElicitation::OptionButton
: public juce::TextButton
{
public:
    explicit OptionButton(juce::String const& text)
    : juce::TextButton(text)
    {
        setWantsKeyboardFocus(false);
        setTooltip(text);
    }

    ~OptionButton() override = default;

    //! @brief Gets the height required to display the whole text
    int getHeightForWidth(int width) const
    {
        auto const layout = createLayout(width - horizontalMargin * 2, juce::Colours::black);
        return std::max(minimumHeight, static_cast<int>(std::ceil(layout.getHeight())) + verticalMargin * 2);
    }

    // juce::Button
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto& lookAndFeel = getLookAndFeel();
        lookAndFeel.drawButtonBackground(g, *this, findColour(juce::TextButton::buttonColourId), shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
        auto const bounds = getLocalBounds().reduced(horizontalMargin, verticalMargin).toFloat();
        auto const colour = findColour(juce::TextButton::textColourOffId).withMultipliedAlpha(isEnabled() ? 1.0f : 0.5f);
        auto const layout = createLayout(static_cast<int>(bounds.getWidth()), colour);
        layout.draw(g, bounds.withSizeKeepingCentre(bounds.getWidth(), layout.getHeight()));
    }

private:
    juce::TextLayout createLayout(int width, juce::Colour const& colour) const
    {
        juce::AttributedString text;
        text.setWordWrap(juce::AttributedString::WordWrap::byWord);
        text.setJustification(juce::Justification::centred);
        text.append(getButtonText(), juce::Font(juce::FontOptions(14.0f)), colour);
        juce::TextLayout layout;
        layout.createLayout(text, static_cast<float>(std::max(width, 1)));
        return layout;
    }

    static auto constexpr minimumHeight = 22;
    static auto constexpr horizontalMargin = 8;
    static auto constexpr verticalMargin = 4;
};

static auto constexpr buttonGap = 4;

Application::Neuralyzer::ChatElicitation::ChatElicitation()
{
    mViewport.setViewedComponent(&mContent, false);
    mViewport.setScrollBarsShown(true, false, true, false);
    addAndMakeVisible(mViewport);
}

Application::Neuralyzer::ChatElicitation::~ChatElicitation() = default;

void Application::Neuralyzer::ChatElicitation::setOptions(juce::StringArray const& options)
{
    mButtons.clear();
    for(auto const& option : options)
    {
        auto button = std::make_unique<OptionButton>(option);
        button->onClick = [this, option]()
        {
            if(onOptionClicked != nullptr)
            {
                onOptionClicked(option);
            }
        };
        mContent.addAndMakeVisible(button.get());
        mButtons.push_back(std::move(button));
    }
    mViewport.setViewPosition(0, 0);
    resized();
}

int Application::Neuralyzer::ChatElicitation::layoutButtons(int width, bool apply) const
{
    auto y = 0;
    for(auto const& button : mButtons)
    {
        auto const height = button->getHeightForWidth(width);
        if(apply)
        {
            button->setBounds(0, y, width, height);
        }
        y += height + buttonGap;
    }
    return std::max(y - buttonGap, 0);
}

int Application::Neuralyzer::ChatElicitation::getOptimalHeight(int width) const
{
    return layoutButtons(width, false);
}

void Application::Neuralyzer::ChatElicitation::resized()
{
    auto const bounds = getLocalBounds();
    mViewport.setBounds(bounds);
    // The scrollbar reduces the width of the buttons if they cannot all be displayed
    auto const needsScrollbar = layoutButtons(bounds.getWidth(), false) > bounds.getHeight();
    auto const width = needsScrollbar ? bounds.getWidth() - mViewport.getScrollBarThickness() : bounds.getWidth();
    mContent.setSize(width, layoutButtons(width, true));
}

ANALYSE_FILE_END
