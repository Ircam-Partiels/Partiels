#include "AnlConcertinaPanel.h"

ANALYSE_FILE_BEGIN

void ConcertinaPanel::Header::paint(juce::Graphics& g)
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
    auto const& section = *static_cast<ConcertinaPanel*>(getParentComponent());
    auto const font = lookAndFeel->getHeaderFont(section, getHeight());
    lookAndFeel->drawHeaderBackground(g, section, bounds, isMouseDown, isMouseOver);
    lookAndFeel->drawHeaderButton(g, section, bounds.removeFromRight(getHeight()), section.mSizeRatio, isMouseDown, isMouseOver);
    lookAndFeel->drawHeaderTitle(g, section, bounds, font, isMouseDown, isMouseOver);
}

void ConcertinaPanel::Header::mouseDown(juce::MouseEvent const& event)
{
    if(event.mods.isLeftButtonDown() && onClicked != nullptr)
    {
        onClicked();
    }
}

void ConcertinaPanel::Separator::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::separatorColourId, true));
}

ConcertinaPanel::Container::Container(ComponentRef ref)
: component(&(ref.get()))
{
}

ConcertinaPanel::ConcertinaPanel(juce::String const& title, bool resizeOnClick, juce::String const& tooltip)
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
    setTooltip(tooltip);
    setOpen(true, false);
}

void ConcertinaPanel::resized()
{
    auto const* lookAndFeel = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(lookAndFeel != nullptr);
    
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
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
            auto const component = container.get()->component.getComponent();
            anlWeakAssert(component != nullptr);
            if(component != nullptr)
            {
                auto const containerSize = component->getHeight();
                component->setBounds(bounds.removeFromTop(containerSize));
                container.get()->separator.setBounds(bounds.removeFromTop(separatorSize));
            }
        }
    }
    
    if(onResized != nullptr)
    {
        onResized();
    }
}

juce::String ConcertinaPanel::getTitle() const
{
    return mTitle;
}

void ConcertinaPanel::setComponents(std::vector<ComponentRef> const& components)
{
    for(auto& container : mContainers)
    {
        if(container != nullptr)
        {
            removeChildComponent(&(container.get()->separator));
            auto const component = container.get()->component.getComponent();
            if(component != nullptr)
            {
                component->removeComponentListener(this);
                removeChildComponent(component);
            }
        }
    }
    
    mContainers.clear();
    mContainers.reserve(components.size());
    for(auto component : components)
    {
        auto container = std::make_unique<Container>(component);
        anlStrongAssert(container != nullptr);
        if(container != nullptr)
        {
            component.get().addComponentListener(this);
            addAndMakeVisible(container.get()->component);
            addAndMakeVisible(container.get()->separator);
            mContainers.push_back(std::move(container));
        }
    }
    componentMovedOrResized(*this, false, true);
    resized();
}

void ConcertinaPanel::setOpen(bool isOpen, bool shouldAnimate)
{
    mOpened = isOpen;
    if(!isTimerRunning() && shouldAnimate)
    {
        startTimer(20);
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
        componentMovedOrResized(*this, false, true);
    }
}

void ConcertinaPanel::componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized)
{
    juce::ignoreUnused(component, wasMoved);
    if(wasResized)
    {
        auto const* lookAndFeel = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());
        anlWeakAssert(lookAndFeel != nullptr);
        
        auto const separatorSize = lookAndFeel != nullptr ? lookAndFeel->getSeparatorHeight(*this) : 2;
        auto const fullSize = std::accumulate(mContainers.cbegin(), mContainers.cend(), 0, [separatorSize](int value, auto const& container)
        {
            anlStrongAssert(container != nullptr);
            if(container != nullptr)
            {
                auto const content = container.get()->component.getComponent();
                anlStrongAssert(content != nullptr);
                if(content != nullptr)
                {
                    return value + content->getHeight() + separatorSize;
                }
            }
            return value;
        });
        
        auto const contentSize = static_cast<int>(std::ceil(static_cast<double>(fullSize) * mSizeRatio));
        auto const headerHeight = lookAndFeel != nullptr ? lookAndFeel->getHeaderHeight(*this) : 20;
        setSize(getWidth(), contentSize + (mHeader.isVisible() ? headerHeight : 0));
    }
}

void ConcertinaPanel::timerCallback()
{
    auto constexpr increment = 0.1f;
    mSizeRatio = std::max(std::min(mSizeRatio + (mOpened ? increment : -increment), 1.0f), 0.0f);
    if((mOpened && mSizeRatio >= 1.0f) || (!mOpened && mSizeRatio <= 0.0f))
    {
        stopTimer();
    }
    mHeader.repaint();
    componentMovedOrResized(*this, false, true);
}

ANALYSE_FILE_END
