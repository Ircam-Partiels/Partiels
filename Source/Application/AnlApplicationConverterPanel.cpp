#include "AnlApplicationConverterPanel.h"
#include "AnlApplicationInstance.h"
#include "AnlApplicationTools.h"

ANALYSE_FILE_BEGIN

Application::ConverterPanel::ConverterPanel()
: mPropertyOpen("Open", "Select a SDIF or a JSON file to convert", [&]()
                {
                    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Load a SDIF or a JSON file"), juce::File{}, "*.sdif;*.json");
                    if(mFileChooser == nullptr)
                    {
                        return;
                    }
                    using Flags = juce::FileBrowserComponent::FileChooserFlags;
                    mFileChooser->launchAsync(Flags::openMode | Flags::canSelectFiles, [this](juce::FileChooser const& fileChooser)
                                              {
                                                  auto const results = fileChooser.getResults();
                                                  if(results.isEmpty())
                                                  {
                                                      return;
                                                  }
                                                  setFile(results.getFirst());
                                              });
                })
, mPropertyName("File", "The SDIF/JSON file to convert", nullptr)

, mPropertyToSdifFrame("Frame", "Define the frame signature to encode the results in the SDIF file", [this](juce::String const& text)
                       {
                           juce::ignoreUnused(text);
                           sdifAttributeUpdated();
                       })
, mPropertyToSdifMatrix("Matrix", "Define the matrix signature to encode the results in the SDIF file", [this](juce::String const& text)
                        {
                            juce::ignoreUnused(text);
                            sdifAttributeUpdated();
                        })
, mPropertyToSdifColName("Column Name", "Define the name of the column to encode the results in the SDIF file", nullptr)
, mPropertyToSdifExport("Convert to SDIF", "Convert the JSON file to a SDIF file", [&]()
                        {
                            exportToSdif();
                        })

, mPropertyToJsonFrame("Frame", "Select the frame signature to decode from the SDIF file", "", {}, [&](size_t index)
                       {
                           juce::ignoreUnused(index);
                           selectedFrameUpdated();
                       })
, mPropertyToJsonMatrix("Matrix", "Select the matrix signature to decode from the SDIF file", "", {}, [&](size_t index)
                        {
                            juce::ignoreUnused(index);
                            selectedMatrixUpdated();
                        })
, mPropertyToJsonRow("Row", "Select the row(s) to decode from the SDIF file", "", {}, [&](size_t index)
                     {
                         juce::ignoreUnused(index);
                         selectedRowColumnUpdated();
                     })
, mPropertyToJsonColumn("Column", "Select the colum(s) to decode from the SDIF file", "", {}, [&](size_t index)
                        {
                            juce::ignoreUnused(index);
                            selectedRowColumnUpdated();
                        })
, mPropertyToJsonUnit("Unit", "Define the unit of the results", [&](juce::String const& text)
                      {
                          mPropertyToJsonMinValue.entry.setTextValueSuffix(text);
                          mPropertyToJsonMaxValue.entry.setTextValueSuffix(text);
                      })
, mPropertyToJsonMinValue("Value Range Min.", "Define the minimum value of the results.", "", {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()}, 0.0f, [&](float value)
                          {
                              auto const max = std::max(static_cast<float>(mPropertyToJsonMaxValue.entry.getValue()), value);
                              mPropertyToJsonMaxValue.entry.setValue(max, juce::NotificationType::dontSendNotification);
                          })
, mPropertyToJsonMaxValue("Value Range Max.", "Define the maximum value of the results.", "", {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max()}, 0.0f, [&](float value)
                          {
                              auto const min = std::min(static_cast<float>(mPropertyToJsonMinValue.entry.getValue()), value);
                              mPropertyToJsonMinValue.entry.setValue(min, juce::NotificationType::dontSendNotification);
                          })
, mPropertyToJsonLoadInDocument("Load In Current Document", "Load the JSON file directly in the current document", [](bool state)
                                {
                                    Instance::get().getApplicationAccessor().setAttr<AttrType::autoLoadConvertedFile>(state, NotificationType::synchronous);
                                })
