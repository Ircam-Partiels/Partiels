#include "AnlAnalyzerThumbnail.h"
#include "AnlAnalyzerExporter.h"

ANALYSE_FILE_BEGIN

Analyzer::Thumbnail::Thumbnail(Accessor& accessor)
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
        auto const canBeExported = !processing && !results.empty();
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
            {
                auto const state = acsr.getAttr<AttrType::processing>();
                mProcessingButton.setActive(state);
                mProcessingButton.setTooltip(state ? juce::translate("Processing analysis...") : juce::translate("Analysis finished!"));
            }
                break;
            case AttrType::key:
            case AttrType::description:
            case AttrType::state:
            case AttrType::results:
            case AttrType::warnings:
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

Analyzer::Thumbnail::~Thumbnail()
{
    mAccessor.removeListener(mListener);
}

void Analyzer::Thumbnail::resized()
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

void Analyzer::Thumbnail::paint(juce::Graphics& g)
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

void Analyzer::Thumbnail::lookAndFeelChanged()
{
    auto* lookAndFeel = dynamic_cast<IconManager::LookAndFeelMethods*>(&getLookAndFeel());
    anlWeakAssert(lookAndFeel != nullptr);
    if(lookAndFeel != nullptr)
    {
        lookAndFeel->setButtonIcon(mExportButton, IconManager::IconType::share);
        lookAndFeel->setButtonIcon(mPropertiesButton, IconManager::IconType::properties);
        lookAndFeel->setButtonIcon(mRemoveButton, IconManager::IconType::cancel);
    }
}

void Analyzer::Thumbnail::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

void Analyzer::Thumbnail::colourChanged()
{
    setOpaque(findColour(ColourIds::backgroundColourId).isOpaque());
}

void Analyzer::Thumbnail::mouseDrag(juce::MouseEvent const& event)
{
    auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this);
    anlWeakAssert(dragContainer != nullptr);
    if(dragContainer != nullptr && !dragContainer->isDragAndDropActive())
    {
        auto description = std::make_unique<juce::DynamicObject>();
        anlWeakAssert(description != nullptr);
        if(description != nullptr)
        {
            JUCE_COMPILER_WARNING("clean this");
            auto* parent = getParentComponent();
            juce::Image snapshot(juce::Image::ARGB, parent->getWidth(), parent->getHeight(), true);
            juce::Graphics g(snapshot);
            g.beginTransparencyLayer(0.6f);
            parent->paintEntireComponent(g, false);
            g.endTransparencyLayer();
            
            auto const p = -event.getMouseDownPosition();
            
            description->setProperty("type", "DraggableTable::Content");
            description->setProperty("offset", p.getY());
            dragContainer->startDragging(description.release(), parent, snapshot, true, &p, &event.source);
        }
    }
}

ANALYSE_FILE_END
