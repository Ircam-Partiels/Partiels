#include "AnlDocumentFileInfo.h"
#include <AnlIconsData.h>

ANALYSE_FILE_BEGIN

Document::FileInfoContent::FileInfoContent(Director& director, juce::ApplicationCommandManager& commandManager)
: mDirector(director)
, mApplicationCommandManager(commandManager)
, mNameLabel(juce::translate("File"), juce::translate("Document not saved"))
, mDirectoryLabel(juce::translate("Directory"), juce::translate("Document not saved"))
, mRevealButton(juce::translate("Save to..."), juce::translate("Document not saved"), [this]()
                {
                    auto const file = mAccessor.getAttr<AttrType::path>();
                    if(file != juce::File{})
                    {
                        mAccessor.getAttr<AttrType::path>().revealToUser();
                    }
                    else
                    {
                        mApplicationCommandManager.invokeDirectly(ApplicationCommandIDs::documentSave, true);
                    }
                })
, mDescriptionLabel("description", juce::translate("Description:"))
, mDescriptionEditor("description")
{
    addAndMakeVisible(mNameLabel);
    addAndMakeVisible(mDirectoryLabel);
    addAndMakeVisible(mRevealButton);
    addAndMakeVisible(mSeparator);
    addAndMakeVisible(mDescriptionLabel);
    addAndMakeVisible(mDescriptionEditor);

    mSeparator.setSize(200, 1);

    mDescriptionLabel.setJustificationType(juce::Justification::centredLeft);
    mDescriptionLabel.setSize(300, 24);

    mDescriptionEditor.setMultiLine(true);
    mDescriptionEditor.setReturnKeyStartsNewLine(true);
    mDescriptionEditor.setEscapeAndReturnKeysConsumed(true);
    mDescriptionEditor.setPopupMenuEnabled(true);
    mDescriptionEditor.setScrollbarsShown(true);
    mDescriptionEditor.setTabKeyUsedAsCharacter(false);
    mDescriptionEditor.onEscapeKey = [this]()
    {
        mDescriptionEditor.giveAwayKeyboardFocus();
    };
    mDescriptionEditor.onFocusLost = [this]()
    {
        mDirector.startAction();
        auto const text = mDescriptionEditor.getText().trim();
        mAccessor.setAttr<AttrType::description>(text, NotificationType::synchronous);
        mDirector.endAction(ActionState::newTransaction, juce::translate("Edit document description"));
    };

    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::path:
            {
                auto const file = acsr.getAttr<AttrType::path>();
                mNameLabel.entry.setText(file != juce::File{} ? file.getFileName() : juce::translate("Untitled"), juce::NotificationType::dontSendNotification);
                mNameLabel.setTooltip(file != juce::File{} ? file.getFullPathName() : juce::translate("Document not saved"));

                mDirectoryLabel.entry.setText(file != juce::File{} ? file.getParentDirectory().getFullPathName() : juce::translate("Undefined"), juce::NotificationType::dontSendNotification);
                mDirectoryLabel.setTooltip(file != juce::File{} ? file.getFullPathName() : juce::translate("Document not saved"));

#if JUCE_MAC
                mRevealButton.entry.setButtonText(file != juce::File{} ? juce::translate("Show in finder") : juce::translate("Save to..."));
#elif JUCE_WINDOWS
                mRevealButton.entry.setButtonText(file != juce::File{} ? juce::translate("Show in directory") : juce::translate("Save to..."));
#else
                mRevealButton.entry.setButtonText(file != juce::File{} ? juce::translate("Show in file explorer") : juce::translate("Save to..."));
#endif
                mRevealButton.setTooltip(file != juce::File{} ? file.getFullPathName() : juce::translate("Document not saved"));

                resized();
            }
            break;
            case AttrType::description:
            {
                auto const description = acsr.getAttr<AttrType::description>();
                if(mDescriptionEditor.getText() != description)
                {
                    mDescriptionEditor.setColour(juce::TextEditor::ColourIds::textColourId, findColour(juce::TextEditor::ColourIds::textColourId));
                    mDescriptionEditor.setText(description, false);
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
    setSize(300, 200);
}

Document::FileInfoContent::~FileInfoContent()
{
    mAccessor.removeListener(mListener);
}

void Document::FileInfoContent::resized()
{
    auto bounds = getLocalBounds().withHeight(200);
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mNameLabel);
    setBounds(mDirectoryLabel);
    setBounds(mRevealButton);
    setBounds(mSeparator);
    setBounds(mDescriptionLabel);
    mDescriptionEditor.setSize(bounds.getWidth(), bounds.getHeight());
    setBounds(mDescriptionEditor);
}

void Document::FileInfoContent::colourChanged()
{
    mDescriptionEditor.setTextToShowWhenEmpty(juce::translate("Type information about the document"), findColour(juce::Label::ColourIds::textColourId).withAlpha(0.5f));
    mDescriptionEditor.applyColourToAllText(findColour(juce::TextEditor::ColourIds::textColourId));
}

void Document::FileInfoContent::parentHierarchyChanged()
{
    colourChanged();
}

Document::FileInfoPanel::FileInfoPanel(Director& director, juce::ApplicationCommandManager& commandManager)
: mFileInfoContent(director, commandManager)
{
    setContent(juce::translate("Document Info"), std::addressof(mFileInfoContent));
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