, mPropertyToJsonExport("Convert to JSON", "Convert the SDIF file to a JSON file", [&]()
                        {
                            exportToJson();
                        })
{
    mPropertyName.entry.setEnabled(false);
    addAndMakeVisible(mPropertyOpen);
    addAndMakeVisible(mPropertyName);

    addChildComponent(mPropertyToSdifFrame);
    addChildComponent(mPropertyToSdifMatrix);
    addChildComponent(mPropertyToSdifColName);
    addChildComponent(mPropertyToSdifExport);
    mPropertyToSdifExport.entry.addShortcut(juce::KeyPress(juce::KeyPress::returnKey));
    mPropertyToSdifFrame.entry.onEditorShow = [this]()
    {
        if(auto* textEditor = mPropertyToSdifFrame.entry.getCurrentTextEditor())
        {
            textEditor->setInputRestrictions(4);
            textEditor->setJustification(juce::Justification::right);
        }
    };
    mPropertyToSdifFrame.entry.setText("????", juce::NotificationType::dontSendNotification);
    mPropertyToSdifMatrix.entry.onEditorShow = [this]()
    {
        if(auto* textEditor = mPropertyToSdifMatrix.entry.getCurrentTextEditor())
        {
            textEditor->setInputRestrictions(4);
            textEditor->setJustification(juce::Justification::right);
        }
    };
    mPropertyToSdifMatrix.entry.setText("????", juce::NotificationType::dontSendNotification);

    addChildComponent(mPropertyToJsonFrame);
    addChildComponent(mPropertyToJsonMatrix);
    addChildComponent(mPropertyToJsonRow);
    addChildComponent(mPropertyToJsonColumn);
    addChildComponent(mPropertyToJsonUnit);
    addChildComponent(mPropertyToJsonMinValue);
    addChildComponent(mPropertyToJsonMaxValue);
    addChildComponent(mPropertyToJsonLoadInDocument);
    addChildComponent(mPropertyToJsonExport);
    mPropertyToJsonExport.entry.addShortcut(juce::KeyPress(juce::KeyPress::returnKey));
    mPropertyToJsonFrame.entry.setTextWhenNoChoicesAvailable(juce::translate("No frame available"));
    mPropertyToJsonMatrix.entry.setTextWhenNoChoicesAvailable(juce::translate("No matrix available"));
    mPropertyToJsonRow.entry.setTextWhenNoChoicesAvailable(juce::translate("No row available"));
    mPropertyToJsonColumn.entry.setTextWhenNoChoicesAvailable(juce::translate("No column available"));

    mInfos.setSize(300, 72);
    mInfos.setText(juce::translate("Select or drag and drop a SDIF file to convert to a JSON file or a JSON file to convert to a SDIF file."));
    mInfos.setMultiLine(true);
    mInfos.setReadOnly(true);
    addAndMakeVisible(mInfos);

    sdifAttributeUpdated();
    setFile({});
    setSize(300, 200);

    auto updateLoadInDocument = [this]()
    {
        auto const hasReader = !Instance::get().getDocumentAccessor().getAttr<Document::AttrType::reader>().empty();
        auto const automaticLoad = Instance::get().getApplicationAccessor().getAttr<AttrType::autoLoadConvertedFile>();
        mPropertyToJsonLoadInDocument.setEnabled(hasReader);
        mPropertyToJsonLoadInDocument.entry.setToggleState(hasReader && automaticLoad, juce::NotificationType::dontSendNotification);
    };

    mListener.onAttrChanged = [=](Accessor const& accessor, AttrType attr)
    {
        juce::ignoreUnused(accessor);
        switch(attr)
        {
            case AttrType::windowState:
            case AttrType::recentlyOpenedFilesList:
            case AttrType::currentDocumentFile:
            case AttrType::colourMode:
            case AttrType::showInfoBubble:
            case AttrType::exportOptions:
            case AttrType::adaptationToSampleRate:
            case AttrType::desktopGlobalScaleFactor:
                break;
            case AttrType::autoLoadConvertedFile:
            {
                updateLoadInDocument();
            }
            break;
        }
    };

    mDocumentListener.onAttrChanged = [=](Document::Accessor const& accessor, Document::AttrType attr)
    {
        juce::ignoreUnused(accessor);
        switch(attr)
        {
            case Document::AttrType::reader:
            {
                updateLoadInDocument();
            }
            break;
            case Document::AttrType::layout:
            case Document::AttrType::viewport:
            case Document::AttrType::path:
            case Document::AttrType::grid:
            case Document::AttrType::autoresize:
            case Document::AttrType::samplerate:
                break;
        }
    };

    Instance::get().getApplicationAccessor().addListener(mListener, NotificationType::synchronous);
    Instance::get().getDocumentAccessor().addListener(mDocumentListener, NotificationType::synchronous);
}

