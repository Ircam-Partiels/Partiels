#include "MiscTooltip.h"

MISC_FILE_BEGIN

void Tooltip::Server::timerCallback()
{
    auto getTipFor = [](juce::Component* component) -> juce::String
    {
        if(component != nullptr && !component->isCurrentlyBlockedByAnotherModalComponent() && juce::Process::isForegroundProcess())
        {
            if(auto* client = dynamic_cast<Client*>(component))
            {
                return client->getTooltip();
            }
            else if(auto* parent = component->findParentComponentOfClass<BubbleClient>())
            {
                return parent->getTooltip();
            }
        }
        return {};
    };

    auto mouseSource = juce::Desktop::getInstance().getMainMouseSource();
    auto const newTip = getTipFor(mouseSource.isTouch() ? nullptr : mouseSource.getComponentUnderMouse());
    if(newTip != mTip)
    {
        mTip = newTip;
        if(onChanged != nullptr)
        {
            onChanged(mTip);
        }
    }
}

Tooltip::Display::Display()
{
    addAndMakeVisible(mLabel);
    mServer.onChanged = [&](juce::String const& tip)
    {
        mLabel.setText(tip.upToFirstOccurrenceOf("\n", false, false), juce::NotificationType::dontSendNotification);
    };
    if(juce::Desktop::getInstance().getMainMouseSource().canHover())
    {
        mServer.startTimer(123);
    }
}

void Tooltip::Display::resized()
{
    mLabel.setBounds(getLocalBounds());
}

void Tooltip::BubbleClient::setTooltip(juce::String const& tooltip)
{
    mTooltip = tooltip;
}

juce::String Tooltip::BubbleClient::getTooltip() const
{
    return mTooltip;
}

Tooltip::BubbleWindow::BubbleWindow(juce::Component* owner, bool acceptsMouseDown)
: juce::Component("TooltipBubbleWindow")
, mAcceptsMouseDown(acceptsMouseDown)
{
    setAlwaysOnTop(true);

    if(owner != nullptr)
    {
        owner->addChildComponent(this);
    }
    if(juce::Desktop::getInstance().getMainMouseSource().canHover())
    {
        startTimer(25);
    }
}

Tooltip::BubbleWindow::~BubbleWindow()
{
    removeFromDesktop();
    setVisible(false);
}

