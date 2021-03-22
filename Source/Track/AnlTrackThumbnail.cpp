#include "AnlTrackThumbnail.h"
#include "AnlTrackExporter.h"

ANALYSE_FILE_BEGIN

Track::Thumbnail::Thumbnail(Accessor& accessor)
: mAccessor(accessor)
{
    addAndMakeVisible(mPropertiesButton);
    addAndMakeVisible(mExportButton);
    addAndMakeVisible(mRemoveButton);
    addAndMakeVisible(mProcessingButton);
    
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
        auto const processing = mAccessor.getAttr<AttrType::processing>();
        auto const& results = mAccessor.getAttr<AttrType::results>();
        auto const canBeExported = !processing && results != nullptr && !results->empty();
        juce::PopupMenu menu;
        menu.addItem(juce::translate("Export as template..."), true, false, [this]()
        {
            Exporter::toTemplate(mAccessor, AlertType::window);
        });
        menu.addItem("Export as image", canBeExported, false, [this]()
        {
            Exporter::toImage(mAccessor, AlertType::window);
        });
        menu.addItem("Export as CSV...", canBeExported, false, [this]()
        {
            Exporter::toCsv(mAccessor, AlertType::window);
        });
        menu.addItem("Export as XML...", canBeExported, false, [this]()
        {
            Exporter::toXml(mAccessor, AlertType::window);
        });
        menu.showAt(&mExportButton);
        // Force to repaint to update the state
        mExportButton.setState(juce::Button::ButtonState::buttonNormal);
    };
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::name:
            {
                repaint();
            }
                break;
            case AttrType::processing:
            case AttrType::warnings:
            {
                auto const state = acsr.getAttr<AttrType::processing>();
                auto const warnings = acsr.getAttr<AttrType::warnings>();
                auto getInactiveIconType = [warnings]()
                {
                    return warnings == WarningType::none ? IconManager::IconType::checked : IconManager::IconType::alert;
                };
                auto getTooltip = [state, warnings]()
                {
                    if(state)
                    {
                        return "Processing analysis...";
                    }
                    switch(warnings)
                    {
                        case WarningType::none:
                            return "Analysis successfully completed!";
                        case WarningType::plugin:
                            return "Analysis failed: The plugin cannot be found or allocated!";
                        case WarningType::state:
                            return "Analysis failed: The step size or the block size might not be supported!";
                    }
                    return "Analysis finished!";
                };
                mProcessingButton.setInactiveImage(IconManager::getIcon(getInactiveIconType()));
                mProcessingButton.setTooltip(juce::translate(getTooltip()));
                mProcessingButton.setActive(state);
            }
                break;
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
    mProcessingButton.setVisible(bounds.getHeight() >= size);
    mProcessingButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mExportButton.setVisible(bounds.getHeight() >= size);
    mExportButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
    mPropertiesButton.setVisible(bounds.getHeight() >= size);
    mPropertiesButton.setBounds(bounds.removeFromBottom(size).reduced(separator));
}

void Track::Thumbnail::paint(juce::Graphics& g)
{
    g.fillAll(findColour(ColourIds::backgroundColourId));
    
    g.setColour(findColour(ColourIds::borderColourId));
    g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 4.0f, 1.0f);
    
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

void Track::Thumbnail::colourChanged()
{
    setOpaque(findColour(ColourIds::backgroundColourId).isOpaque());
}

void Track::Thumbnail::mouseDrag(juce::MouseEvent const& event)
{
    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    auto* parent = getParentComponent();
    anlWeakAssert(dragContainer != nullptr && parent != nullptr);
    if(dragContainer != nullptr && !dragContainer->isDragAndDropActive() && parent != nullptr)
    {
        juce::Image snapshot(juce::Image::ARGB, parent->getWidth(), parent->getHeight(), true);
        juce::Graphics g(snapshot);
        g.beginTransparencyLayer(0.6f);
        parent->paintEntireComponent(g, false);
        g.endTransparencyLayer();
        
        auto const p = -event.getMouseDownPosition();
        dragContainer->startDragging(DraggableTable::createDescription(event), parent, snapshot, true, &p, &event.source);
    }
}

ANALYSE_FILE_END