Application::ConverterPanel::~ConverterPanel()
{
    Instance::get().getDocumentAccessor().removeListener(mDocumentListener);
    Instance::get().getApplicationAccessor().removeListener(mListener);
}

void Application::ConverterPanel::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyOpen);
    setBounds(mPropertyName);

    setBounds(mPropertyToSdifFrame);
    setBounds(mPropertyToSdifMatrix);
    setBounds(mPropertyToSdifColName);
    setBounds(mPropertyToSdifExport);

    setBounds(mPropertyToJsonFrame);
    setBounds(mPropertyToJsonMatrix);
    setBounds(mPropertyToJsonRow);
    setBounds(mPropertyToJsonColumn);
    setBounds(mPropertyToJsonUnit);
    setBounds(mPropertyToJsonMinValue);
    setBounds(mPropertyToJsonMaxValue);
    setBounds(mPropertyToJsonLoadInDocument);
    setBounds(mPropertyToJsonExport);

    setBounds(mInfos);

    setSize(300, std::max(bounds.getY(), 120) + 2);
}

void Application::ConverterPanel::paintOverChildren(juce::Graphics& g)
{
    if(mFileIsDragging)
    {
        g.fillAll(findColour(juce::TextButton::ColourIds::buttonColourId).withAlpha(0.5f));
    }
}

void Application::ConverterPanel::lookAndFeelChanged()
{
    auto const text = mInfos.getText();
    mInfos.clear();
    mInfos.setText(text);
}

void Application::ConverterPanel::parentHierarchyChanged()
{
    lookAndFeelChanged();
}

bool Application::ConverterPanel::isInterestedInFileDrag(juce::StringArray const& files)
{
    return std::any_of(files.begin(), files.end(), [](auto const& path)
                       {
                           juce::File const file(path);
                           return file.hasFileExtension("sdif") || file.hasFileExtension("json");
                       });
}

void Application::ConverterPanel::fileDragEnter(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(files, x, y);
    mFileIsDragging = true;
    repaint();
}

void Application::ConverterPanel::fileDragExit(juce::StringArray const& files)
{
    juce::ignoreUnused(files);
    mFileIsDragging = false;
    repaint();
}

void Application::ConverterPanel::filesDropped(juce::StringArray const& files, int x, int y)
{
    juce::ignoreUnused(files, x, y);
    mFileIsDragging = false;
    repaint();
    for(auto const& path : files)
    {
        juce::File const file(path);
        if(file.hasFileExtension("sdif") || file.hasFileExtension("json"))
        {
            setFile(file);
            return;
        }
    }
}

