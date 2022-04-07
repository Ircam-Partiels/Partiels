#include "AnlApplicationBatcher.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Batcher::WindowContainer::WindowContainer(Batcher& batcher)
: FloatingWindowContainer(juce::translate("Batch"), batcher)
, mBatcher(batcher)
, mTooltip(&mBatcher)
{
    mFloatingWindow.onCloseButtonPressed = [this]()
    {
        if(mBatcher.canCloseWindow())
        {
            return true;
        }
        getLookAndFeel().playAlertSound();
        return false;
    };
}

Application::Batcher::WindowContainer::~WindowContainer()
{
    mFloatingWindow.onCloseButtonPressed = nullptr;
}

Application::Batcher::Batcher()
: mDocumentDirector(mDocumentAccessor, Instance::get().getAudioFormatManager(), mUndoManager)
, mAudioFileLayoutTable(Instance::get().getAudioFormatManager(), AudioFileLayoutTable::SupportMode::all, AudioFileLayout::ChannelLayout::all)
, mExporterPanel(Instance::get().getDocumentAccessor(), nullptr)
, mPropertyAdaptationToSampleRate("Adapt to Sample Rate", "Adapt the block size and the step size of the analyzes to the sample rate", [](bool state)
                                  {
                                      auto& acsr = Instance::get().getApplicationAccessor();
                                      acsr.setAttr<AttrType::adaptationToSampleRate>(state, NotificationType::synchronous);
                                  })
