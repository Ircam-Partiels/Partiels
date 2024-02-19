#include "AnlApplicationKeyMappings.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::KeyMappingsContent::Container::Section::Content::Content(juce::ApplicationCommandManager const& commandManager, juce::String const& category)
{
    for(auto const& commandId : commandManager.getCommandsInCategory(category))
    {
        auto const* command = commandManager.getCommandForID(commandId);
        MiscWeakAssert(command != nullptr);
        if(command != nullptr)
        {
            if(!command->defaultKeypresses.isEmpty())
            {
                auto property = std::make_unique<PropertyLabel>(command->shortName, command->description);
                if(property != nullptr)
                {
                    if(command->defaultKeypresses[0] == juce::KeyPress(0x08, juce::ModifierKeys::noModifiers, 0))
                    {
                        property->entry.setText(juce::KeyPress(juce::KeyPress::backspaceKey, juce::ModifierKeys::noModifiers, 0).getTextDescription(), juce::NotificationType::dontSendNotification);
                    }
                    else
                    {
                        property->entry.setText(command->defaultKeypresses[0].getTextDescription(), juce::NotificationType::dontSendNotification);
                    }
                    addAndMakeVisible(property.get());
                    mProperties.push_back(std::move(property));
                }
            }
        }
    }
}

void Application::KeyMappingsContent::Container::Section::Content::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component* component)
    {
        if(component != nullptr && component->isVisible())
        {
            component->setBounds(bounds.removeFromTop(component->getHeight()));
        }
    };
    for(auto& property : mProperties)
    {
        setBounds(property.get());
    }
    setSize(getWidth(), bounds.getY());
}

Application::KeyMappingsContent::Container::Section::Section(juce::ApplicationCommandManager const& commandManager, juce::String const& category)
: content(commandManager, category)
, table(category.toUpperCase(), true)
{
    table.setComponents({content});
}

Application::KeyMappingsContent::Container::Container(juce::KeyPressMappingSet& keyPressMappingSet)
: mKeyPressMappingSet(keyPressMappingSet)
{
    mKeyPressMappingSet.addChangeListener(this);
    changeListenerCallback(std::addressof(mKeyPressMappingSet));
    mComponentListener.onComponentResized = [&]([[maybe_unused]] juce::Component& component)
    {
        resized();
    };
}

Application::KeyMappingsContent::Container::~Container()
{
    mKeyPressMappingSet.removeChangeListener(this);
    for(auto& section : mSections)
    {
        if(section != nullptr)
        {
            mComponentListener.detachFrom(section->table);
        }
    }
    mSections.clear();
}

void Application::KeyMappingsContent::Container::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    for(auto& section : mSections)
    {
        if(section != nullptr)
        {
            setBounds(section->table);
        }
    }
    setSize(getWidth(), std::max(bounds.getY(), 120) + 2);
}

void Application::KeyMappingsContent::Container::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    for(auto& section : mSections)
    {
        if(section != nullptr)
        {
            mComponentListener.detachFrom(section->table);
        }
    }
    mSections.clear();
    auto const& commandManager = mKeyPressMappingSet.getCommandManager();
    auto const hasKeyCommand = [&](juce::String const& category)
    {
        for(auto const& commandID : commandManager.getCommandsInCategory(category))
        {
            auto const* command = commandManager.getCommandForID(commandID);
            if(command != nullptr && !command->defaultKeypresses.isEmpty())
            {
                return true;
            }
        }
        return false;
    };
    for(auto const& category : commandManager.getCommandCategories())
    {
        if(hasKeyCommand(category))
        {
            auto section = std::make_unique<Section>(commandManager, category);
            if(section != nullptr)
            {
                addAndMakeVisible(section->table);
                mComponentListener.attachTo(section->table);
                mSections.push_back(std::move(section));
            }
        }
    }
    resized();
}

Application::KeyMappingsContent::KeyMappingsContent()
: mContainer(*Instance::get().getApplicationCommandManager().getKeyMappings())
{
    setSize(280, 400);
    mContainer.setSize(getWidth() - mViewport.getScrollBarThickness(), 400);
    mViewport.setViewedComponent(std::addressof(mContainer), false);
    addAndMakeVisible(mViewport);
}

void Application::KeyMappingsContent::resized()
{
    mViewport.setBounds(getLocalBounds());
}

Application::KeyMappingsPanel::KeyMappingsPanel()
: HideablePanelTyped<KeyMappingsContent>(juce::translate("Key Mappings"))
{
#ifdef JUCE_DEBUG
    addMouseListener(this, true);
#endif
}

#ifdef JUCE_DEBUG
void Application::KeyMappingsPanel::mouseDown(juce::MouseEvent const& event)
{
    if(event.mods.isCommandDown())
    {
        auto keyMappingFile = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory).getChildFile("PartielsKeyMappingFile.xml");
        if(!keyMappingFile.existsAsFile())
        {
            auto* keyMapping = Instance::get().getApplicationCommandManager().getKeyMappings();
            if(keyMapping == nullptr)
            {
                return;
            }
            auto xml = keyMapping->createXml(false);
            if(xml == nullptr)
            {
                return;
            }
            if(!xml->writeTo(keyMappingFile))
            {
                return;
            }
        }
    }
}
#endif

ANALYSE_FILE_END
