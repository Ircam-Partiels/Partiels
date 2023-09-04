#include "AnlHideablePanel.h"

ANALYSE_FILE_BEGIN

HideablePanel::Packager::Packager(juce::String const& title, juce::Component& content)
: mTitle("Title", title)
, mContent(content)
{
    mTitle.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mTitle);
    addAndMakeVisible(mSeparator);
    addAndMakeVisible(mContent);
}

void HideablePanel::Packager::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void HideablePanel::Packager::resized()
{
    auto bounds = getLocalBounds();
    mTitle.setBounds(bounds.removeFromTop(24));
    mSeparator.setBounds(bounds.removeFromTop(1));
    mContent.setBounds(bounds);
}

void HideablePanel::Packager::colourChanged()
{
    setOpaque(findColour(juce::ResizableWindow::backgroundColourId).isOpaque());
}

void HideablePanel::Packager::parentHierarchyChanged()
{
    colourChanged();
}

HideablePanel::Container::Container(juce::String const& title, juce::Component& content)
: mPackager(title, content)
, mContent(content)
{
    mListener.onComponentResized = [this](juce::Component& component)
    {
        setSize(component.getWidth() + shadowRadius * 2, component.getHeight() + 25 + shadowRadius * 2);
    };
    mListener.attachTo(mContent);
    mListener.onComponentResized(mContent);
    setComponentEffect(&mDropShadowEffect);
    addAndMakeVisible(mPackager);
}

HideablePanel::Container::~Container()
{
    mListener.detachFrom(mContent);
}

void HideablePanel::Container::resized()
{
    mPackager.setBounds(getLocalBounds().reduced(8));
}

void HideablePanel::Container::colourChanged()
{
    mDropShadowEffect.setShadowProperties({juce::Colours::black.withAlpha(0.5f), shadowRadius, {0, 0}});
}

void HideablePanel::Container::parentHierarchyChanged()
{
    colourChanged();
}

HideablePanel::HideablePanel()
{
}

void HideablePanel::resized()
{
    if(mContainer != nullptr)
    {
        mContainer->setBounds(getLocalBounds());
    }
}

void HideablePanel::childBoundsChanged([[maybe_unused]] juce::Component* child)
{
    MiscWeakAssert(child == mContainer.get());
    if(mContainer != nullptr)
    {
        setSize(mContainer->getWidth(), mContainer->getHeight());
    }
}

void HideablePanel::inputAttemptWhenModal()
{
    if(auto* parent = dynamic_cast<HideablePanelManager*>(getParentComponent()))
    {
        parent->hide();
    }
}

void HideablePanel::setContent(juce::String const& title, juce::Component* content)
{
    if(content == nullptr)
    {
        mContainer.reset();
    }
    else
    {
        mContainer = std::make_unique<Container>(title, *content);
        addAndMakeVisible(mContainer.get());
        if(mContainer != nullptr)
        {
            setSize(mContainer->getWidth(), mContainer->getHeight());
        }
    }
}

HideablePanelManager::HideablePanelManager()
{
    mComponentListener.onComponentMoved = [this]([[maybe_unused]] juce::Component& component)
    {
        resized();
    };

    mComponentListener.onComponentResized = [this]([[maybe_unused]] juce::Component& component)
    {
        resized();
    };
}

HideablePanelManager::~HideablePanelManager()
{
    setContent(nullptr, {});
}

void HideablePanelManager::resized()
{
    auto bounds = getLocalBounds();
    if(mBackground != nullptr)
    {
        mBackground->setBounds(bounds);
    }
    for(auto& panel : mHideablePanels)
    {
        panel.get().setBounds(bounds.withSizeKeepingCentre(panel.get().getWidth(), panel.get().getHeight()));
    }
}

void HideablePanelManager::setContent(juce::Component* background, std::vector<std::reference_wrapper<HideablePanel>> panels)
{
    for(auto& panel : mHideablePanels)
    {
        mComponentListener.detachFrom(panel.get());
    }
    mHideablePanels = std::move(panels);
    mBackground = background;
    addAndMakeVisible(mBackground, 0);
    for(auto& panel : mHideablePanels)
    {
        mComponentListener.attachTo(panel.get());
        addChildComponent(panel.get());
    }
    resized();
}

void HideablePanelManager::show(HideablePanel& panel)
{
    if(std::none_of(mHideablePanels.cbegin(), mHideablePanels.cend(), [&](auto const cpanel)
                    {
                        return std::addressof(cpanel.get()) != std::addressof(panel);
                    }))
    {
        return;
    }
    auto& desktop = juce::Desktop::getInstance();

    mWindows.clear();
    auto const numWindows = desktop.getNumComponents();
    for(auto index = 0; index < numWindows; ++index)
    {
        auto* window = desktop.getComponent(index);
        if(window != nullptr && !window->isParentOf(mBackground) && window->isOnDesktop() && window->isVisible())
        {
            mWindows.emplace_back(window);
            window->setVisible(false);
        }
    }

    auto& animator = desktop.getAnimator();
    if(mBackground != nullptr)
    {
        animator.animateComponent(mBackground, mBackground->getBounds(), 0.25f, fadeTime, true, 1.0, 1.0);
    }
    auto delay = false;
    for(auto& cpanel : mHideablePanels)
    {
        if(std::addressof(cpanel.get()) != std::addressof(panel))
        {
            if(cpanel.get().isCurrentlyModal())
            {
                exitModalState();
            }
            delay = delay || cpanel.get().isVisible();
            animator.fadeOut(std::addressof(cpanel.get()), fadeTime);
        }
    }
    if(delay)
    {
        juce::WeakReference<juce::Component> weakReference(std::addressof(panel));
        juce::Timer::callAfterDelay(100, [weakReference, &panel]()
                                    {
                                        if(weakReference.get() == nullptr)
                                        {
                                            return;
                                        }
                                        juce::Desktop::getInstance().getAnimator().fadeIn(std::addressof(panel), fadeTime);
                                        panel.toFront(true);
                                        if(!panel.isCurrentlyModal())
                                        {
                                            panel.enterModalState();
                                        }
                                    });
    }
    else
    {
        juce::Desktop::getInstance().getAnimator().fadeIn(std::addressof(panel), fadeTime);
        panel.toFront(true);
        if(!panel.isCurrentlyModal())
        {
            panel.enterModalState();
        }
    }
}

void HideablePanelManager::hide()
{
    auto& animator = juce::Desktop::getInstance().getAnimator();
    if(mBackground != nullptr)
    {
        animator.animateComponent(mBackground, mBackground->getBounds(), 1.0f, fadeTime, true, 1.0, 1.0);
    }
    for(auto& cpanel : mHideablePanels)
    {
        if(cpanel.get().isCurrentlyModal())
        {
            cpanel.get().exitModalState();
        }
        animator.fadeOut(std::addressof(cpanel.get()), fadeTime);
    }
    for(auto& window : mWindows)
    {
        if(window.get() != nullptr)
        {
            window->setVisible(true);
        }
    }
}

ANALYSE_FILE_END
