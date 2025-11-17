#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Application::Interface::ReaderLayoutPanel::ReaderLayoutPanel()
: mReaderLayoutContent(Instance::get().getDocumentDirector())
{
    setContent(juce::translate("Audio Files Layout"), std::addressof(mReaderLayoutContent));
}

Application::Interface::ReaderLayoutPanel::~ReaderLayoutPanel()
{
    setContent("", nullptr);
}

bool Application::Interface::ReaderLayoutPanel::escapeKeyPressed()
{
    mReaderLayoutContent.warnBeforeClosing();
    hide();
    return true;
}

Application::Interface::PluginSearchPathPanel::PluginSearchPathPanel()
: mPluginSearchPath(Instance::get().getPluginListAccessor())
{
    setContent("Plugin Settings", std::addressof(mPluginSearchPath));
}

Application::Interface::PluginSearchPathPanel::~PluginSearchPathPanel()
{
    setContent("", nullptr);
}

bool Application::Interface::PluginSearchPathPanel::escapeKeyPressed()
{
    mPluginSearchPath.warnBeforeClosing();
    hide();
    return true;
}

Application::Interface::PluginListTablePanel::PluginListTablePanel(juce::Component& content)
: mTitleLabel("Title", juce::translate("Add Plugins..."))
, mCloseButton(juce::ImageCache::getFromMemory(AnlIconsData::cancel_png, AnlIconsData::cancel_pngSize))
, mSettingsButton(juce::ImageCache::getFromMemory(AnlIconsData::settings_png, AnlIconsData::settings_pngSize))
, mContent(content)
{
    mCloseButton.onClick = []()
    {
        if(auto* window = Instance::get().getWindow())
        {
            window->getInterface().hidePluginListTablePanel();
        }
    };
    mSettingsButton.onClick = []()
    {
        if(auto* window = Instance::get().getWindow())
        {
            window->getInterface().showPluginSearchPathPanel();
        }
    };
    mTitleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mTitleLabel);
    addAndMakeVisible(mCloseButton);
    addAndMakeVisible(mSettingsButton);
    addAndMakeVisible(mTopSeparator);
    addAndMakeVisible(mLeftSeparator);
    addAndMakeVisible(mContent);
}

void Application::Interface::PluginListTablePanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void Application::Interface::PluginListTablePanel::resized()
{
    auto bounds = getLocalBounds();
    mLeftSeparator.setBounds(bounds.removeFromLeft(1));
    auto top = bounds.removeFromTop(24);
    mCloseButton.setBounds(top.removeFromLeft(24).reduced(4));
    mSettingsButton.setBounds(top.removeFromRight(24).reduced(4));
    mTitleLabel.setBounds(top);
    mTopSeparator.setBounds(bounds.removeFromTop(1));
    mContent.setBounds(bounds);
}

Application::Interface::NeuralyzerPanel::NeuralyzerPanel(juce::Component& content)
: mTitleLabel("Title", juce::translate("Neuralyzer (Beta)"))
, mCloseButton(juce::ImageCache::getFromMemory(AnlIconsData::cancel_png, AnlIconsData::cancel_pngSize))
, mSettingsButton(juce::ImageCache::getFromMemory(AnlIconsData::settings_png, AnlIconsData::settings_pngSize))
, mContent(content)
{
    mCloseButton.onClick = []()
    {
        if(auto* window = Instance::get().getWindow())
        {
            window->getInterface().hideNeuralyzerPanel();
        }
    };
    mSettingsButton.onClick = []()
    {
        if(auto* window = Instance::get().getWindow())
        {
            window->getInterface().showNeuralyzerSettingsPanel();
        }
    };
    mTitleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mTitleLabel);
    addAndMakeVisible(mCloseButton);
    addAndMakeVisible(mSettingsButton);
    addAndMakeVisible(mTopSeparator);
    addAndMakeVisible(mLeftSeparator);
    addAndMakeVisible(mContent);
}