void Application::ConverterPanel::setFile(juce::File const& file)
{
    mFile = file;
    if(file.hasFileExtension("json"))
    {
        mInfos.setVisible(false);
        mPropertyName.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);

        mPropertyToSdifFrame.setVisible(true);
        mPropertyToSdifMatrix.setVisible(true);
        mPropertyToSdifColName.setVisible(true);
        mPropertyToSdifExport.setVisible(true);

        mPropertyToJsonFrame.setVisible(false);
        mPropertyToJsonMatrix.setVisible(false);
        mPropertyToJsonRow.setVisible(false);
        mPropertyToJsonColumn.setVisible(false);
        mPropertyToJsonExport.setVisible(false);
        mPropertyToJsonUnit.setVisible(false);
        mPropertyToJsonMinValue.setVisible(false);
        mPropertyToJsonMaxValue.setVisible(false);
        mPropertyToJsonLoadInDocument.setVisible(false);
    }
    else if(file.hasFileExtension("sdif"))
    {
        mInfos.setVisible(false);
        mPropertyName.entry.setText(file.getFileName(), juce::NotificationType::dontSendNotification);

        mPropertyToSdifFrame.setVisible(false);
        mPropertyToSdifMatrix.setVisible(false);
        mPropertyToSdifColName.setVisible(false);
        mPropertyToSdifExport.setVisible(false);

        mPropertyToJsonFrame.setVisible(true);
        mPropertyToJsonMatrix.setVisible(true);
        mPropertyToJsonRow.setVisible(true);
        mPropertyToJsonColumn.setVisible(true);
        mPropertyToJsonUnit.setVisible(true);
        mPropertyToJsonMinValue.setVisible(true);
        mPropertyToJsonMaxValue.setVisible(true);
        mPropertyToJsonExport.setVisible(true);
        mPropertyToJsonLoadInDocument.setVisible(true);

        mEntries = SdifConverter::getEntries(file);

        // Update of the frames
        mPropertyToJsonFrame.entry.clear(juce::NotificationType::dontSendNotification);
        mFrameSigLinks.clear();

        for(auto const& frame : mEntries)
        {
            auto const frameSignature = frame.first;
            mFrameSigLinks.push_back(frameSignature);
            mPropertyToJsonFrame.entry.addItem(SdifConverter::getString(frameSignature), static_cast<int>(mFrameSigLinks.size()));
        }
        mPropertyToJsonFrame.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
        mPropertyToJsonFrame.entry.setEnabled(mPropertyToJsonFrame.entry.getNumItems() > 0);

        selectedFrameUpdated();
    }
    else
    {
        mInfos.setVisible(true);
        mPropertyName.entry.setText(file == juce::File{} ? juce::translate("No file selected") : juce::translate("FLNAME: File extension not supported").replace("FLNAME", file.getFileName()), juce::NotificationType::dontSendNotification);

        mPropertyToSdifFrame.setVisible(false);
        mPropertyToSdifMatrix.setVisible(false);
        mPropertyToSdifColName.setVisible(false);
        mPropertyToSdifExport.setVisible(false);

        mPropertyToJsonFrame.setVisible(false);
        mPropertyToJsonMatrix.setVisible(false);
        mPropertyToJsonRow.setVisible(false);
        mPropertyToJsonColumn.setVisible(false);
        mPropertyToJsonExport.setVisible(false);
        mPropertyToJsonUnit.setVisible(false);
        mPropertyToJsonMinValue.setVisible(false);
        mPropertyToJsonMaxValue.setVisible(false);
        mPropertyToJsonLoadInDocument.setVisible(false);
    }
    resized();
}

void Application::ConverterPanel::sdifAttributeUpdated()
{
    auto getFormattedText = [](juce::String text)
    {
        while(text.length() < 4)
        {
            text += "?";
        }
        return text.substring(0, 4).toUpperCase();
    };
    auto const matrixText = getFormattedText(mPropertyToSdifMatrix.entry.getText().toUpperCase());
    mPropertyToSdifMatrix.entry.setText(matrixText, juce::NotificationType::dontSendNotification);
    auto const frameText = getFormattedText(mPropertyToSdifFrame.entry.getText().toUpperCase());
    mPropertyToSdifFrame.entry.setText(frameText, juce::NotificationType::dontSendNotification);
    mPropertyToSdifExport.setEnabled(!matrixText.contains("?") && !frameText.contains("?"));
}

