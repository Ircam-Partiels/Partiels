#include "AnlPropertyLayout.h"

ANALYSE_FILE_BEGIN

Tools::PropertyLayout::Container::Container(PanelInfo const& p) : panelInfos(p)
{
}

void Tools::PropertyLayout::Separator::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::separatorColourId, true));
}

Tools::PropertyLayout::PropertyLayout()
{
    mViewport.setScrollBarThickness(6);
    mViewport.setViewedComponent(&mContent, false);
    addAndMakeVisible(mViewport);
}

void Tools::PropertyLayout::resized()
{
    mViewport.setBounds(getLocalBounds());
}

void Tools::PropertyLayout::setPanels(std::vector<PanelInfo> const& panels)
{
    mContent.removeAllChildren();
    mContainers.clear();
    mContainers.reserve(panels.size());
    for(auto panel : panels)
    {
        auto container = std::make_unique<Container>(panel);
        anlStrongAssert(container != nullptr);
        if(container != nullptr)
        {
            mContent.addAndMakeVisible(std::get<0>(container->panelInfos));
            mContent.addAndMakeVisible(container->separator);
            mContainers.push_back(std::move(container));
        }
    }
}

void Tools::PropertyLayout::organizePanels(Orientation orientation, int size)
{
    auto constexpr separatorSize = 2;
    auto const isVertical = orientation == Orientation::vertical;
    juce::Rectangle<int> bounds {0, 0, isVertical ? size : std::numeric_limits<int>::max(), !isVertical ? size : std::numeric_limits<int>::max()};
    for(auto& container : mContainers)
    {
        anlStrongAssert(container != nullptr);
        if(container != nullptr)
        {
            auto const panel = std::get<0>(container->panelInfos);
            auto const containerSize = isVertical ? panel.get().getHeight() : panel.get().getWidth();
            panel.get().setBounds(isVertical ? bounds.removeFromTop(containerSize) : bounds.removeFromLeft(containerSize));
            auto localBounds = panel.get().getLocalBounds();
            
            auto& title = panel.get().title;
            auto const font = title.getLookAndFeel().getLabelFont(title).withHorizontalScale(title.getMinimumHorizontalScale());
            auto const borderSize = title.getLookAndFeel().getLabelBorderSize(title);
            
            auto const titleSize = isVertical ? font.getStringWidthFloat(title.getText()) + borderSize.getLeft() + borderSize.getRight() : font.getHeight();
            auto const labelSize = std::min(std::get<1>(container->panelInfos), static_cast<int>(std::ceil(titleSize)));
            panel.get().title.setBounds(isVertical ? localBounds.removeFromLeft(labelSize) : localBounds.removeFromTop(labelSize));
            panel.get().content->setBounds(localBounds);
            container->separator.setBounds(isVertical ? bounds.removeFromTop(separatorSize) : bounds.removeFromLeft(separatorSize));
        }
    }
    mContent.setSize(isVertical ? size : bounds.getX() - separatorSize, !isVertical ? size : bounds.getY() - separatorSize);
    mViewport.setScrollBarsShown(isVertical, !isVertical, isVertical, !isVertical);
}

ANALYSE_FILE_END