void Application::Interface::NeuralyzerPanel::resized()
{
    auto bounds = getLocalBounds();
    mLeftSeparator.setBounds(bounds.removeFromLeft(1));
    auto top = bounds.removeFromTop(24);
    mCloseButton.setBounds(top.removeFromLeft(24).reduced(4));
    mSettingsButton.setBounds(top.removeFromRight(24).reduced(4));
    mTitleLabel.setBounds(top);
    mTopSeparator.setBounds(bounds.removeFromTop(1));
    mContent.setBounds(bounds);
}

void Application::Interface::NeuralyzerPanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

Application::Interface::RightBorder::RightBorder()
: pluginListButton(juce::ImageCache::getFromMemory(AnlIconsData::plugin_png, AnlIconsData::plugin_pngSize))
, neuralyzerButton(juce::ImageCache::getFromMemory(AnlIconsData::neuralyzer_png, AnlIconsData::neuralyzer_pngSize))
{
    pluginListButton.setToggleable(true);
    pluginListButton.onClick = []()
    {
        if(auto* window = Instance::get().getWindow())
        {
            window->getInterface().togglePluginListTablePanel();
        }
    };

    neuralyzerButton.setToggleable(true);
    neuralyzerButton.onClick = []()
    {
        if(auto* window = Instance::get().getWindow())
        {
            window->getInterface().toggleNeuralyzerPanel();
        }
    };
    addAndMakeVisible(pluginListButton);
    addAndMakeVisible(neuralyzerButton);
}

void Application::Interface::RightBorder::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId));
}

void Application::Interface::RightBorder::resized()
{
    auto bounds = getLocalBounds();
    pluginListButton.setBounds(bounds.removeFromTop(bounds.getWidth()).reduced(4));
    neuralyzerButton.setBounds(bounds.removeFromTop(bounds.getWidth()).reduced(4));
}

Application::Interface::DocumentContainer::DocumentContainer()
: mDocumentSection(Instance::get().getDocumentDirector(), Instance::get().getApplicationCommandManager(), Instance::get().getTrackPresetListAccessor())
, mPluginListTable(Instance::get().getPluginListAccessor(), Instance::get().getPluginListScanner())
, mNeuralyzerChat(Instance::get().getApplicationAccessor().getAcsr<AcsrType::neuralyzer>(), Instance::get().getNeuralyzerAgent())
{
    mPluginListTable.setMultipleSelectionEnabled(true);
    mPluginListTable.onAddPlugins = [](std::vector<Plugin::Key> keys)
    {
        auto const position = Tools::getNewTrackPosition();
        Tools::addPluginTracks(std::get<0_z>(position), std::get<1_z>(position), keys);
    };

    addAndMakeVisible(mDocumentSection);
    addChildComponent(mLoaderDecorator);
    addAndMakeVisible(mPluginListTablePanel);
    addAndMakeVisible(mNeuralyzerChatPanel);
    addAndMakeVisible(mRightBorder);
    addAndMakeVisible(mRightSeparator);
    addAndMakeVisible(mToolTipSeparator);
    addAndMakeVisible(mToolTipDisplay);

    auto const showHideLoader = [this]()
    {
        auto const& documentAccessor = Instance::get().getDocumentAccessor();
        auto const noAudioFiles = documentAccessor.getAttr<Document::AttrType::reader>().empty();
        auto const noTrackNorGroup = documentAccessor.getNumAcsrs<Document::AcsrType::tracks>() == 0_z && documentAccessor.getNumAcsrs<Document::AcsrType::groups>() == 0_z;
        mLoaderDecorator.setVisible(noAudioFiles || noTrackNorGroup);
        mDocumentSection.setEnabled(!mLoaderDecorator.isVisible());
    };

    mDocumentListener.onAttrChanged = [=]([[maybe_unused]] Document::Accessor const& acsr, Document::AttrType attribute)
    {
        switch(attribute)
        {
            case Document::AttrType::reader:
            {
                showHideLoader();
            }
            break;
            case Document::AttrType::layout:
            case Document::AttrType::viewport:
            case Document::AttrType::path:
            case Document::AttrType::grid:
            case Document::AttrType::autoresize:
            case Document::AttrType::samplerate:
            case Document::AttrType::channels:
            case Document::AttrType::editMode:
            case Document::AttrType::drawingState:
            case Document::AttrType::description:
                break;
        }
    };

    mDocumentListener.onAccessorErased = mDocumentListener.onAccessorInserted = [=]([[maybe_unused]] Document::Accessor const& acsr, [[maybe_unused]] Document::AcsrType type, [[maybe_unused]] size_t index)
    {
        showHideLoader();
    };

    Instance::get().getDocumentAccessor().addListener(mDocumentListener, NotificationType::synchronous);
}

