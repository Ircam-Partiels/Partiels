#include "AnlApplicationBatcher.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Batcher::Batcher()
: FloatingWindowContainer("Batch Processing", *this)
, mDocumentDirector(mDocumentAccessor, Instance::get().getAudioFormatManager(), mUndoManager)
, mAudioFileLayoutTable(Instance::get().getAudioFormatManager(), AudioFileLayoutTable::SupportMode::all, AudioFileLayout::ChannelLayout::all)
, mExporterPanel(Instance::get().getDocumentAccessor(), nullptr)
, mPropertyExport("Process", "Process the files", [this]()
                  {
                      if(mProcess.valid())
                      {
                          mShoulAbort.store(true);
                          mPropertyExport.entry.setButtonText(juce::translate("Aborting..."));
                          mPropertyExport.setEnabled(false);
                      }
                      else
                      {
                          process();
                      }
                  })
{
    // Add clear button
    mAudioFileLayoutTable.onLayoutChanged = [this]()
    {
        mPropertyExport.setEnabled(!mAudioFileLayoutTable.getLayout().empty());
    };

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
                break;
            case AttrType::exportOptions:
            {
                auto options = acsr.getAttr<AttrType::exportOptions>();
                options.useAutoSize = false;
                mExporterPanel.setOptions(acsr.getAttr<AttrType::exportOptions>(), juce::NotificationType::dontSendNotification);
            }
            break;
        }
    };

    mBoundsListener.onComponentResized = [this](juce::Component const&)
    {
        resized();
    };
    mBoundsListener.attachTo(mExporterPanel);

    auto& acsr = Instance::get().getApplicationAccessor();
    acsr.addListener(mListener, NotificationType::synchronous);

    addAndMakeVisible(mAudioFileLayoutTable);
    addAndMakeVisible(mSeparator);
    addAndMakeVisible(mExporterPanel);
    addAndMakeVisible(mPropertyExport);
    addAndMakeVisible(mLoadingCircle);
    setSize(300, 200);
}

Application::Batcher::~Batcher()
{
    auto& acsr = Instance::get().getApplicationAccessor();
    acsr.removeListener(mListener);

    mBoundsListener.detachFrom(mExporterPanel);
}

void Application::Batcher::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mAudioFileLayoutTable.setBounds(bounds.removeFromTop(200));
    mSeparator.setBounds(bounds.removeFromTop(1));
    mExporterPanel.setBounds(bounds.removeFromTop(mExporterPanel.getHeight()));
    mPropertyExport.setBounds(bounds.removeFromTop(mPropertyExport.getHeight()));
    mLoadingCircle.setBounds(bounds.removeFromTop(22).withSizeKeepingCentre(22, 22));
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::Batcher::showAt(juce::Point<int> const& pt)
{
    FloatingWindowContainer::showAt(pt);
    mFloatingWindow.runModalLoop();
}

void Application::Batcher::handleAsyncUpdate()
{
    mAudioFileLayoutTable.setEnabled(true);
    mExporterPanel.setEnabled(true);
    mPropertyExport.setEnabled(true);

    mShoulAbort.store(false);
    mPropertyExport.entry.setButtonText(juce::translate("Export"));
    mFloatingWindow.onCloseButtonPressed = nullptr;
    mLoadingCircle.setActive(false);
    juce::MouseCursor::hideWaitCursor();
    anlWeakAssert(mProcess.valid());
    if(!mProcess.valid())
    {
        return;
    }
    auto const result = mProcess.get();
    AlertWindow::showMessage(std::get<0>(result), std::get<1>(result), std::get<2>(result));
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

    juce::FileChooser fc(juce::translate("Process files to..."), {}, "");
    if(!fc.browseForDirectory())
    {
        return;
    }
    auto const file = fc.getResult();

    mLoadingCircle.setActive(true);
    juce::MouseCursor::showWaitCursor();
    mFloatingWindow.onCloseButtonPressed = [this]()
    {
        getLookAndFeel().playAlertSound();
        return false;
    };

    mAudioFileLayoutTable.setEnabled(false);
    mExporterPanel.setEnabled(false);
    mPropertyExport.entry.setButtonText(juce::translate("Abort"));

    mShoulAbort.store(false);
    mDocumentAccessor.copyFrom(Instance::get().getDocumentAccessor(), NotificationType::synchronous);
    mDocumentAccessor.setAttr<Document::AttrType::reader>(std::vector<AudioFileLayout>{}, NotificationType::synchronous);
    for(auto const acsr : mDocumentAccessor.getAcsrs<Document::AcsrType::tracks>())
    {
        auto const resultFile = acsr.get().getAttr<Track::AttrType::file>();
        if(resultFile.hasFileExtension("dat"))
        {
            acsr.get().setAttr<Track::AttrType::file>(juce::File{}, NotificationType::synchronous);
        }
    }

    mProcess = std::async([=, this]() -> ProcessResult
                          {
                              juce::Thread::setCurrentThreadName("Batch");
                              juce::MessageManager::Lock lock;

                              if(!lock.tryEnter())
                              {
                                  triggerAsyncUpdate();
                                  return std::make_tuple(AlertWindow::MessageType::warning, juce::translate("Batch processing failed!"), "Message manager could be unlocked!");
                              }
                              auto const trackAcsrs = mDocumentAccessor.getAcsrs<Document::AcsrType::tracks>();
                              lock.exit();
        
                              for(auto const& layout : layouts)
                              {
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

                                  if(!lock.tryEnter())
                                  {
                                      triggerAsyncUpdate();
                                      return std::make_tuple(AlertWindow::MessageType::warning, juce::translate("Batch processing failed!"), "Message manager could be unlocked!");
                                  }
                                  mDocumentAccessor.setAttr<Document::AttrType::reader>({layout}, NotificationType::synchronous);
                                  mDocumentAccessor.getAcsr<Document::AcsrType::timeZoom>().setAttr<Zoom::AttrType::visibleRange>(Zoom::Range{0.0, std::numeric_limits<double>::max()}, NotificationType::synchronous);
                                  for(auto trackAcsr : trackAcsrs)
                                  {
                                      auto trackChannelsLayout = trackAcsr.get().getAttr<Track::AttrType::channelsLayout>();
                                      std::fill(trackChannelsLayout.begin(), trackChannelsLayout.end(), true);
                                      trackAcsr.get().setAttr<Track::AttrType::channelsLayout>(trackChannelsLayout, NotificationType::synchronous);
                                  }
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
}

ANALYSE_FILE_END
