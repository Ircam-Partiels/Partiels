#include "AnlGroupThumbnail.h"
#include "AnlGroupStrechableSection.h"

ANALYSE_FILE_BEGIN

Group::Thumbnail::Thumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    addAndMakeVisible(mNameButton);
    addAndMakeVisible(mExportButton);
    addAndMakeVisible(mStateButton);
    addAndMakeVisible(mRemoveButton);
    addAndMakeVisible(mExpandButton);
    addAndMakeVisible(mDropdownButton);

    mNameButton.setTooltip(juce::translate("Change the name of the group or the tracks' properties"));
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

    auto getNameMenu = [&]()
    {
        juce::PopupMenu menu;
        menu.addItem("Change Group Name", [&]()
                     {
                         class Editor
                         : public juce::TextEditor
                         {
                         public:
                             Editor(Accessor& accessor)
                             : mAccessor(accessor)
                             {
                                 setMultiLine(false);
                                 setJustification(juce::Justification::left);
                                 setText(mAccessor.getAttr<AttrType::name>(), juce::NotificationType::dontSendNotification);
                                 onTextChange = [&]()
                                 {
                                     mAccessor.setAttr<AttrType::name>(getText(), NotificationType::synchronous);
                                 };
                                 onReturnKey = onEscapeKey = onFocusLost = [&]()
                                 {
                                     exitModalState(0);
                                 };
                                 setColour(juce::TextEditor::ColourIds::backgroundColourId, getLookAndFeel().findColour(FloatingWindow::ColourIds::backgroundColourId));
                             }

                             void inputAttemptWhenModal() override
                             {
                                 exitModalState(0);
                             }

                         private:
                             Accessor& mAccessor;
                         };

                         Editor editor(mAccessor);
                         auto const buttonBounds = mNameButton.getScreenBounds();
                         editor.setBounds(buttonBounds.getRight(), buttonBounds.getY(), 80, 22);
                         editor.addToDesktop(juce::ComponentPeer::windowHasDropShadow | juce::ComponentPeer::windowIsTemporary);
                         editor.enterModalState(true, nullptr, false);
                         editor.runModalLoop();
                     });
        auto const layout = mAccessor.getAttr<AttrType::layout>();
        for(auto const& identifier : layout)
        {
            auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
            if(trackAcsr.has_value())
            {
                auto const trackName = trackAcsr->get().getAttr<Track::AttrType::name>();
                menu.addItem("Show " + trackName + " properties", [=]
                             {
                                 auto var = std::make_unique<juce::DynamicObject>();
                                 if(var != nullptr)
                                 {
                                     auto const position = juce::Desktop::getInstance().getMousePosition();
                                     var->setProperty("x", position.x);
                                     var->setProperty("y", position.y - 40);
                                     trackAcsr->get().sendSignal(Track::SignalType::showProperties, var.release(), NotificationType::synchronous);
                                 }
                             });
            }
        }
        return menu;
    };

    mNameButton.onClick = [=, this]()
    {
        getNameMenu().showAt(&mNameButton);
    };

    mDropdownButton.onClick = [=, this]()
    {
        juce::PopupMenu menu;
        if(!mNameButton.isVisible())
        {
            menu.addSubMenu(mNameButton.getTooltip(), getNameMenu());
        }
        auto addItem = [&](juce::Button& button)
        {
            if(!button.isVisible())
            {
                menu.addItem(button.getTooltip(), button.onClick);
            }
        };
        addItem(mNameButton);
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
            case AttrType::focused:
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
    layoutButton(mNameButton);
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
        laf->setButtonIcon(mNameButton, IconManager::IconType::properties);
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

void Group::Thumbnail::mouseDown(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    beginDragAutoRepeat(5);
}

void Group::Thumbnail::mouseDrag(juce::MouseEvent const& event)
{
    if((event.eventTime - event.mouseDownTime).inMilliseconds() < static_cast<juce::int64>(100))
    {
        return;
    }
    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    auto* section = findParentComponentOfClass<Section>();
    auto* parent = findParentComponentOfClass<StrechableSection>();
    anlWeakAssert(dragContainer != nullptr && parent != nullptr);
    if(dragContainer != nullptr && !dragContainer->isDragAndDropActive() && section != nullptr && parent != nullptr)
    {
        juce::Image snapshot(juce::Image::ARGB, section->getWidth(), section->getHeight(), true);
        juce::Graphics g(snapshot);
        g.beginTransparencyLayer(0.6f);
        section->paintEntireComponent(g, false);
        g.endTransparencyLayer();

        auto const p = -event.getMouseDownPosition();
        auto const expanded = mAccessor.getAttr<AttrType::expanded>();
        dragContainer->startDragging(DraggableTable::createDescription(
                                         event, "Group", mAccessor.getAttr<AttrType::identifier>(), section->getHeight(), [=, this]()
                                         {
                                             if(expanded)
                                             {
                                                 mAccessor.setAttr<AttrType::expanded>(false, NotificationType::synchronous);
                                             }
                                         },
                                         [=, this]()
                                         {
                                             if(expanded)
                                             {
                                                 mAccessor.setAttr<AttrType::expanded>(true, NotificationType::synchronous);
                                             }
                                         }),
                                     parent, snapshot, true, &p, &event.source);
    }
}

ANALYSE_FILE_END