Application::Interface::DocumentContainer::~DocumentContainer()
{
    Instance::get().getDocumentAccessor().removeListener(mDocumentListener);
}

void Application::Interface::DocumentContainer::resized()
{
    auto bounds = getLocalBounds();
    mToolTipDisplay.setBounds(bounds.removeFromBottom(22));
    mToolTipSeparator.setBounds(bounds.removeFromBottom(1));
    auto& animator = juce::Desktop::getInstance().getAnimator();
    animator.cancelAnimation(&mPluginListTablePanel, false);
    animator.cancelAnimation(&mNeuralyzerChatPanel, false);
    animator.cancelAnimation(&mDocumentSection, false);
    mRightBorder.setBounds(bounds.removeFromRight(30));
    mRightSeparator.setBounds(bounds.removeFromRight(1));
    auto const setRightPanelBounds = [&](juce::Component& component, bool visible)
    {
        if(visible)
        {
            component.setBounds(bounds.removeFromRight(rightPanelsWidth).withWidth(rightPanelsWidth));
        }
        else
        {
            component.setBounds(bounds.withX(bounds.getWidth()).withWidth(rightPanelsWidth));
        }
    };
    setRightPanelBounds(mNeuralyzerChatPanel, mNeuralyzerChatVisible);
    setRightPanelBounds(mPluginListTablePanel, mPluginListTableVisible);
    mDocumentSection.setBounds(bounds);
    Document::Section::getMainSectionBorderSize().subtractFrom(bounds);
    bounds.reduce(4, 4);
    mLoaderDecorator.setBounds(bounds.withSizeKeepingCentre(std::min(800, bounds.getWidth()), std::min(600, bounds.getHeight())));
}

void Application::Interface::DocumentContainer::paint(juce::Graphics& g)
{
    g.setColour(findColour(juce::Label::ColourIds::textColourId));
    g.setFont(g.getCurrentFont().withHeight(12.0f).withStyle(juce::Font::FontStyleFlags::italic));
    g.drawText(juce::translate("Partiels by DEV at COMPANY").replace("DEV", "Pierre Guillot").replace("COMPANY", "IRCAM - Centre Pompidou - IMR department"), getLocalBounds().removeFromBottom(22).withTrimmedRight(12), juce::Justification::centredRight);
}

Document::Section const& Application::Interface::DocumentContainer::getDocumentSection() const
{
    return mDocumentSection;
}

PluginList::Table& Application::Interface::DocumentContainer::getPluginListTable()
{
    return mPluginListTable;
}