void Application::ConverterPanel::selectedFrameUpdated()
{
    mPropertyToJsonMatrix.entry.clear(juce::NotificationType::dontSendNotification);
    mMatrixSigLinks.clear();
    auto const selectedFrameIndex = mPropertyToJsonFrame.entry.getSelectedItemIndex();
    if(selectedFrameIndex < 0 || static_cast<size_t>(selectedFrameIndex) > mFrameSigLinks.size())
    {
        mPropertyToSdifFrame.entry.setText("????", juce::NotificationType::dontSendNotification);
        mPropertyToJsonMatrix.entry.setEnabled(false);
        mPropertyToJsonRow.entry.setEnabled(false);
        selectedMatrixUpdated();
        return;
    }
    auto const frameIdentifier = mFrameSigLinks[static_cast<size_t>(selectedFrameIndex)];
    if(mEntries.count(frameIdentifier) == 0_z)
    {
        mPropertyToSdifFrame.entry.setText("????", juce::NotificationType::dontSendNotification);
        mPropertyToJsonMatrix.entry.setEnabled(false);
        mPropertyToJsonRow.entry.setEnabled(false);
        selectedMatrixUpdated();
        return;
    }

    mPropertyToSdifFrame.entry.setText(SdifConverter::getString(frameIdentifier), juce::NotificationType::dontSendNotification);
    auto const& matrixEntries = mEntries.at(frameIdentifier);
    for(auto const& matrix : matrixEntries)
    {
        auto const matrixSignature = matrix.first;
        mMatrixSigLinks.push_back(matrixSignature);
        mPropertyToJsonMatrix.entry.addItem(SdifConverter::getString(matrixSignature), static_cast<int>(mMatrixSigLinks.size()));
    }
    mPropertyToJsonMatrix.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyToJsonMatrix.entry.setEnabled(mPropertyToJsonMatrix.entry.getNumItems() > 0);

    selectedMatrixUpdated();
}

void Application::ConverterPanel::selectedMatrixUpdated()
{
    mPropertyToJsonRow.entry.clear(juce::NotificationType::dontSendNotification);
    mPropertyToJsonColumn.entry.clear(juce::NotificationType::dontSendNotification);

    auto const selectedFrameIndex = mPropertyToJsonFrame.entry.getSelectedItemIndex();
    auto const selectedMatrixIndex = mPropertyToJsonMatrix.entry.getSelectedItemIndex();
    if(selectedFrameIndex < 0 || static_cast<size_t>(selectedFrameIndex) > mFrameSigLinks.size() || selectedMatrixIndex < 0 || static_cast<size_t>(selectedMatrixIndex) > mMatrixSigLinks.size())
    {
        mPropertyToSdifMatrix.entry.setText("????", juce::NotificationType::dontSendNotification);
        mPropertyToJsonRow.entry.setEnabled(false);
        mPropertyToJsonColumn.entry.setEnabled(false);
        mPropertyToJsonExport.entry.setEnabled(false);
        return;
    }

    auto const frameIdentifier = mFrameSigLinks[static_cast<size_t>(selectedFrameIndex)];
    auto const matrixIdentifier = mMatrixSigLinks[static_cast<size_t>(selectedMatrixIndex)];
    if(mEntries.count(frameIdentifier) == 0_z || mEntries.at(frameIdentifier).count(matrixIdentifier) == 0_z)
    {
        mPropertyToSdifMatrix.entry.setText("????", juce::NotificationType::dontSendNotification);
        mPropertyToJsonRow.entry.setEnabled(false);
        mPropertyToJsonColumn.entry.setEnabled(false);
        mPropertyToJsonExport.entry.setEnabled(false);
        return;
    }

    mPropertyToSdifMatrix.entry.setText(SdifConverter::getString(matrixIdentifier), juce::NotificationType::dontSendNotification);
    auto const matrixSize = mEntries.at(frameIdentifier).at(matrixIdentifier);

    if(matrixSize.first > 1_z)
    {
        mPropertyToJsonRow.entry.addItem("All", 1);
    }
    for(auto row = 0_z; row < matrixSize.first; ++row)
    {
        mPropertyToJsonRow.entry.addItem(juce::String(row), static_cast<int>(row + 2));
    }
    mPropertyToJsonRow.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyToJsonRow.entry.setEnabled(mPropertyToJsonRow.entry.getNumItems() > 0);

    if(matrixSize.second.size() > 1_z)
    {
        mPropertyToJsonColumn.entry.addItem("All", 1);
    }
    for(auto column = 0_z; column < matrixSize.second.size(); ++column)
    {
        mPropertyToJsonColumn.entry.addItem(matrixSize.second[column], static_cast<int>(column + 2));
    }
    mPropertyToJsonColumn.entry.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
    mPropertyToJsonColumn.entry.setEnabled(mPropertyToJsonColumn.entry.getNumItems() > 0);

    mPropertyToJsonExport.entry.setEnabled(mPropertyToJsonRow.entry.getNumItems() > 0 && mPropertyToJsonColumn.entry.getNumItems() > 0);
    selectedRowColumnUpdated();
}