void Tooltip::BubbleWindow::paint(juce::Graphics& g)
{
    auto constexpr cornerSize = 5.0f;
    auto bounds = getLocalBounds().toFloat();
    g.setColour(findColour(juce::TooltipWindow::ColourIds::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerSize);

    g.setColour(findColour(juce::TooltipWindow::ColourIds::outlineColourId));
    g.drawRoundedRectangle(bounds.reduced(0.5f, 0.5f), cornerSize, 1.0f);

    static auto constexpr offset = 8.0f;
    auto const font = juce::Font(juce::FontOptions(13.0f)).withHorizontalScale(1.0f);
    auto const fontHeight = font.getHeight();

    auto const lines = juce::StringArray::fromLines(mTooltip);
    auto const textColour = findColour(juce::TooltipWindow::ColourIds::textColourId);
    auto const maxWidth = 480.0f - offset * 2;
    auto const maxHeight = std::min(bounds.getHeight() - offset * 2.0f, static_cast<float>(620 - offset * 2));
    auto const maxLines = static_cast<int>(std::floor(maxHeight / fontHeight));
    bounds.reduce(offset, offset);

    auto lineIndex = 0;
    juce::AttributedString attributedString;
    attributedString.setJustification(juce::Justification::topLeft);
    juce::TextLayout tl;
    while(lineIndex < lines.size())
    {
        auto subIndex = 0;
        while(lineIndex < lines.size() && subIndex <= maxLines)
        {
            auto const br = lineIndex <= lines.size() && subIndex <= maxLines - 1;
            attributedString.append(lines.getReference(lineIndex) + (br ? "\n" : ""), font, textColour);
            ++lineIndex;
            ++subIndex;
        }
        tl.createLayout(attributedString, std::min(bounds.getWidth(), maxWidth), maxHeight);
        tl.draw(g, bounds.removeFromLeft(std::ceil(tl.getWidth())).withHeight(tl.getHeight()));
        bounds.removeFromLeft(offset);
        attributedString.clear();
        attributedString.setJustification(juce::Justification::topLeft);
    }
}

void Tooltip::BubbleWindow::timerCallback()
{
    auto* owner = getParentComponent();
    auto const getTipFor = [&](juce::Component* component) -> juce::String
    {
        if(component == nullptr || component->isCurrentlyBlockedByAnotherModalComponent() || !juce::Process::isForegroundProcess())
        {
            return {};
        }
        for(auto* parent = component; parent != nullptr; parent = parent->getParentComponent())
        {
            if(dynamic_cast<BubbleClient*>(parent) != nullptr && (owner == nullptr || owner == parent || owner->isParentOf(parent)))
            {
                return dynamic_cast<BubbleClient*>(parent)->getTooltip();
            }
        }
        return {};
    };

    auto& desktop = juce::Desktop::getInstance();
    auto mouseSource = desktop.getMainMouseSource();
    mTooltip = getTipFor(mouseSource.isTouch() ? nullptr : mouseSource.getComponentUnderMouse());

    if((mouseSource.isDragging() && !mAcceptsMouseDown) || mTooltip.isEmpty())
    {
        if(isVisible())
        {
            mTooltip.clear();
            if(owner == nullptr)
            {
                removeFromDesktop();
            }
            setVisible(false);
        }
    }
    else
    {
        static auto constexpr offset = 8;
        auto const font = juce::Font(juce::FontOptions(13.0f)).withHorizontalScale(1.0f);
        auto const fontHeight = font.getHeight();
        auto const lines = juce::StringArray::fromLines(mTooltip);
        auto const screenPosition = mouseSource.getScreenPosition().toInt();
        auto const mousePosition = owner == nullptr ? screenPosition : owner->getLocalPoint(nullptr, screenPosition);
        auto const targetArea = owner == nullptr ? desktop.getDisplays().getDisplayForPoint(mousePosition)->userArea : owner->getLocalBounds();
        auto const maxHeight = std::min(static_cast<float>(targetArea.getHeight() - offset * 2), static_cast<float>(620 - offset * 2));
        auto const maxLines = static_cast<int>(std::floor(maxHeight / fontHeight));
        auto maxWidth = std::min(static_cast<float>(targetArea.getWidth() - offset * 2), static_cast<float>(480 - offset * 2));

        auto width = 0;
        auto height = 0;
        auto lineIndex = 0;
        juce::AttributedString attributedString;
        attributedString.setJustification(juce::Justification::topLeft);
        juce::TextLayout tl;
        while(lineIndex < lines.size())
        {
            auto subIndex = 0;
            while(lineIndex < lines.size() && subIndex <= maxLines)
            {
                auto const br = lineIndex <= lines.size() && subIndex <= maxLines - 1;
                attributedString.append(lines.getReference(lineIndex) + (br ? "\n" : ""), font);
                ++lineIndex;
                ++subIndex;
            }
            tl.createLayout(attributedString, maxWidth, maxHeight);
            auto const w = static_cast<int>(std::ceil(tl.getWidth()));
            auto const h = static_cast<int>(std::ceil(tl.getHeight()));
            width += (w + offset);
            height = std::max(height, h);
            maxWidth -= static_cast<float>(w);
            attributedString.clear();
            attributedString.setJustification(juce::Justification::topLeft);
        }
        width += offset * 2;
        height += offset * 2;

        auto const x = mousePosition.x > targetArea.getCentreX() ? mousePosition.x - (width + offset) : mousePosition.x + offset;
        auto const y = mousePosition.y > targetArea.getCentreY() ? mousePosition.y - (height + offset) : mousePosition.y + offset + 1;
        auto const bounds = juce::Rectangle<int>(x, y, width, height).constrainedWithin(targetArea);
        setBounds(bounds);
        if(owner == nullptr)
        {
            addToDesktop(juce::ComponentPeer::windowHasDropShadow | juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowIgnoresKeyPresses | juce::ComponentPeer::windowIgnoresMouseClicks);
        }
        else
        {
            toFront(false);
        }
        setVisible(true);
        repaint();
    }
}

MISC_FILE_END
