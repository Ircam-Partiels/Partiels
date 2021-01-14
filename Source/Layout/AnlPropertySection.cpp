#include "AnlPropertySection.h"

ANALYSE_FILE_BEGIN

void Layout::PropertySection::Header::paint(juce::Graphics& g)
{
    auto const* lookAndFeel = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(lookAndFeel != nullptr);
    if(lookAndFeel == nullptr)
    {
        return;
    }
    
    auto bounds = getLocalBounds();
    auto const isMouseDown = isMouseButtonDown();
    auto const isMouseOver = juce::Component::isMouseOver();
    auto const& section = *static_cast<PropertySection*>(getParentComponent());
    auto const font = lookAndFeel->getHeaderFont(section, getHeight());
    lookAndFeel->drawHeaderBackground(g, section, bounds, isMouseDown, isMouseOver);
    lookAndFeel->drawHeaderButton(g, section, bounds.removeFromRight(getHeight()), section.mSizeRatio, isMouseDown, isMouseOver);
    lookAndFeel->drawHeaderTitle(g, section, bounds, font, isMouseDown, isMouseOver);
}

void Layout::PropertySection::Header::mouseDown(juce::MouseEvent const& event)
{
    if(event.mods.isLeftButtonDown() && onClicked != nullptr)
    {
        onClicked();
    }
}

void Layout::PropertySection::Separator::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::separatorColourId, true));
}

Layout::PropertySection::PropertySection(juce::String const& title, bool resizeOnClick)
: mTitle(title)
{
    mHeader.onClicked = [&]()
    {
        setOpen(!mOpened, true);
    };
    mHeader.setVisible(title.isNotEmpty() || resizeOnClick);
    mHeader.setRepaintsOnMouseActivity(resizeOnClick);
    mHeader.setInterceptsMouseClicks(resizeOnClick, resizeOnClick);
    addChildComponent(mHeader);
    setOpen(true, false);
}

void Layout::PropertySection::resized()
{
    auto const* lookAndFeel = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(lookAndFeel != nullptr);
    
    auto constexpr maxSize = std::numeric_limits<int>::max();
    auto bounds = getLocalBounds().withHeight(maxSize);
    if(mHeader.isVisible())
    {
        mHeader.setBounds(bounds.removeFromTop(lookAndFeel != nullptr ? lookAndFeel->getHeaderHeight(*this) : 20));
    }
    
    auto const separatorSize = lookAndFeel != nullptr ? lookAndFeel->getSeparatorHeight(*this) : 2;
    for(auto& container : mContainers)
    {
        anlStrongAssert(container != nullptr);
        if(container != nullptr)
        {
            auto const panel = container.get()->panel;
            auto const containerSize = panel.get().getHeight();
            panel.get().setBounds(bounds.removeFromTop(containerSize));
            container.get()->separator.setBounds(bounds.removeFromTop(separatorSize));
        }
    }
    mContentsSize = bounds.getY() - separatorSize;
    
    if(onResized != nullptr)
    {
        onResized();
    }
}

juce::String Layout::PropertySection::getTitle() const
{
    return mTitle;
}

void Layout::PropertySection::setPanels(std::vector<PanelRef> const& panels)
{
    for(auto& container : mContainers)
    {
        if(container != nullptr)
        {
            removeChildComponent(&(container->panel.get()));
        }
    }
    
    mContainers.clear();
    mContainers.reserve(panels.size());
    for(auto panel : panels)
    {
        auto container = std::make_unique<Container>(panel);
        anlStrongAssert(container != nullptr);
        if(container != nullptr)
        {
            addAndMakeVisible(container.get()->panel);
            addAndMakeVisible(container.get()->separator);
            mContainers.push_back(std::move(container));
        }
    }
    resized();
}

void Layout::PropertySection::setOpen(bool isOpen, bool shouldAnimate)
{
    mOpened = isOpen;
    if(!isTimerRunning() && shouldAnimate)
    {
        startTimer(50);
    }
    else if(!shouldAnimate)
    {
        if(isTimerRunning())
        {
            stopTimer();
        }
        auto const newRatio = mOpened ? 1.0f : 0.0f;
        if(std::abs(newRatio - mSizeRatio) > std::numeric_limits<float>::epsilon())
        {
            mHeader.repaint();
        }
        mSizeRatio = newRatio;
        auto const contentSize = static_cast<int>(std::ceil(static_cast<double>(mContentsSize) * mSizeRatio));
        setSize(getWidth(), contentSize + (mHeader.isVisible() ? mHeader.getHeight() : 0));
    }
}

void Layout::PropertySection::timerCallback()
{
    auto constexpr increment = 0.1f;
    mSizeRatio = std::max(std::min(mSizeRatio + (mOpened ? increment : -increment), 1.0f), 0.0f);
    if((mOpened && mSizeRatio >= 1.0f) || (!mOpened && mSizeRatio <= 0.0f))
    {
        stopTimer();
    }
    mHeader.repaint();
    auto const contentSize = static_cast<int>(std::ceil(static_cast<double>(mContentsSize) * mSizeRatio));
    setSize(getWidth(), contentSize + (mHeader.isVisible() ? mHeader.getHeight() : 0));
}


ANALYSE_FILE_END
