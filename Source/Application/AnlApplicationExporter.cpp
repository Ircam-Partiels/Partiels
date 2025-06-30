#include "AnlApplicationExporter.h"
#include "../Document/AnlDocumentTools.h"
#include "../Group/AnlGroupExporter.h"
#include "../Track/AnlTrackExporter.h"
#include "../Track/AnlTrackTools.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::ExporterContent::ExporterContent()
: mExporterPanel(Instance::get().getDocumentAccessor(), true, true)
, mPropertyExport("Export", "Export the results", [this]()
                  {
                      if(mProcess.valid())
                      {
                          mShoulAbort.store(true);
                          mPropertyExport.entry.setButtonText(juce::translate("Aborting..."));
                          mPropertyExport.entry.setTooltip(juce::translate("Aborting the export..."));
                          mPropertyExport.setEnabled(false);
                      }
                      else
                      {
                          exportToFile();
                      }
                  })
{
    addAndMakeVisible(mExporterPanel);
    addAndMakeVisible(mPropertyExport);
    mPropertyExport.entry.addShortcut(juce::KeyPress(juce::KeyPress::returnKey));
    addAndMakeVisible(mLoadingIcon);
    mComponentListener.onComponentResized = [this](juce::Component const&)
    {
        resized();
    };
    mComponentListener.attachTo(mExporterPanel);

    mListener.onAttrChanged = [this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::defaultTemplateFile:
            case AttrType::currentTranslationFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::adaptationToSampleRate:
            case AttrType::autoLoadConvertedFile:
            case AttrType::silentFileManagement:
            case AttrType::desktopGlobalScaleFactor:
            case AttrType::routingMatrix:
            case AttrType::autoUpdate:
            case AttrType::lastVersion:
            case AttrType::timeZoomAnchorOnPlayhead:
                break;
            case AttrType::exportOptions:
            {
                auto const options = acsr.getAttr<AttrType::exportOptions>();
                mExporterPanel.setOptions(options, juce::NotificationType::dontSendNotification);
                mPropertyExport.setEnabled(options.isValid());
            }
            break;
        }
    };

    mExporterPanel.onOptionsChanged = [this]()
    {
        auto& acsr = Instance::get().getApplicationAccessor();
        acsr.setAttr<AttrType::exportOptions>(mExporterPanel.getOptions(), NotificationType::synchronous);
    };

    auto& acsr = Instance::get().getApplicationAccessor();
    acsr.addListener(mListener, NotificationType::synchronous);
    setSize(300, 200);
}

Application::ExporterContent::~ExporterContent()
{
    auto& acsr = Instance::get().getApplicationAccessor();
    acsr.removeListener(mListener);
    mComponentListener.detachFrom(mExporterPanel);
    if(mProcess.valid())
    {
        mProcess.get();
    }
}

void Application::ExporterContent::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    mExporterPanel.setBounds(bounds.removeFromTop(mExporterPanel.getHeight()));
    mPropertyExport.setBounds(bounds.removeFromTop(mPropertyExport.getHeight()));
    mLoadingIcon.setBounds(bounds.removeFromTop(22).withSizeKeepingCentre(22, 22));
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::ExporterContent::exportToFile()
{
    auto const& acsr = Instance::get().getApplicationAccessor();
    auto const& options = acsr.getAttr<AttrType::exportOptions>();
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const identifier = mExporterPanel.getSelectedIdentifier();
    auto const timeRange = mExporterPanel.getTimeRange();
    auto const useDirectory = identifier.isEmpty() || (Document::Tools::hasGroupAcsr(documentAcsr, identifier) && (!options.useGroupOverview || options.useTextFormat()));

    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Export as FORMATNAME").replace("FORMATNAME", options.getFormatName()), juce::File{}, useDirectory ? "" : options.getFormatWilcard());
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    auto const fcFlags = useDirectory ? (Flags::canSelectDirectories | Flags::openMode) : (Flags::saveMode | Flags::canSelectFiles | Flags::warnAboutOverwriting);
    mFileChooser->launchAsync(fcFlags, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  auto const results = fileChooser.getResults();
                                  if(results.isEmpty())
                                  {
                                      return;
                                  }
                                  anlWeakAssert(!mProcess.valid());
                                  if(mProcess.valid())
                                  {
                                      return;
                                  }

                                  mLoadingIcon.setActive(true);
                                  juce::MouseCursor::showWaitCursor();

                                  mExporterPanel.setEnabled(false);
                                  mPropertyExport.entry.setButtonText(juce::translate("Abort"));
                                  mPropertyExport.entry.setTooltip(juce::translate("Abort the export"));

                                  mShoulAbort.store(false);
                                  mProcess = std::async([=, this, file = results.getFirst()]() -> ProcessResult
                                                        {
                                                            juce::Thread::setCurrentThreadName("ExporterContent");
                                                            auto const result = Document::Exporter::toFile(Instance::get().getDocumentAccessor(), file, timeRange, {}, "", identifier, options, mShoulAbort);
                                                            triggerAsyncUpdate();
                                                            if(result.failed())
                                                            {
                                                                return std::make_tuple(AlertWindow::MessageType::warning, juce::translate("Export as FORMATNAME failed!").replace("FORMATNAME", options.getFormatName()), result.getErrorMessage());
                                                            }
                                                            return std::make_tuple(AlertWindow::MessageType::info, juce::translate("Export as FORMATNAME succeeded!").replace("FORMATNAME", options.getFormatName()), juce::translate("The analyses have been exported as FORMATNAME to FILENAME.").replace("FORMATNAME", options.getFormatName()).replace("FILENAME", file.getFullPathName()));
                                                        });
                              });
}

void Application::ExporterContent::handleAsyncUpdate()
{
    mExporterPanel.setEnabled(true);
    mPropertyExport.setEnabled(true);

    mShoulAbort.store(false);
    mPropertyExport.entry.setButtonText(juce::translate("Export"));
    mPropertyExport.entry.setTooltip(juce::translate("Export the results"));

    mLoadingIcon.setActive(false);
    juce::MouseCursor::hideWaitCursor();
    anlWeakAssert(mProcess.valid());
    if(!mProcess.valid())
    {
        return;
    }
    auto const result = mProcess.get();
    auto const options = juce::MessageBoxOptions()
                             .withIconType(static_cast<juce::MessageBoxIconType>(std::get<0>(result)))
                             .withTitle(std::get<1>(result))
                             .withMessage(std::get<2>(result))
                             .withButton(juce::translate("Ok"));
    juce::AlertWindow::showAsync(options, nullptr);
}

bool Application::ExporterContent::canCloseWindow() const
{
    return !mLoadingIcon.isActive();
}

Application::ExporterPanel::ExporterPanel()
: HideablePanelTyped<ExporterContent>(juce::translate("Export..."))
{
}

bool Application::ExporterPanel::escapeKeyPressed()
{
    if(!mContent.canCloseWindow())
    {
        return false;
    }
    hide();
    return true;
}

ANALYSE_FILE_END