void Application::ConverterPanel::selectedRowColumnUpdated()
{
    auto const row = mPropertyToJsonRow.entry.getSelectedId() - 1;
    auto const column = mPropertyToJsonColumn.entry.getSelectedId() - 1;
    if(row < 0 || column < 0)
    {
        anlWeakAssert(false);
        return;
    }
    mPropertyToJsonColumn.entry.setItemEnabled(1, row != 0);
    if(row == 0 && column == 0)
    {
        mPropertyToJsonColumn.entry.setSelectedId(2, juce::NotificationType::dontSendNotification);
    }
}

void Application::ConverterPanel::exportToSdif()
{
    auto const frameName = mPropertyToSdifFrame.entry.getText();
    auto const matrixName = mPropertyToSdifMatrix.entry.getText();
    auto const colName = mPropertyToSdifColName.entry.getText();
    if(frameName.isEmpty() || matrixName.isEmpty())
    {
        anlWeakAssert(false);
        return;
    }

    if(frameName.contains("?") || matrixName.contains("?") || frameName.length() != 4 || matrixName.length() != 4)
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::WarningIcon)
                                 .withTitle(juce::translate("Failed to convert JSON file..."))
                                 .withMessage(juce::translate("The frame and matrix signatures must contain 4 characters."))
                                 .withButton(juce::translate("Ok"));
        return;
    }

    auto const frameIdentifier = SdifConverter::getSignature(frameName);
    auto const matrixIdentifier = SdifConverter::getSignature(matrixName);

    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Select a SDIF file"), mFile.withFileExtension("sdif"), "*.sdif");
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    mFileChooser->launchAsync(Flags::saveMode | Flags::canSelectFiles | Flags::warnAboutOverwriting, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  auto const results = fileChooser.getResults();
                                  if(results.isEmpty())
                                  {
                                      return;
                                  }
                                  auto const sdifFile = results.getFirst();
                                  enterModalState();
                                  auto const result = SdifConverter::fromJson(mFile, sdifFile, frameIdentifier, matrixIdentifier, colName.isEmpty() ? std::optional<juce::String>() : colName);
                                  exitModalState(0);
                                  if(result.wasOk())
                                  {
                                      auto const options = juce::MessageBoxOptions()
                                                               .withIconType(juce::AlertWindow::InfoIcon)
                                                               .withTitle(juce::translate("Conversion succeeded!"))
                                                               .withMessage(juce::translate("The JSON file 'JSONFLNM' has been successfully converted to the SDIF file 'SDIFFLNM'.").replace("JSONFLNM", mFile.getFullPathName()).replace("SDIFFLNM", sdifFile.getFullPathName()))
                                                               .withButton(juce::translate("Ok"));
                                      juce::AlertWindow::showAsync(options, nullptr);
                                  }
                                  else
                                  {
                                      auto const options = juce::MessageBoxOptions()
                                                               .withIconType(juce::AlertWindow::WarningIcon)
                                                               .withTitle(juce::translate("Failed to convert JSON file..."))
                                                               .withMessage(juce::translate("There was an error while trying to convert the SDIF file 'JSONFLNM' to the SDIF file 'SDIFFLNM'.").replace("JSONFLNM", mFile.getFullPathName()).replace("SDIFFLNM", sdifFile.getFullPathName()) + "\n\n" + result.getErrorMessage())
                                                               .withButton(juce::translate("Ok"));
                                      juce::AlertWindow::showAsync(options, nullptr);
                                  }
                              });
}

