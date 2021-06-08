#include "AnlDraggableTable.h"

ANALYSE_FILE_BEGIN

juce::var DraggableTable::createDescription(juce::MouseEvent const& event, juce::String const& type, juce::String const& identifier, int height, std::function<void(void)> onEnter, std::function<void(void)> onExit)
{
    auto description = std::make_unique<Description>();
    anlWeakAssert(description != nullptr);
    if(description != nullptr)
    {
        description->setProperty("type", type);
        description->setProperty("identifier", identifier);
        description->setProperty("height", height);
        description->setProperty("offset", -event.getMouseDownY());
        description->onEnter = onEnter;
        description->onExit = onExit;
    }
    return {description.release()};
}

DraggableTable::DraggableTable(juce::String const type, juce::String const& tooltip)
: mType(type)
{
    setTooltip(tooltip);
}

void DraggableTable::resized()
{
    if(mIsDragging)
    {
        return;
    }
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
        if(content != nullptr && std::none_of(components.cbegin(), components.cend(), [&](auto const& component)
                                              {
                                                  return std::addressof(component.get()) == content.getComponent();
                                              }))
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

std::vector<juce::Component::SafePointer<juce::Component>> DraggableTable::getComponents()
{
    return mContents;
}

void DraggableTable::componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized)
{
    juce::ignoreUnused(component, wasMoved);
    if(wasResized && !mIsDragging)
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
    return obj->getProperty("type").toString() == mType;
}

void DraggableTable::itemDragEnter(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* source = dragSourceDetails.sourceComponent.get();
    auto* obj = dragSourceDetails.description.getDynamicObject();
    anlWeakAssert(obj != nullptr && source != nullptr);
    if(obj == nullptr || source == nullptr || source->findParentComponentOfClass<DraggableTable>() != this)
    {
        mIsDragging = false;
        return;
    }
    auto const fullSize = std::accumulate(mContents.cbegin(), mContents.cend(), 0, [](int value, auto const& content)
                                          {
                                              return (content != nullptr) ? value + content->getHeight() : value;
                                          });
    setSize(getWidth(), fullSize + static_cast<int>(obj->getProperty("height")));

    mIsDragging = true;
    if(auto* description = dynamic_cast<Description*>(obj))
    {
        if(description->onEnter != nullptr)
        {
            description->onEnter();
        }
    }
    itemDragMove(dragSourceDetails);
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

    if(auto* viewport = findParentComponentOfClass<juce::Viewport>())
    {
        if(viewport->canScrollVertically())
        {
            auto const point = viewport->getLocalPoint(this, dragSourceDetails.localPosition);
            viewport->autoScroll(point.x, point.y, 20, 10);
        }
    }

    if(source->findParentComponentOfClass<DraggableTable>() != this)
    {
        return;
    }

    auto const isCopy = juce::Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isCtrlDown();
    source->setAlpha(isCopy ? 1.0f : 0.4f);

    auto const offset = static_cast<int>(obj->getProperty("offset"));
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
    auto const position = dragSourceDetails.localPosition.getY() + offset;
    auto const height = static_cast<int>(obj->getProperty("height"));
    auto const sourceHeight = static_cast<double>(height + 2);
    auto const index = static_cast<size_t>(std::distance(mContents.cbegin(), it));

    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    for(size_t i = 0; i < mContents.size(); ++i)
    {
        if(mContents[i] != nullptr)
        {
            if(isCopy || (i != index && i != index + 1))
            {
                auto const difference = std::abs(bounds.getY() - position);
                auto const distance = static_cast<double>(difference) / mContents[i]->getHeight();
                auto const scale = 1.0 - std::min(std::max((0.8 - std::cos(std::min(distance * pi, pi))) / 1.8, 0.0), 1.0);
                bounds.removeFromTop(static_cast<int>(std::ceil(scale * sourceHeight)));
            }
            mContents[i]->setBounds(bounds.removeFromTop(mContents[i]->getHeight()));
        }
    }
}

void DraggableTable::itemDragExit(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* source = dragSourceDetails.sourceComponent.get();
    anlWeakAssert(source != nullptr);
    if(source != nullptr)
    {
        source->setAlpha(1.0f);
    }
    mIsDragging = false;
    if(auto* description = dynamic_cast<Description*>(dragSourceDetails.description.getDynamicObject()))
    {
        if(description->onExit != nullptr)
        {
            description->onExit();
        }
    }
    auto const fullSize = std::accumulate(mContents.cbegin(), mContents.cend(), 0, [](int value, auto const& content)
                                          {
                                              return (content != nullptr) ? value + content->getHeight() : value;
                                          });
    setSize(getWidth(), fullSize);
}

void DraggableTable::itemDropped(juce::DragAndDropTarget::SourceDetails const& dragSourceDetails)
{
    auto* obj = dragSourceDetails.description.getDynamicObject();
    auto* source = dragSourceDetails.sourceComponent.get();
    anlWeakAssert(obj != nullptr && source != nullptr);
    if(obj == nullptr || source == nullptr)
    {
        mIsDragging = false;
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
        mIsDragging = false;
        return;
    }
    auto const isCopy = juce::Desktop::getInstance().getMainMouseSource().getCurrentModifiers().isCtrlDown();
    auto const index = static_cast<size_t>(std::distance(mContents.cbegin(), it));
    auto getNewIndex = [&]()
    {
        auto contentY = 0;
        auto const position = dragSourceDetails.localPosition.getY() + offset;
        auto const sourceHeight = source->getHeight() + 2;
        for(size_t i = 0; i < mContents.size(); ++i)
        {
            if(mContents[i] != nullptr)
            {
                auto const distance = std::abs(contentY - 1 - position);
                if((isCopy || (i != index && i != index + 1)) && distance <= sourceHeight / 2)
                {
                    return isCopy ? i : (i < index ? i : i - 1);
                }
                contentY += mContents[i]->getHeight();
            }
        }
        auto const distance = std::abs(contentY - position);
        if(index < mContents.size() - 1 && distance <= sourceHeight)
        {
            return mContents.size() - 1;
        }
        return index;
    };

    if(auto* description = dynamic_cast<Description*>(obj))
    {
        if(description->onExit != nullptr)
        {
            description->onExit();
        }
    }
    source->setAlpha(1.0f);
    mIsDragging = false;

    auto const newIndex = getNewIndex();
    if((isCopy || index != newIndex) && onComponentDropped != nullptr)
    {
        onComponentDropped(obj->getProperty("identifier"), newIndex, isCopy);
    }

    auto const fullSize = std::accumulate(mContents.cbegin(), mContents.cend(), 0, [](int value, auto const& content)
                                          {
                                              return (content != nullptr) ? value + content->getHeight() : value;
                                          });
    setSize(getWidth(), fullSize);
}

ANALYSE_FILE_END
