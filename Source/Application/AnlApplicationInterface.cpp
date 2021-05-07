#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Loader::Loader()
{
    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    mLoadFileButton.onClick = [&]()
    {
        commandManager.invokeDirectly(CommandIDs::DocumentOpen, true);
    };
    mAddTrackButton.onClick = [&]()
    {
        commandManager.invokeDirectly(CommandIDs::EditNewTrack, true);
    };
    mLoadTemplateButton.onClick = [&]()
    {
        commandManager.invokeDirectly(CommandIDs::DocumentOpenTemplate, true);
    };
    mLoadFileLabel.setJustificationType(juce::Justification::centred);

    mDocumentListener.onAttrChanged = [this](Document::Accessor const& acsr, Document::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case Document::AttrType::file:
            {
                updateState();
            }
            break;
            case Document::AttrType::layout:
                break;
        }
    };

    mDocumentListener.onAccessorErased = mDocumentListener.onAccessorInserted = [this](Document::Accessor const& acsr, Document::AcsrType type, size_t index)
    {
        juce::ignoreUnused(acsr, type, index);
        updateState();
    };

    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.addListener(mDocumentListener, NotificationType::synchronous);
    commandManager.addListener(this);
}

Application::Interface::Loader::~Loader()
{
    Instance::get().getApplicationCommandManager().removeListener(this);
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    documentAccessor.removeListener(mDocumentListener);
}

void Application::Interface::Loader::updateState()
{
    auto& documentAccessor = Instance::get().getDocumentAccessor();
    auto const documentHasFile = documentAccessor.getAttr<Document::AttrType::file>().existsAsFile();
    auto const documentHasTrackOrGroup = documentAccessor.getNumAcsr<Document::AcsrType::tracks>() > 0_z || documentAccessor.getNumAcsr<Document::AcsrType::groups>() > 0_z;

    removeChildComponent(&mLoadFileButton);
    removeChildComponent(&mLoadFileLabel);
    removeChildComponent(&mAddTrackButton);
    removeChildComponent(&mLoadTemplateButton);
    setVisible(!documentHasFile || !documentHasTrackOrGroup);
    if(!documentHasFile)
    {
        addAndMakeVisible(mLoadFileButton);
        addAndMakeVisible(mLoadFileLabel);
    }
    else if(!documentHasTrackOrGroup)
    {
        addAndMakeVisible(mAddTrackButton);
        addAndMakeVisible(mLoadTemplateButton);
    }
}

void Application::Interface::Loader::resized()
{
    auto const bounds = getLocalBounds();
    {
        auto innerBounds = bounds.withSizeKeepingCentre(200, 100);
        mLoadFileButton.setBounds(innerBounds.removeFromTop(32));
        mLoadFileLabel.setBounds(innerBounds);
    }
    {
        auto innerBounds = bounds.withSizeKeepingCentre(402, 32);
        mAddTrackButton.setBounds(innerBounds.removeFromLeft(200));
        mLoadTemplateButton.setBounds(innerBounds.withTrimmedLeft(2));
    }
}

void Application::Interface::Loader::paint(juce::Graphics& g)
{
    g.fillAll(mIsDragging ? findColour(juce::TextButton::ColourIds::buttonColourId) : findColour(juce::ResizableWindow::ColourIds::backgroundColourId));
}

void Application::Interface::Loader::applicationCommandInvoked(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    juce::ignoreUnused(info);
}

void Application::Interface::Loader::applicationCommandListChanged()
{
    using CommandIDs = CommandTarget::CommandIDs;
    auto& commandManager = Instance::get().getApplicationCommandManager();
    mLoadFileButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::DocumentOpen));
    mAddTrackButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::EditNewTrack));
    mLoadTemplateButton.setTooltip(commandManager.getDescriptionOfCommand(CommandIDs::DocumentOpenTemplate));
}

bool Application::Interface::Loader::isInterestedInFileDrag(juce::StringArray const& files)
{
    if(!mLoadFileButton.isVisible())
    {
        return false;
    }
    auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats() + ";" + Instance::getFileWildCard();
    for(auto const& fileName : files)
    {
        if(audioFormatWildcard.contains(juce::File(fileName).getFileExtension()))
        {
            return true;
        }
    }
    return false;
}

void Application::Interface::Loader::fileDragEnter(juce::StringArray const& files, int x, int y)
{
    mIsDragging = true;
    juce::ignoreUnused(files, x, y);
    mLoadFileButton.setState(juce::Button::ButtonState::buttonOver);
    repaint();
}

void Application::Interface::Loader::fileDragExit(juce::StringArray const& files)
{
    mIsDragging = false;
    juce::ignoreUnused(files);
    mLoadFileButton.setState(juce::Button::ButtonState::buttonNormal);
    repaint();
}

void Application::Interface::Loader::filesDropped(juce::StringArray const& files, int x, int y)
{
    mIsDragging = false;
    juce::ignoreUnused(x, y);
    mLoadFileButton.setState(juce::Button::ButtonState::buttonNormal);
    auto const audioFormatWildcard = Instance::get().getAudioFormatManager().getWildcardForAllFormats() + ";" + Instance::getFileWildCard();
    auto getFile = [&]()
    {
        for(auto const& fileName : files)
        {
            if(audioFormatWildcard.contains(juce::File(fileName).getFileExtension()))
            {
                return juce::File(fileName);
            }
        }
        return juce::File();
    };
    auto const file = getFile();
    if(file == juce::File())
    {
        return;
    }
    Instance::get().openFile(file);
    repaint();
}

Application::Interface::Interface()
: mTransportDisplay(Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>())
, mDocumentSection(Instance::get().getDocumentDirector())
{
    addAndMakeVisible(mTransportDisplay);
    addAndMakeVisible(mDocumentSection);
    addAndMakeVisible(mLoaderDecorator);
    addAndMakeVisible(mToolTipDisplay);
    mLoader.addComponentListener(this);
}

Application::Interface::~Interface()
{
    mLoader.removeComponentListener(this);
}

void Application::Interface::resized()
{
    auto bounds = getLocalBounds();
    mTransportDisplay.setBounds(bounds.removeFromTop(40).withSizeKeepingCentre(284, 40));
    mToolTipDisplay.setBounds(bounds.removeFromBottom(24));
    mDocumentSection.setBounds(bounds);
    mLoaderDecorator.setBounds(bounds);
}

void Application::Interface::moveKeyboardFocusTo(juce::String const& identifier)
{
    mDocumentSection.moveKeyboardFocusTo(identifier);
}

void Application::Interface::componentVisibilityChanged(juce::Component& component)
{
    anlStrongAssert(&component == &mLoader);
    if(&component != &mLoader)
    {
        return;
    }
    mLoaderDecorator.setVisible(mLoader.isVisible());
    mTransportDisplay.setVisible(!mLoader.isVisible());
    mDocumentSection.setVisible(!mLoader.isVisible());
}

ANALYSE_FILE_END
