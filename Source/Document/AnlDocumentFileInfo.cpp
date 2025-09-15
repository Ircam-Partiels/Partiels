#include "AnlDocumentFileInfo.h"
#include "AnlApplicationInstance.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Document::FileInfoContent::FileInfoContent(Director& director)
: mDirector(director)
, mFileButton(juce::ImageCache::getFromMemory(AnlIconsData::documentfile_png, AnlIconsData::documentfile_pngSize))
, mDescriptionLabel("description", juce::translate("Description:"))
, mDescriptionEditor("description")
{
    addAndMakeVisible(mFileButton);
    addAndMakeVisible(mDescriptionLabel);
    addAndMakeVisible(mDescriptionEditor);

    mFileButton.setWantsKeyboardFocus(false);
    mFileButton.onClick = [this]()
    {
        auto const file = mAccessor.getAttr<AttrType::path>();
        if(file != juce::File{})
        {
            file.revealToUser();
        }
    };

    mDescriptionLabel.setJustificationType(juce::Justification::centredLeft);
    mDescriptionLabel.attachToComponent(&mDescriptionEditor, true);

    mDescriptionEditor.setMultiLine(true);
    mDescriptionEditor.setReturnKeyStartsNewLine(true);
    mDescriptionEditor.setPopupMenuEnabled(true);
    mDescriptionEditor.setScrollbarsShown(true);
    mDescriptionEditor.onTextChange = [this]()
    {
        mAccessor.setAttr<AttrType::description>(mDescriptionEditor.getText(), NotificationType::synchronous);
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::path:
            {
                auto const file = acsr.getAttr<AttrType::path>();
                mFileButton.setTooltip(file != juce::File{} ? file.getFullPathName() : juce::translate("Document not saved"));
                resized();
            }
            break;
            case AttrType::description:
            {
                auto const description = acsr.getAttr<AttrType::description>();
                if(mDescriptionEditor.getText() != description)
                {
                    mDescriptionEditor.setText(description, juce::NotificationType::dontSendNotification);
                }
            }
            break;
            case AttrType::reader:
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::grid:
            case AttrType::autoresize:
            case AttrType::samplerate:
            case AttrType::channels:
            case AttrType::editMode:
            case AttrType::drawingState:
                break;
        }
    };

    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(400, 300);
}

Document::FileInfoContent::~FileInfoContent()
{
    mAccessor.removeListener(mListener);
}

void Document::FileInfoContent::resized()
{
    auto bounds = getLocalBounds();
    static auto constexpr margin = 8;
    static auto constexpr buttonHeight = 32;
    static auto constexpr labelWidth = 80;
    
    bounds.removeFromTop(margin);
    
    // File button row
    auto fileRow = bounds.removeFromTop(buttonHeight);
    fileRow.removeFromLeft(margin);
    fileRow.removeFromRight(margin);
    mFileButton.setBounds(fileRow.removeFromLeft(buttonHeight));
    
    bounds.removeFromTop(margin);
    
    // Description editor
    auto descRow = bounds.reduced(margin);
    descRow.removeFromLeft(labelWidth + margin); // Space for label
    mDescriptionEditor.setBounds(descRow);
}

Document::FileInfoPanel::FileInfoPanel()
: mFileInfoContent(Instance::get().getDocumentDirector())
{
    setContent(juce::translate("File Info"), std::addressof(mFileInfoContent));
}

Document::FileInfoPanel::~FileInfoPanel()
{
    setContent("", nullptr);
}

bool Document::FileInfoPanel::escapeKeyPressed()
{
    hide();
    return true;
}

ANALYSE_FILE_END