void Application::ConverterPanel::exportToJson()
{
    auto const selectedFrameIndex = mPropertyToJsonFrame.entry.getSelectedItemIndex();
    auto const selectedMatrixIndex = mPropertyToJsonMatrix.entry.getSelectedItemIndex();
    if(selectedFrameIndex < 0 || static_cast<size_t>(selectedFrameIndex) > mFrameSigLinks.size() || selectedMatrixIndex < 0 || static_cast<size_t>(selectedMatrixIndex) > mMatrixSigLinks.size())
    {
        anlWeakAssert(false);
        return;
    }

    auto const frameIdentifier = mFrameSigLinks[static_cast<size_t>(selectedFrameIndex)];
    auto const matrixIdentifier = mMatrixSigLinks[static_cast<size_t>(selectedMatrixIndex)];
    if(mEntries.count(frameIdentifier) == 0_z || mEntries.at(frameIdentifier).count(matrixIdentifier) == 0_z)
    {
        anlWeakAssert(false);
        return;
    }

    auto const row = mPropertyToJsonRow.entry.getSelectedId() - 1;
    auto const column = mPropertyToJsonColumn.entry.getSelectedId() - 1;
    if(row < 0 || column < 0)
    {
        anlWeakAssert(false);
        return;
    }

    auto const unit = mPropertyToJsonUnit.entry.getText();
    juce::Range<double> const range{mPropertyToJsonMinValue.entry.getValue(), mPropertyToJsonMaxValue.entry.getValue()};
    auto extra = std::optional<nlohmann::json>{};
    if(!unit.isEmpty() || !range.isEmpty())
    {
        auto json = nlohmann::json::object();
        auto& output = json["track"]["description"]["output"];
        if(!unit.isEmpty())
        {
            output["unit"] = unit.toStdString();
        }
        if(!range.isEmpty())
        {
            output["minValue"] = range.getStart();
            output["maxValue"] = range.getEnd();
            output["hasKnownExtents"] = true;
        }
        extra = std::optional<nlohmann::json>(std::move(json));
    }

    auto const loadInDocument = mPropertyToJsonLoadInDocument.entry.getToggleState();
    auto const position = Tools::getNewTrackPosition();
    mFileChooser = std::make_unique<juce::FileChooser>(juce::translate("Select a JSON file"), mFile.withFileExtension("json"), "*.json");
    if(mFileChooser == nullptr)
    {
        return;
    }
    using Flags = juce::FileBrowserComponent::FileChooserFlags;
    mFileChooser->launchAsync(Flags::saveMode | Flags::canSelectFiles | Flags::warnAboutOverwriting, [=, this](juce::FileChooser const& fileChooser)
                              {
                                  auto const results = fileChooser.getResults();
                                  if(results.isEmpty())
                                  {
                                      return;
                                  }
                                  auto const jsonFile = results.getFirst();
                                  enterModalState();
                                  auto const result = SdifConverter::toJson(mFile, jsonFile, frameIdentifier, matrixIdentifier, row == 0 ? std::optional<size_t>{} : static_cast<size_t>(row - 1), column == 0 ? std::optional<size_t>{} : static_cast<size_t>(column - 1), extra);
                                  exitModalState(0);
                                  if(result.wasOk())
                                  {
                                      if(loadInDocument)
                                      {
                                          Tools::addFileTrack(position, jsonFile);
                                      }
                                      else
                                      {
                                          auto const options = juce::MessageBoxOptions()
                                                                   .withIconType(juce::AlertWindow::InfoIcon)
                                                                   .withTitle(juce::translate("Conversion succeeded!"))
                                                                   .withMessage(juce::translate("The SDIF file 'SDIFFLNM' has been successfully converted to the JSON file 'JSONFLNM'.").replace("SDIFFLNM", mFile.getFullPathName()).replace("JSONFLNM", jsonFile.getFullPathName()))
                                                                   .withButton(juce::translate("Ok"));
                                          juce::AlertWindow::showAsync(options, nullptr);
                                      }
                                  }
                                  else
                                  {
                                      auto const options = juce::MessageBoxOptions()
                                                               .withIconType(juce::AlertWindow::WarningIcon)
                                                               .withTitle(juce::translate("Failed to convert SDIF file..."))
                                                               .withMessage(juce::translate("There was an error while trying to convert the SDIF file 'SDIFFLNM' to the JSON file 'JSONFLNM'.").replace("SDIFFLNM", mFile.getFullPathName()).replace("JSONFLNM", jsonFile.getFullPathName()) + "\n\n" + result.getErrorMessage())
                                                               .withButton(juce::translate("Ok"));
                                      juce::AlertWindow::showAsync(options, nullptr);
                                  }
                              });
}

ANALYSE_FILE_END
