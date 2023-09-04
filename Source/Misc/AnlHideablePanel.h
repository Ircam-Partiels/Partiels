#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

class HideablePanel
: public juce::Component
{
public:
    static auto constexpr shadowRadius = 8;

    HideablePanel();
    ~HideablePanel() override = default;

    void setContent(juce::String const& title, juce::Component* content);

    // juce::Component
    void resized() override;
    void childBoundsChanged(juce::Component* child) override;
    void inputAttemptWhenModal() override;

private:
    class Packager
    : public juce::Component
    {
    public:
        Packager(juce::String const& title, juce::Component& content);
        ~Packager() override = default;

        // juce::Component
        void paint(juce::Graphics& g) override;
        void resized() override;
        void colourChanged() override;
        void parentHierarchyChanged() override;

    protected:
        juce::Label mTitle;
        ColouredPanel mSeparator;
        juce::Component& mContent;
    };

    class Container
    : public juce::Component
    {
    public:
        Container(juce::String const& title, juce::Component& content);
        ~Container() override;

        // juce::Component
        void resized() override;
        void colourChanged() override;
        void parentHierarchyChanged() override;

    private:
        Packager mPackager;
        juce::Component& mContent;
        juce::DropShadowEffect mDropShadowEffect;
        Misc::ComponentListener mListener;
    };

    std::unique_ptr<juce::Component> mContainer;
};

template <class T>
class HideablePanelTyped
: public HideablePanel
{
public:
    HideablePanelTyped(juce::String const& title)
    {
        setContent(title, &mContent);
    }

    ~HideablePanelTyped() override
    {
        setContent("", nullptr);
    }

protected:
    T mContent;
};

class HideablePanelManager
: public juce::Component
{
public:
    static auto constexpr fadeTime = 250;
    HideablePanelManager();
    ~HideablePanelManager() override;

    void setContent(juce::Component* background, std::vector<std::reference_wrapper<HideablePanel>> panels);
    void show(HideablePanel& panel);
    void hide();

    // juce::Component
    void resized() override;

private:
    ComponentListener mComponentListener;
    juce::Component* mBackground;
    std::vector<std::reference_wrapper<HideablePanel>> mHideablePanels;
    std::vector<juce::WeakReference<juce::Component>> mWindows;
};

ANALYSE_FILE_END
