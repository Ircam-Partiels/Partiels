#include "AnlDocumentDirector.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"
#include "../Analyzer/AnlAnalyzerPropertyPanel.h"

ANALYSE_FILE_BEGIN

Document::Director::Director(Accessor& accessor, PluginList::Accessor& pluginAccessor, juce::AudioFormatManager const& audioFormatManager)
: mAccessor(accessor)
, mAudioFormatManager(audioFormatManager)
, mPluginListTable(pluginAccessor)
{
}

void Document::Director::loadAudioFile(juce::File const& file, AlertType alertType)
{
    auto const fileExtension = file.getFileExtension();
    if(mAudioFormatManager.getWildcardForAllFormats().contains(fileExtension))
    {
        mAccessor.setAttr<AttrType::file>(file, NotificationType::synchronous);
        auto reader = createAudioFormatReader(mAccessor, mAudioFormatManager, alertType);
        if(reader != nullptr)
        {
            auto const sampleRate = reader->sampleRate;
            auto const duration = sampleRate > 0.0 ? static_cast<double>(reader->lengthInSamples) / sampleRate : 0.0;
            Zoom::range_type const range = {0.0, duration};
            auto& zoomAcsr = mAccessor.getAccessor<AttrType::timeZoom>(0);
            zoomAcsr.setAttr<Zoom::AttrType::globalRange>(range, NotificationType::synchronous);
            zoomAcsr.setAttr<Zoom::AttrType::visibleRange>(range, NotificationType::synchronous);
        }
    }
    else if(alertType == AlertType::window)
    {
        auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
        auto const title = juce::translate("File format unsupported");
        auto const message = juce::translate("The file extension FLEX is not supported by the audio format manager.").replace("FLEX", fileExtension);
        juce::AlertWindow::showMessageBox(icon, title, message);
    }
}

void Document::Director::addAnalysis(AlertType alertType)
{
    if(mModalWindow != nullptr)
    {
        mModalWindow->exitModalState(0);
        mModalWindow = nullptr;
    }
    
    mPluginListTable.onPluginSelected = [this, alertType](juce::String key)
    {
        if(mModalWindow != nullptr)
        {
            mModalWindow->exitModalState(0);
            mModalWindow = nullptr;
        }
        auto const name = PluginList::Scanner::getPluginDescriptions()[key].name;
        static auto const colour = juce::Colours::blue;
        static auto const colourMap = Analyzer::ColorMap::Heat;
        Analyzer::Container const ctnr {{key}, {name}, {0}, {{}}, {}, {colour}, {colourMap},  {}};
        if(!mAccessor.insertAccessor<Document::AttrType::analyzers>(-1, ctnr))
        {
            if(alertType == AlertType::window)
            {
                auto constexpr icon = juce::AlertWindow::AlertIconType::WarningIcon;
                auto const title = juce::translate("Analysis cannot be created");
                auto const message = juce::translate("The analysis ANNM cannot be inserted into the document.").replace("ANNM", name);
                juce::AlertWindow::showMessageBox(icon, title, message);
            }
            return;
        }
        
        auto& anlAcsr = mAccessor.getAccessors<Document::AttrType::analyzers>().back();
        Analyzer::PropertyPanel panel(anlAcsr);
        auto const title = juce::translate("Analyzer Properties");
        auto const& lookAndFeel = juce::Desktop::getInstance().getDefaultLookAndFeel();
        auto const bgColor = lookAndFeel.findColour(juce::ResizableWindow::backgroundColourId);
        juce::DialogWindow::showModalDialog(title, &panel, nullptr, bgColor, true, false, false);
    };
    
    auto const& lookAndFeel = juce::Desktop::getInstance().getDefaultLookAndFeel();
    auto const bgColor = lookAndFeel.findColour(juce::ResizableWindow::backgroundColourId);
    
    juce::DialogWindow::LaunchOptions o;
    o.dialogTitle = juce::translate("New Analyzer...");
    o.content.setNonOwned(&mPluginListTable);
    o.componentToCentreAround = nullptr;
    o.dialogBackgroundColour = bgColor;
    o.escapeKeyTriggersCloseButton = true;
    o.useNativeTitleBar = false;
    o.resizable = false;
    o.useBottomRightCornerResizer = false;
    mModalWindow = o.launchAsync();
    if(mModalWindow != nullptr)
    {
        mModalWindow->runModalLoop();
    }
}

ANALYSE_FILE_END
