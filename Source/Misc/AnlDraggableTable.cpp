#include "AnlDraggableTable.h"

ANALYSE_FILE_BEGIN

DraggableTable::DraggableTable(juce::String const& tooltip)
{
    setTooltip(tooltip);
}

void DraggableTable::resized()
{
    auto bounds = getLocalBounds();
    for(auto& content : mContents)
    {
        anlWeakAssert(content != nullptr);
        if(content != nullptr)
        {
            content->setBounds(bounds.removeFromTop(content->getHeight()));
        }
    }
}

void DraggableTable::setComponents(std::vector<ComponentRef> const& components)
{
    for(auto& content : mContents)
    {
        if(content != nullptr)
        {
            content->removeComponentListener(this);
            removeChildComponent(content);
        }
    }
    
    mContents.clear();
    mContents.reserve(components.size());
    for(auto& content : components)
    {
        content.get().addComponentListener(this);
        addAndMakeVisible(content.get());
        mContents.emplace_back(&content.get());
    }
    componentMovedOrResized(*this, false, true);
    resized();
}

void DraggableTable::componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized)
{
    juce::ignoreUnused(component, wasMoved);
    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    if(wasResized && (dragContainer == nullptr || !dragContainer->isDragAndDropActive()))
    {
        auto const fullSize = std::accumulate(mContents.cbegin(), mContents.cend(), 0, [](int value, auto const& content)
        {
            return (content != nullptr) ? value + content->getHeight() : value;
        });
        setSize(getWidth(), fullSize);
    }
}


bool DraggableTable::isInterestedInDragSource(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    if(mContents.size() <= 1)
    {
        return false;
    }
    auto* source = dragSourceDetails.sourceComponent.get();
    auto* obj = dragSourceDetails.description.getDynamicObject();
    if(source == nullptr || obj == nullptr)
    {
        return false;
    }
    return obj->getProperty("type").toString() == "DraggableTable::Content";
}

void DraggableTable::itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* source = dragSourceDetails.sourceComponent.get();
    auto* obj = dragSourceDetails.description.getDynamicObject();
    anlWeakAssert(obj != nullptr && source != nullptr);
    if(obj == nullptr || source == nullptr)
    {
        return;
    }
    auto const fullSize = std::accumulate(mContents.cbegin(), mContents.cend(), 0, [](int value, auto const& content)
    {
        return (content != nullptr) ? value + content->getHeight() : value;
    });
    setSize(getWidth(), fullSize + source->getHeight());
    source->setAlpha(0.4f);
}

void DraggableTable::itemDragMove(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* obj = dragSourceDetails.description.getDynamicObject();
    auto* source = dragSourceDetails.sourceComponent.get();
    anlWeakAssert(obj != nullptr && source != nullptr);
    if(obj == nullptr || source == nullptr)
    {
        return;
    }
    
    auto offset = static_cast<int>(obj->getProperty("offset"));
    auto const it = std::find_if(mContents.cbegin(), mContents.cend(), [source](auto const& content)
    {
        return content.getComponent() == source;
    });
    anlWeakAssert(it != mContents.cend());
    if(it == mContents.cend())
    {
        return;
    }
    
    auto constexpr pi = 3.14159265358979323846;
    auto const y = dragSourceDetails.localPosition.getY() + offset;
    auto const sourceHeight = source->getHeight();
    auto const index = static_cast<size_t>(std::distance(mContents.cbegin(), it));
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    for(size_t i = 0; i < mContents.size(); ++i)
    {
        if(mContents[i] != nullptr)
        {
            if(i != index && i != index + 1)
            {
                auto const distance = static_cast<double>(std::abs(bounds.getY() - y)) / static_cast<double>(sourceHeight + 2);
                auto const scale = 1.0 - std::max((0.8 - std::cos(std::min(distance * pi, pi))) / 1.8, 0.0);
                bounds.removeFromTop(static_cast<int>(std::ceil(scale * static_cast<double>(sourceHeight + 2))));
            }
            mContents[i]->setBounds(bounds.removeFromTop(mContents[i]->getHeight()));
        }
    }
    
    // Scroll the viewport if this is necessary
    JUCE_COMPILER_WARNING("check if we should keep that here")
    if(auto* viewport = findParentComponentOfClass<juce::Viewport>())
    {
        auto const position = viewport->getMouseXYRelative();
        if(viewport->autoScroll(viewport->getWidth() / 2, position.getY(), 20, 2))
        {
            juce::Component::beginDragAutoRepeat(20);
        }
    }
}

void DraggableTable::itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto const fullSize = std::accumulate(mContents.cbegin(), mContents.cend(), 0, [](int value, auto const& content)
    {
        return (content != nullptr) ? value + content->getHeight() : value;
    });
    setSize(getWidth(), fullSize);
    auto* source = dragSourceDetails.sourceComponent.get();
    anlWeakAssert(source != nullptr);
    if(source != nullptr)
    {
        source->setAlpha(1.0f);
    }
}

void DraggableTable::itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* obj = dragSourceDetails.description.getDynamicObject();
    auto* source = dragSourceDetails.sourceComponent.get();
    anlWeakAssert(obj != nullptr && source != nullptr);
    if(obj == nullptr || source == nullptr)
    {
        return;
    }
    
    auto offset = static_cast<int>(obj->getProperty("offset"));
    auto const it = std::find_if(mContents.cbegin(), mContents.cend(), [source](auto const& content)
    {
        return content.getComponent() == source;
    });
    anlWeakAssert(it != mContents.cend());
    if(it == mContents.cend())
    {
        return;
    }
    
    auto const y = dragSourceDetails.localPosition.getY() + offset;
    auto const sourceHeight = source->getHeight();
    auto const index = static_cast<size_t>(std::distance(mContents.cbegin(), it));
    auto previousY = 0;
    auto distance = sourceHeight;
    auto newIndex = index;
    for(size_t i = 0; i < mContents.size(); ++i)
    {
        if(mContents[i] != nullptr)
        {
            if(i != index && i != index + 1)
            {
                auto const dist = static_cast<double>(std::abs(previousY - y));
                if(dist < sourceHeight)
                {
                    distance = dist;
                    newIndex = i > index ? i - 1 : i;
                }
            }
            previousY += mContents[i]->getHeight();
        }
    }
    
    if(newIndex != index)
    {
        onComponentDragged(index, newIndex);
    }
    
    itemDragExit(dragSourceDetails);
}

ANALYSE_FILE_END