void Application::Interface::DocumentContainer::setRightPanelsVisible(bool const pluginListTableVisible, bool const neuralyzerPanelVisible)
{
    auto& animator = juce::Desktop::getInstance().getAnimator();
    auto bounds = getLocalBounds().withTrimmedBottom(23).withTrimmedRight(31);
    auto const right = bounds.getRight();
    auto const setPanelBounds = [&](juce::Component& component, bool visible, bool& previousState)
    {
        auto const panelBounds = [&]()
        {
            if(visible)
            {
                return bounds.removeFromRight(rightPanelsWidth).withWidth(rightPanelsWidth);
            }
            else
            {
                return bounds.withX(right).withWidth(rightPanelsWidth);
            }
        }();
        animator.animateComponent(&component, panelBounds, 1.0f, HideablePanelManager::fadeTime, true, 1.0, 1.0);
        previousState = visible;
    };
    setPanelBounds(mNeuralyzerChatPanel, neuralyzerPanelVisible, mNeuralyzerChatVisible);
    setPanelBounds(mPluginListTablePanel, pluginListTableVisible, mPluginListTableVisible);
    animator.animateComponent(&mDocumentSection, bounds, 1.0f, HideablePanelManager::fadeTime, false, 1.0, 1.0);
}

void Application::Interface::DocumentContainer::showPluginListTablePanel()
{
    mRightBorder.pluginListButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    setRightPanelsVisible(true, mNeuralyzerChatVisible);
}

void Application::Interface::DocumentContainer::hidePluginListTablePanel()
{
    mRightBorder.pluginListButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    setRightPanelsVisible(false, mNeuralyzerChatVisible);
}

void Application::Interface::DocumentContainer::togglePluginListTablePanel()
{
    if(mPluginListTableVisible)
    {
        hidePluginListTablePanel();
    }
    else
    {
        showPluginListTablePanel();
    }
}

bool Application::Interface::DocumentContainer::isPluginListTablePanelVisible() const
{
    return mPluginListTableVisible;
}

void Application::Interface::DocumentContainer::showNeuralyzerPanel()
{
    mRightBorder.neuralyzerButton.setToggleState(true, juce::NotificationType::dontSendNotification);
    setRightPanelsVisible(mPluginListTableVisible, true);
}

void Application::Interface::DocumentContainer::hideNeuralyzerPanel()
{
    mRightBorder.neuralyzerButton.setToggleState(false, juce::NotificationType::dontSendNotification);
    setRightPanelsVisible(mPluginListTableVisible, false);
}

void Application::Interface::DocumentContainer::toggleNeuralyzerPanel()
{
    if(mNeuralyzerChatVisible)
    {
        hideNeuralyzerPanel();
    }
    else
    {
        showNeuralyzerPanel();
    }
}

bool Application::Interface::DocumentContainer::isNeuralyzerPanelVisible() const
{
    return mNeuralyzerChatVisible;
}

Application::Interface::Interface()
: mOscSettingsPanel(Instance::get().getOscSender())
, mNeuralyzerSettingsPanel(Instance::get().getApplicationAccessor().getAcsr<AcsrType::neuralyzer>())
, mDocumentFileInfoPanel(Instance::get().getDocumentDirector(), Instance::get().getApplicationCommandManager())
{
    addAndMakeVisible(mPanelManager);
    // clang-format off
    mPanelManager.setContent(&mDocumentContainer,
                             std::vector<std::reference_wrapper<HideablePanel>>
                             { std::ref<HideablePanel>(mAboutPanel)
                             , std::ref<HideablePanel>(mAudioSettingsPanel)
                             , std::ref<HideablePanel>(mOscSettingsPanel)
                             , std::ref<HideablePanel>(mNeuralyzerSettingsPanel)
                             , std::ref<HideablePanel>(mBatcherPanel)
                             , std::ref<HideablePanel>(mConverterPanel)
                             , std::ref<HideablePanel>(mExporterPanel)
                             , std::ref<HideablePanel>(mPluginSearchPathPanel)
                             , std::ref<HideablePanel>(mGraphicPresetPanel)
                             , std::ref<HideablePanel>(mReaderLayoutPanel)
                             , std::ref<HideablePanel>(mDocumentFileInfoPanel)
                             , std::ref<HideablePanel>(mKeyMappingsPanel)
                             });
    // clang-format on
    mDocumentReceiver.onSignal = [this]([[maybe_unused]] Document::Accessor const& acsr, Document::SignalType signal, [[maybe_unused]] juce::var value)
    {
        switch(signal)
        {
            case Document::SignalType::viewport:
            case Document::SignalType::isLoading:
                break;
            case Document::SignalType::showReaderLayoutPanel:
            {
                showReaderLayoutPanel();
            }
            break;
            case Document::SignalType::showDocumentFilePanel:
            {
                showDocumentFileInfoPanel();
            }
            break;
        }
    };
    Instance::get().getDocumentAccessor().addReceiver(mDocumentReceiver);
    Instance::get().getDocumentDirector().setPluginTable(std::addressof(getPluginListTable()), [this](bool show)
                                                         {
                                                             if(show)
                                                             {
                                                                 showPluginListTablePanel();
                                                             }
                                                             else
                                                             {
                                                                 hidePluginListTablePanel();
                                                             }
                                                         });
}

