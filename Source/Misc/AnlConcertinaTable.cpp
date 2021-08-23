#include "AnlConcertinaTable.h"

ANALYSE_FILE_BEGIN

void ConcertinaTable::Header::paint(juce::Graphics& g)
{
    auto const* laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf == nullptr)
    {
        return;
    }

    auto bounds = getLocalBounds();
    auto const isMouseDown = isMouseButtonDown();
    auto const isMouseOver = juce::Component::isMouseOver();
    auto const& section = *static_cast<ConcertinaTable*>(getParentComponent());
    auto const font = laf->getHeaderFont(section, getHeight());
    laf->drawHeaderBackground(g, section, bounds, isMouseDown, isMouseOver);
    laf->drawHeaderButton(g, section, bounds.removeFromRight(getHeight()), section.mSizeRatio, isMouseDown, isMouseOver);
    laf->drawHeaderTitle(g, section, bounds, font, isMouseDown, isMouseOver);
}

void ConcertinaTable::Header::mouseDown(juce::MouseEvent const& event)
{
    if(event.mods.isLeftButtonDown() && onClicked != nullptr)
    {
        onClicked();
    }
}

ConcertinaTable::ConcertinaTable(juce::String const& title, bool resizeOnClick, juce::String const& tooltip)
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

    mComponentListener.onComponentResized = [this](juce::Component& component)
    {
        juce::ignoreUnused(component);
        auto const* laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());
        anlWeakAssert(laf != nullptr);
        auto const fullSize = std::accumulate(mContents.cbegin(), mContents.cend(), 0, [](int value, auto const& content)
                                              {
                                                  anlWeakAssert(content != nullptr);
                                                  if(content != nullptr)
                                                  {
                                                      return value + content->getHeight();
                                                  }
                                                  return value;
                                              });

        auto const contentSize = static_cast<int>(std::ceil(static_cast<double>(fullSize) * mSizeRatio));
        auto const headerHeight = laf != nullptr ? laf->getHeaderHeight(*this) : 20;
        setSize(getWidth(), contentSize + (mHeader.isVisible() ? headerHeight : 0));
    };

    setTooltip(tooltip);
    setOpen(true, false);
}

ConcertinaTable::~ConcertinaTable()
{
    setComponents({});
}

void ConcertinaTable::resized()
{
    auto const* laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);

    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    if(mHeader.isVisible())
    {
        mHeader.setBounds(bounds.removeFromTop(laf != nullptr ? laf->getHeaderHeight(*this) : 20));
    }

    for(auto& content : mContents)
    {
        anlWeakAssert(content != nullptr);
        if(content != nullptr)
        {
            auto const containerSize = content->getHeight();
            content->setBounds(bounds.removeFromTop(containerSize));
        }
    }
}

juce::String ConcertinaTable::getTitle() const
{
    return mTitle;
}

void ConcertinaTable::setComponents(std::vector<ComponentRef> const& components)
{
    for(auto& content : mContents)
    {
        if(content != nullptr)
        {
            mComponentListener.detachFrom(*content.getComponent());
            removeChildComponent(content);
        }
    }

    mContents.clear();
    mContents.reserve(components.size());
    for(auto content : components)
    {
        mComponentListener.attachTo(content.get());
        addAndMakeVisible(content.get());
        mContents.emplace_back(&content.get());
    }
    mComponentListener.onComponentResized(*this);
    resized();
}

std::vector<juce::Component::SafePointer<juce::Component>> ConcertinaTable::getComponents()
{
    return mContents;
}

void ConcertinaTable::setOpen(bool isOpen, bool shouldAnimate)
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
        mComponentListener.onComponentResized(*this);
    }
}

bool ConcertinaTable::isOpen() const
{
    return mOpened;
}

void ConcertinaTable::timerCallback()
{
    auto constexpr increment = 0.1f;
    mSizeRatio = std::max(std::min(mSizeRatio + (mOpened ? increment : -increment), 1.0f), 0.0f);
    if((mOpened && mSizeRatio >= 1.0f) || (!mOpened && mSizeRatio <= 0.0f))
    {
        stopTimer();
    }
    mHeader.repaint();
    mComponentListener.onComponentResized(*this);
}

ANALYSE_FILE_END