, mPropertyExport("Process", "Process the files", [this]()
                  {
                      if(mProcess.valid())
                      {
                          mShoulAbort.store(true);
                          mPropertyExport.entry.setButtonText(juce::translate("Aborting..."));
                          mPropertyExport.entry.setTooltip(juce::translate("Batch processing aborting..."));
                          mAudioFileLayoutTable.onLayoutChanged();
                      }
                      else
                      {
                          process();
                      }
                  })
{
    mAudioFileLayoutTable.onLayoutChanged = [this]()
    {
        mPropertyExport.setEnabled(!mAudioFileLayoutTable.getLayout().empty());
    };
    mAudioFileLayoutTable.onLayoutChanged();

    mExporterPanel.onOptionsChanged = [this]()
    {
        auto& acsr = Instance::get().getApplicationAccessor();
        auto options = mExporterPanel.getOptions();
        options.useAutoSize = acsr.getAttr<AttrType::exportOptions>().useAutoSize;
        acsr.setAttr<AttrType::exportOptions>(options, NotificationType::synchronous);
    };

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        switch(attribute)
        {
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::autoLoadConvertedFile:
            case AttrType::desktopGlobalScaleFactor:
                break;
            case AttrType::exportOptions:
            {
                auto options = acsr.getAttr<AttrType::exportOptions>();
                options.useAutoSize = false;
                mExporterPanel.setOptions(options, juce::NotificationType::dontSendNotification);
                mPropertyExport.setEnabled(options.isValid());
            }
            break;
            case AttrType::adaptationToSampleRate:
            {
                mPropertyAdaptationToSampleRate.entry.setToggleState(acsr.getAttr<AttrType::adaptationToSampleRate>(), juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };

    mComponentListener.onComponentResized = [this](juce::Component const&)
    {
        resized();
    };
    mComponentListener.attachTo(mExporterPanel);

    auto& acsr = Instance::get().getApplicationAccessor();
    acsr.addListener(mListener, NotificationType::synchronous);

    addAndMakeVisible(mAudioFileLayoutTable);
    addAndMakeVisible(mSeparator);
    addAndMakeVisible(mExporterPanel);
    addAndMakeVisible(mPropertyExport);
    addAndMakeVisible(mPropertyAdaptationToSampleRate);
    addAndMakeVisible(mLoadingCircle);
    setSize(300, 200);
}

Application::Batcher::~Batcher()
{
    auto& acsr = Instance::get().getApplicationAccessor();
    acsr.removeListener(mListener);

    mComponentListener.detachFrom(mExporterPanel);
}

void Application::Batcher::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mAudioFileLayoutTable.setBounds(bounds.removeFromTop(200));
    mSeparator.setBounds(bounds.removeFromTop(1));
    mExporterPanel.setBounds(bounds.removeFromTop(mExporterPanel.getHeight()));
    mPropertyAdaptationToSampleRate.setBounds(bounds.removeFromTop(mPropertyAdaptationToSampleRate.getHeight()));
    mPropertyExport.setBounds(bounds.removeFromTop(mPropertyExport.getHeight()));
    mLoadingCircle.setBounds(bounds.removeFromTop(22).withSizeKeepingCentre(22, 22));
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::Batcher::handleAsyncUpdate()
{
    mAudioFileLayoutTable.setEnabled(true);
    mExporterPanel.setEnabled(true);
    mPropertyAdaptationToSampleRate.setEnabled(true);
    mAudioFileLayoutTable.onLayoutChanged();

    mShoulAbort.store(false);
    mPropertyExport.entry.setButtonText(juce::translate("Process"));
    mPropertyExport.entry.setTooltip(juce::translate("Launch the batch processing."));

    mLoadingCircle.setActive(false);
    juce::MouseCursor::hideWaitCursor();
    anlWeakAssert(mProcess.valid());
    if(!mProcess.valid())
    {
        return;
    }
    mDocumentDirector.setAlertCatcher(nullptr);
    auto const result = mProcess.get();
    auto message = std::get<2>(result);
    auto const alertMessages = mAlertCatcher.getMessages();
    if(!alertMessages.empty())
    {
        message += "\n\n" + juce::translate("Alert messages have been generated during the batch processing:");
    }
    for(auto const& alertMessage : alertMessages)
    {
        message += "\n";
        message += juce::String(std::string(magic_enum::enum_name(std::get<0>(alertMessage.first)).substr())).toUpperCase() + " - ";
        message += std::get<1>(alertMessage.first) + (alertMessage.second.size() > 1 ? "(" + juce::String(alertMessage.second.size()) + ")" : ": ");
        message += alertMessage.second.joinIntoString(" - ");
    }
    auto const options = juce::MessageBoxOptions()
                             .withIconType(static_cast<juce::AlertWindow::AlertIconType>(std::get<0>(result)))
                             .withTitle(std::get<1>(result))
                             .withMessage(message)
                             .withButton(juce::translate("Ok"));
    juce::AlertWindow::showAsync(options, nullptr);
}

void Application::Batcher::process()
{
    anlWeakAssert(!mProcess.valid());
    if(mProcess.valid())
    {
        return;
    }

    auto const layouts = mAudioFileLayoutTable.getLayout();
    anlWeakAssert(!layouts.empty());
    if(layouts.empty())
    {
        return;
    }

    auto options = mExporterPanel.getOptions();
    options.useAutoSize = false;
    auto const identifier = mExporterPanel.getSelectedIdentifier();

    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Process files to..."));
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    mFileChooser->launchAsync(Flags::openMode | Flags::canSelectDirectories, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  auto copyAcsr = std::make_unique<Document::Accessor>();
                                  if(copyAcsr == nullptr)
                                  {
                                      return;
                                  }
                                  auto const files = fileChooser.getResults();
                                  if(files.isEmpty())
                                  {
                                      return;
                                  }
                                  auto const file = files.getFirst();

                                  mLoadingCircle.setActive(true);
                                  juce::MouseCursor::showWaitCursor();

                                  mAudioFileLayoutTable.setEnabled(false);
                                  mExporterPanel.setEnabled(false);
                                  mPropertyAdaptationToSampleRate.setEnabled(false);
                                  mPropertyExport.entry.setButtonText(juce::translate("Abort"));
                                  mPropertyExport.entry.setTooltip(juce::translate("Abort the batch processing."));

                                  mShoulAbort.store(false);

                                  auto const adaptationToSampleRate = Instance::get().getApplicationAccessor().getAttr<AttrType::adaptationToSampleRate>();
                                  auto const originalSampleRate = Instance::get().getDocumentAccessor().getAttr<Document::AttrType::samplerate>();

                                  copyAcsr->copyFrom(Instance::get().getDocumentAccessor(), NotificationType::synchronous);
                                  copyAcsr->setAttr<Document::AttrType::reader>(std::vector<AudioFileLayout>{}, NotificationType::synchronous);
                                  for(auto const& acsr : copyAcsr->getAcsrs<Document::AcsrType::tracks>())
                                  {
                                      auto const resultFile = acsr.get().getAttr<Track::AttrType::file>().file;
                                      if(resultFile.hasFileExtension("dat"))
                                      {
                                          acsr.get().setAttr<Track::AttrType::file>(Track::FileInfo{}, NotificationType::synchronous);
                                      }
                                  }
                                  mAlertCatcher.clearMessages();
                                  mDocumentDirector.setAlertCatcher(&mAlertCatcher);

                                  mProcess = std::async([=, copyAcsr = std::move(copyAcsr), this]() -> ProcessResult
                                                        {
                                                            juce::Thread::setCurrentThreadName("Batch");
                                                            juce::MessageManager::Lock lock;

                                                            for(auto const& layout : layouts)
                                                            {
                                                                if(!lock.tryEnter())
                                                                {
                                                                    triggerAsyncUpdate();
                                                                    return std::make_tuple(AlertWindow::MessageType::warning, juce::translate("Batch processing failed!"), "Message manager could be unlocked!");
                                                                }

                                                                mDocumentAccessor.copyFrom({}, NotificationType::synchronous);
                                                                mDocumentAccessor.setAttr<Document::AttrType::reader>({layout}, NotificationType::synchronous);
                                                                auto const currentSampleRate = mDocumentAccessor.getAttr<Document::AttrType::samplerate>();

                                                                {
                                                                    Document::Accessor tempAcsr;
                                                                    tempAcsr.copyFrom(*copyAcsr.get(), NotificationType::synchronous);
                                                                    tempAcsr.setAttr<Document::AttrType::reader>({layout}, NotificationType::synchronous);
                                                                    for(auto trackAcsr : tempAcsr.getAcsrs<Document::AcsrType::tracks>())
                                                                    {
                                                                        auto trackChannelsLayout = trackAcsr.get().getAttr<Track::AttrType::channelsLayout>();
                                                                        std::fill(trackChannelsLayout.begin(), trackChannelsLayout.end(), true);
                                                                        trackAcsr.get().setAttr<Track::AttrType::channelsLayout>(trackChannelsLayout, NotificationType::synchronous);
                                                                    }
                                                                    if(adaptationToSampleRate && originalSampleRate > 0.0)
                                                                    {
                                                                        auto const ratio = currentSampleRate / originalSampleRate;
                                                                        for(auto trackAcsr : tempAcsr.getAcsrs<Document::AcsrType::tracks>())
                                                                        {
                                                                            auto state = trackAcsr.get().getAttr<Track::AttrType::state>();
                                                                            state.blockSize = static_cast<size_t>(std::round(static_cast<double>(state.blockSize) * ratio));
                                                                            state.stepSize = static_cast<size_t>(std::round(static_cast<double>(state.stepSize) * ratio));
                                                                            trackAcsr.get().setAttr<Track::AttrType::state>(state, NotificationType::synchronous);
                                                                        }
                                                                    }

                                                                    mDocumentAccessor.copyFrom(tempAcsr, NotificationType::synchronous);
                                                                }

                                                                mDocumentAccessor.getAcsr<Document::AcsrType::timeZoom>().setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, std::numeric_limits<double>::max()}, NotificationType::synchronous);

                                                                auto const trackAcsrs = mDocumentAccessor.getAcsrs<Document::AcsrType::tracks>();
                                                                lock.exit();

                                                                while(std::any_of(trackAcsrs.cbegin(), trackAcsrs.cend(), [&](auto const& trackAcsr)
                                                                                  {
                                                                                      if(!lock.tryEnter())
                                                                                      {
                                                                                          return false;
                                                                                      }
                                                                                      auto const processing = trackAcsr.get().template getAttr<Track::AttrType::processing>();
                                                                                      lock.exit();
                                                                                      return std::get<0>(processing) || std::get<2>(processing);
                                                                                  }))
                                                                {
                                                                    using namespace std::chrono_literals;
                                                                    std::this_thread::sleep_for(20ms);
                                                                }

                                                                auto const result = Document::Exporter::toFile(mDocumentAccessor, file, layout.file.getFileNameWithoutExtension() + " ", identifier, options, mShoulAbort, nullptr);
                                                                if(result.failed())
                                                                {
                                                                    triggerAsyncUpdate();
                                                                    return std::make_tuple(AlertWindow::MessageType::warning, juce::translate("Batch processing failed!"), result.getErrorMessage());
                                                                }
                                                            }
                                                            triggerAsyncUpdate();
                                                            return std::make_tuple(AlertWindow::MessageType::info, juce::translate("Batch processing succeeded!"), juce::translate("The files have been successfully exported to DIRNAME.").replace("DIRNAME", file.getFullPathName()));
                                                        });
                              });
}

bool Application::Batcher::canCloseWindow() const
{
    return !mLoadingCircle.isActive();
}

ANALYSE_FILE_END
