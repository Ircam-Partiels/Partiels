#include "AnlGroupThumbnail.h"
#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::Thumbnail::Thumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    addAndMakeVisible(mExportButton);
    addAndMakeVisible(mStateButton);
    addAndMakeVisible(mRemoveButton);
    addAndMakeVisible(mExpandButton);
    addAndMakeVisible(mDropdownButton);
    
    mExportButton.setTooltip(juce::translate("Export the group"));
    mRemoveButton.setTooltip(juce::translate("Remove the group"));
    mExpandButton.setTooltip(juce::translate("Expand the group"));
    mDropdownButton.setTooltip(juce::translate("Show group actions menu"));
    
    mRemoveButton.onClick = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    
    mExpandButton.onClick = [&]()
    {
        mAccessor.setAttr<AttrType::expanded>(!mAccessor.getAttr<AttrType::expanded>(), NotificationType::synchronous);
    };
    
    mExportButton.onClick = [&]()
    {
        // Force to repaint to update the state
        mExportButton.setState(juce::Button::ButtonState::buttonNormal);
    };
    
    mDropdownButton.onClick = [&]()
    {
        juce::PopupMenu menu;
        auto addItem = [&](juce::Button& button)
        {
            if(!button.isVisible())
            {
                menu.addItem(button.getTooltip(), button.onClick);
            }
        };
        addItem(mExportButton);
        addItem(mRemoveButton);
        addItem(mExpandButton);
        menu.showAt(&mDropdownButton);
    };
    
    mListener.onAttrChanged = [&](Group::Accessor const& acsr, AttrType const attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::identifier:
                break;
            case AttrType::name:
            {
                setTooltip(acsr.getAttr<AttrType::name>());
                repaint();
            }
                break;
            case AttrType::height:
            case AttrType::colour:
            case AttrType::layout:
            case AttrType::tracks:
                break;
            case AttrType::expanded:
            {
                lookAndFeelChanged();
            }
                break;
        }
    };
    
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Group::Thumbnail::~Thumbnail()
{
    mAccessor.removeListener(mListener);
}

void Group::Thumbnail::resized()
{
    bool useDropdown = false;
    auto bounds = getLocalBounds().withTrimmedLeft(getWidth() / 2);
    auto constexpr separator = 2;
    auto const size = bounds.getWidth() - separator;
    
    auto layoutButton = [&](juce::Component& component)
    {
        useDropdown = bounds.getHeight() < size * 2;
        component.setVisible(!useDropdown);
        if(!useDropdown)
        {
            component.setBounds(bounds.removeFromBottom(size).reduced(separator));
        }
    };
    
    layoutButton(mExpandButton);
    layoutButton(mRemoveButton);
    layoutButton(mStateButton);
    layoutButton(mExportButton);
    mDropdownButton.setVisible(useDropdown);
    if(useDropdown)
    {
        mDropdownButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    }
}

void Group::Thumbnail::paint(juce::Graphics& g)
{
    auto constexpr separator = 2;
    auto constexpr rotation = -1.5707963268f;
    
    auto const width = getWidth() / 2;
    auto const height = getHeight();
    auto const bottom = height - 2 * separator;
    auto const size = height - 4 * separator;
    
    g.setColour(findColour(ColourIds::titleBackgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().removeFromLeft(width).toFloat(), 2.0f);
    
    g.setColour(findColour(ColourIds::textColourId));
    g.addTransform(juce::AffineTransform::rotation(rotation, 0.0f, static_cast<float>(bottom)));
    g.drawFittedText(mAccessor.getAttr<AttrType::name>(), 0, bottom, size, width, juce::Justification::centredLeft, 1, 1.0f);
}

void Group::Thumbnail::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mDropdownButton, IconManager::IconType::chevron);
        laf->setButtonIcon(mExportButton, IconManager::IconType::share);
        laf->setButtonIcon(mRemoveButton, IconManager::IconType::cancel);
        if(mAccessor.getAttr<AttrType::expanded>())
        {
            laf->setButtonIcon(mExpandButton, IconManager::IconType::shrink);
            mExpandButton.setTooltip(juce::translate("Shrink the analyses"));
        }
        else
        {
            laf->setButtonIcon(mExpandButton, IconManager::IconType::expand);
            mExpandButton.setTooltip(juce::translate("Expand the analyses"));
        }
    }
}

void Group::Thumbnail::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

void Group::Thumbnail::mouseDrag(juce::MouseEvent const& event)
{
    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    auto* parent = findParentComponentOfClass<StrechableSection>();
    anlWeakAssert(dragContainer != nullptr && parent != nullptr);
    if(dragContainer != nullptr && !dragContainer->isDragAndDropActive() && parent != nullptr)
    {
        juce::Image snapshot(juce::Image::ARGB, parent->getWidth(), parent->getHeight(), true);
        juce::Graphics g(snapshot);
        g.beginTransparencyLayer(0.6f);
        parent->paintEntireComponent(g, false);
        g.endTransparencyLayer();
        
        auto const p = -event.getMouseDownPosition();
        dragContainer->startDragging(DraggableTable::createDescription(event, "Group", mAccessor.getAttr<AttrType::identifier>()), parent, snapshot, true, &p, &event.source);
    }
}

ANALYSE_FILE_END