Application::Interface::~Interface()
{
    Instance::get().getDocumentDirector().setPluginTable(nullptr, nullptr);
    Instance::get().getDocumentAccessor().removeReceiver(mDocumentReceiver);
}

void Application::Interface::resized()
{
    mPanelManager.setBounds(getLocalBounds());
}

void Application::Interface::hideCurrentPanel()
{
    mPanelManager.hide();
}

void Application::Interface::showAboutPanel()
{
    mPanelManager.show(mAboutPanel);
}

void Application::Interface::showAudioSettingsPanel()
{
    mPanelManager.show(mAudioSettingsPanel);
}

void Application::Interface::showOscSettingsPanel()
{
    mPanelManager.show(mOscSettingsPanel);
}

void Application::Interface::showNeuralyzerSettingsPanel()
{
    mPanelManager.show(mNeuralyzerSettingsPanel);
}

void Application::Interface::showGraphicPresetPanel()
{
    mPanelManager.show(mGraphicPresetPanel);
}

void Application::Interface::showBatcherPanel()
{
    mPanelManager.show(mBatcherPanel);
}

void Application::Interface::showConverterPanel()
{
    mPanelManager.show(mConverterPanel);
}

void Application::Interface::showExporterPanel()
{
    mPanelManager.show(mExporterPanel);
}

void Application::Interface::showPluginSearchPathPanel()
{
    mPanelManager.show(mPluginSearchPathPanel);
}

void Application::Interface::showReaderLayoutPanel()
{
    mPanelManager.show(mReaderLayoutPanel);
}

void Application::Interface::showDocumentFileInfoPanel()
{
    mPanelManager.show(mDocumentFileInfoPanel);
}

void Application::Interface::showKeyMappingsPanel()
{
    mPanelManager.show(mKeyMappingsPanel);
}

void Application::Interface::showPluginListTablePanel()
{
    mDocumentContainer.showPluginListTablePanel();
}

void Application::Interface::hidePluginListTablePanel()
{
    mDocumentContainer.hidePluginListTablePanel();
}

void Application::Interface::togglePluginListTablePanel()
{
    mDocumentContainer.togglePluginListTablePanel();
}

bool Application::Interface::isPluginListTablePanelVisible() const
{
    return mDocumentContainer.isPluginListTablePanelVisible();
}

void Application::Interface::showNeuralyzerPanel()
{
    mDocumentContainer.showNeuralyzerPanel();
}

void Application::Interface::hideNeuralyzerPanel()
{
    mDocumentContainer.hideNeuralyzerPanel();
}

void Application::Interface::toggleNeuralyzerPanel()
{
    mDocumentContainer.toggleNeuralyzerPanel();
}

bool Application::Interface::isNeuralyzerPanelVisible() const
{
    return mDocumentContainer.isNeuralyzerPanelVisible();
}

juce::Component const* Application::Interface::getPlot(juce::String const& identifier) const
{
    return mDocumentContainer.getDocumentSection().getPlot(identifier);
}

PluginList::Table& Application::Interface::getPluginListTable()
{
    return mDocumentContainer.getPluginListTable();
}

ANALYSE_FILE_END
