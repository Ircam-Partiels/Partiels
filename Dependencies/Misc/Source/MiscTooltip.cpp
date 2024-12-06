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
    auto const bounds = getLocalBounds().toFloat();
    auto const font = juce::Font(juce::FontOptions(13.0f)).withHorizontalScale(1.0f);

    g.setColour(findColour(juce::TooltipWindow::ColourIds::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerSize);

    g.setColour(findColour(juce::TooltipWindow::ColourIds::outlineColourId));
    g.drawRoundedRectangle(bounds.reduced(0.5f, 0.5f), cornerSize, 1.0f);

    juce::AttributedString attributedString;
    attributedString.setJustification(juce::Justification::centredLeft);
    attributedString.append(mTooltip, font, findColour(juce::TooltipWindow::ColourIds::textColourId));

    juce::TextLayout tl;
    tl.createLayoutWithBalancedLineLengths(attributedString, 500.0f);
    tl.draw(g, bounds.reduced(4.0f, 4.0f));
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
        auto const mousePosition = mouseSource.getScreenPosition().toInt();
        if(owner == nullptr)
        {
            auto const parentArea = desktop.getDisplays().getDisplayForPoint(mousePosition)->userArea;
            setBounds(getLookAndFeel().getTooltipBounds(mTooltip, mousePosition, parentArea));
            addToDesktop(juce::ComponentPeer::windowHasDropShadow | juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowIgnoresKeyPresses | juce::ComponentPeer::windowIgnoresMouseClicks);
        }
        else
        {
            toFront(false);
            setBounds(getLookAndFeel().getTooltipBounds(mTooltip, owner->getLocalPoint(nullptr, mousePosition), owner->getLocalBounds()));
        }
        setVisible(true);
        repaint();
    }
}

MISC_FILE_END
