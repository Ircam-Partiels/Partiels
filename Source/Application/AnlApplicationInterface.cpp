#include "AnlApplicationInterface.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Application::Interface::TrackLoaderPanel::TrackLoaderPanel()
: HideablePanelTyped<Track::Loader::ArgumentSelector>(juce::translate("Load File..."))
{
}

Track::Loader::ArgumentSelector& Application::Interface::TrackLoaderPanel::getArgumentSelector()
{
    return mContent;
}

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

void Application::Interface::PluginListTablePanel::colourChanged()
{
    setOpaque(findColour(juce::ResizableWindow::backgroundColourId).isOpaque());
}

void Application::Interface::PluginListTablePanel::parentHierarchyChanged()
{
    colourChanged();
}

Application::Interface::DocumentContainer::DocumentContainer()
: mDocumentSection(Instance::get().getDocumentDirector(), Instance::get().getApplicationCommandManager())
, mPluginListTable(Instance::get().getPluginListAccessor(), Instance::get().getPluginListScanner())
{
    mPluginListTable.setMultipleSelectionEnabled(true);
    mPluginListTable.onAddPlugins = [](std::set<Plugin::Key> keys)
    {
        Tools::addPluginTracks(Tools::getNewTrackPosition(), keys);
    };

    addAndMakeVisible(mDocumentSection);
    addChildComponent(mLoaderDecorator);
    addAndMakeVisible(mPluginListTablePanel);
    addAndMakeVisible(mToolTipSeparator);
    addAndMakeVisible(mToolTipDisplay);

    mDocumentSection.pluginListButton.getProperties().set("Font", juce::Font(juce::FontOptions(juce::Font::getDefaultSansSerifFontName(), 8.0, juce::Font::plain)).toString());
    mDocumentSection.pluginListButton.setButtonText(juce::CharPointer_UTF8("\xe2\x86\x90"));
    mDocumentSection.pluginListButton.onClick = []()
    {
        if(auto* window = Instance::get().getWindow())
        {
            window->getInterface().togglePluginListTablePanel();
        }
    };

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
    animator.cancelAnimation(std::addressof(mPluginListTablePanel), false);
    animator.cancelAnimation(std::addressof(mDocumentSection), false);
    if(mPluginListTableVisible)
    {
        mPluginListTablePanel.setBounds(bounds.removeFromRight(pluginListTableWidth).withWidth(pluginListTableWidth));
    }
    else
    {
        mPluginListTablePanel.setBounds(bounds.withX(bounds.getWidth()).withWidth(pluginListTableWidth));
    }
    mDocumentSection.setBounds(bounds);
    Document::Section::getMainSectionBorderSize().subtractFrom(bounds);
    bounds.reduce(4, 4);
    mLoaderDecorator.setBounds(bounds.withSizeKeepingCentre(std::min(800, bounds.getWidth()), std::min(600, bounds.getHeight())));
}

Document::Section const& Application::Interface::DocumentContainer::getDocumentSection() const
{
    return mDocumentSection;
}

PluginList::Table& Application::Interface::DocumentContainer::getPluginListTable()
{
    return mPluginListTable;
}

void Application::Interface::DocumentContainer::showPluginListTablePanel()
{
    mPluginListTableVisible = true;
    auto& animator = juce::Desktop::getInstance().getAnimator();
    auto bounds = getLocalBounds().withTrimmedBottom(23);
    animator.animateComponent(std::addressof(mPluginListTablePanel), bounds.removeFromRight(pluginListTableWidth).withWidth(pluginListTableWidth), 1.0f, HideablePanelManager::fadeTime, true, 1.0, 1.0);
    animator.animateComponent(std::addressof(mDocumentSection), bounds, 1.0f, HideablePanelManager::fadeTime, false, 1.0, 1.0);
    mDocumentSection.pluginListButton.setButtonText(juce::CharPointer_UTF8("\xe2\x86\x92"));
}

void Application::Interface::DocumentContainer::hidePluginListTablePanel()
{
    mPluginListTableVisible = false;
    auto& animator = juce::Desktop::getInstance().getAnimator();
    auto const bounds = getLocalBounds().withTrimmedBottom(23);
    animator.animateComponent(std::addressof(mPluginListTablePanel), bounds.withX(bounds.getWidth()).withWidth(pluginListTableWidth), 1.0f, HideablePanelManager::fadeTime, true, 1.0, 1.0);
    animator.animateComponent(std::addressof(mDocumentSection), bounds, 1.0f, HideablePanelManager::fadeTime, false, 1.0, 1.0);
    mDocumentSection.pluginListButton.setButtonText(juce::CharPointer_UTF8("\xe2\x86\x90"));
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

Application::Interface::Interface()
: mOscSettingsPanel(Instance::get().getOscSender())
{
    addAndMakeVisible(mPanelManager);
    // clang-format off
    mPanelManager.setContent(&mDocumentContainer,
                             std::vector<std::reference_wrapper<HideablePanel>>
                             { std::ref<HideablePanel>(mAboutPanel)
                             , std::ref<HideablePanel>(mAudioSettingsPanel)
                             , std::ref<HideablePanel>(mOscSettingsPanel)
                             , std::ref<HideablePanel>(mBatcherPanel)
                             , std::ref<HideablePanel>(mConverterPanel)
                             , std::ref<HideablePanel>(mExporterPanel)
                             , std::ref<HideablePanel>(mPluginSearchPathPanel)
                             , std::ref<HideablePanel>(mReaderLayoutPanel)
                             , std::ref<HideablePanel>(mKeyMappingsPanel)
                             , std::ref<HideablePanel>(mTrackLoaderPanel)
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
    Instance::get().getDocumentDirector().setLoaderSelector(std::addressof(getTrackLoaderArgumentSelector()), [this](bool show)
                                                            {
                                                                if(show)
                                                                {
                                                                    showTrackLoaderPanel();
                                                                }
                                                                else
                                                                {
                                                                    hideCurrentPanel();
                                                                }
                                                            });
}

Application::Interface::~Interface()
{
    Instance::get().getDocumentDirector().setPluginTable(nullptr, nullptr);
    Instance::get().getDocumentDirector().setLoaderSelector(nullptr, nullptr);
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

void Application::Interface::showKeyMappingsPanel()
{
    mPanelManager.show(mKeyMappingsPanel);
}

void Application::Interface::showTrackLoaderPanel()
{
    mPanelManager.show(mTrackLoaderPanel);
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

juce::Rectangle<int> Application::Interface::getPlotBounds(juce::String const& identifier) const
{
    return mDocumentContainer.getDocumentSection().getPlotBounds(identifier);
}

PluginList::Table& Application::Interface::getPluginListTable()
{
    return mDocumentContainer.getPluginListTable();
}

Track::Loader::ArgumentSelector& Application::Interface::getTrackLoaderArgumentSelector()
{
    return mTrackLoaderPanel.getArgumentSelector();
}

ANALYSE_FILE_END
