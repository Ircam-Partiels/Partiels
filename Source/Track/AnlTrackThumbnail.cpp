#include "AnlTrackThumbnail.h"
#include "AnlTrackSection.h"
#include "AnlTrackExporter.h"

ANALYSE_FILE_BEGIN

Track::Thumbnail::Thumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    addAndMakeVisible(mPropertiesButton);
    addAndMakeVisible(mExportButton);
    addAndMakeVisible(mRemoveButton);
    addAndMakeVisible(mStateButton);
    
    mPropertiesButton.setTooltip(juce::translate("Change the analysis properties"));
    mExportButton.setTooltip(juce::translate("Export the analysis"));
    mRemoveButton.setTooltip(juce::translate("Remove the analysis"));
    
    mRemoveButton.onClick = [&]()
    {
        if(onRemove != nullptr)
        {
            onRemove();
        }
    };
    
    mPropertiesButton.onClick = [&]()
    {
        mPropertyPanel.show();
    };
    
    mExportButton.onClick = [&]()
    {
        JUCE_COMPILER_WARNING("these conditions should be fixed")
        auto const processing = mAccessor.getAttr<AttrType::processing>();
        auto const& results = mAccessor.getAttr<AttrType::results>();
        auto const resultsAreReady = !std::get<0>(processing) && results != nullptr && !results->empty();
        auto const graphicsAreReady = !std::get<2>(processing) && results != nullptr && !results->empty();
        juce::PopupMenu menu;
        menu.addItem(juce::translate("Export as template..."), true, false, [this]()
        {
            Exporter::toTemplate(mAccessor, AlertType::window);
        });
        menu.addItem("Export as image", graphicsAreReady, false, [this]()
        {
            Exporter::toImage(mAccessor, AlertType::window);
        });
        menu.addItem("Export as CSV...", resultsAreReady, false, [this]()
        {
            Exporter::toCsv(mAccessor, AlertType::window);
        });
        menu.addItem("Export as XML...", resultsAreReady, false, [this]()
        {
            Exporter::toXml(mAccessor, AlertType::window);
        });
        menu.showAt(&mExportButton);
        // Force to repaint to update the state
        mExportButton.setState(juce::Button::ButtonState::buttonNormal);
    };
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::name:
            {
                setTooltip(acsr.getAttr<AttrType::name>());
                repaint();
            }
                break;
            case AttrType::processing:
            case AttrType::warnings:
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::results:
            case AttrType::graphics:
            case AttrType::time:
            case AttrType::identifier:
            case AttrType::height:
            case AttrType::colours:
            case AttrType::propertyState:
            case AttrType::zoomLink:
                break;
}
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Track::Thumbnail::~Thumbnail()
{
    mAccessor.removeListener(mListener);
}

void Track::Thumbnail::resized()
{
    auto bounds = getLocalBounds().withTrimmedLeft(getWidth() / 2);
    auto constexpr separator = 2;
    auto const size = bounds.getWidth() - separator;
    mRemoveButton.setVisible(bounds.getHeight() >= size);
    mRemoveButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mStateButton.setVisible(bounds.getHeight() >= size);
    mStateButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mExportButton.setVisible(bounds.getHeight() >= size);
    mExportButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mPropertiesButton.setVisible(bounds.getHeight() >= size);
    mPropertiesButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
}

void Track::Thumbnail::paint(juce::Graphics& g)
{
    auto constexpr separator = 2;
    auto constexpr rotation = -1.5707963268f;
    
    auto const width = getWidth() / 2;
    auto const height = getHeight();
    auto const bottom = height - 2 * separator;
    auto const size = height - 4 * separator;
    
    g.setColour(findColour(ColourIds::textColourId));
    g.addTransform(juce::AffineTransform::rotation(rotation, 0.0f, static_cast<float>(bottom)));
    g.drawFittedText(mAccessor.getAttr<AttrType::name>(), 0, bottom, size, width, juce::Justification::centredLeft, 1, 1.0f);
}

void Track::Thumbnail::lookAndFeelChanged()
{
    auto* laf = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(laf != nullptr);
    if(laf != nullptr)
    {
        laf->setButtonIcon(mExportButton, IconManager::IconType::share);
        laf->setButtonIcon(mPropertiesButton, IconManager::IconType::properties);
        laf->setButtonIcon(mRemoveButton, IconManager::IconType::cancel);
    }
}

void Track::Thumbnail::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

void Track::Thumbnail::mouseDrag(juce::MouseEvent const& event)
{
    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    auto* parent = findParentComponentOfClass<Section>();
    anlWeakAssert(dragContainer != nullptr && parent != nullptr);
    if(dragContainer != nullptr && !dragContainer->isDragAndDropActive() && parent != nullptr)
    {
        juce::Image snapshot(juce::Image::ARGB, parent->getWidth(), parent->getHeight(), true);
        juce::Graphics g(snapshot);
        g.beginTransparencyLayer(0.6f);
        parent->paintEntireComponent(g, false);
        g.endTransparencyLayer();
        
        auto const p = -event.getMouseDownPosition();
        dragContainer->startDragging(DraggableTable::createDescription(event, "Track"), parent, snapshot, true, &p, &event.source);
    }
}

ANALYSE_FILE_END
